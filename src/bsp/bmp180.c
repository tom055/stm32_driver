#include "bmp180.h"

/* 使用 OSS=0：温度和气压转换各需最多 4.5ms。 */
typedef struct
{
    int16_t ac1, ac2, ac3, b1, b2, mb, mc, md;
    uint16_t ac4, ac5, ac6;
} bmp180_calibration_t;

static bmp180_calibration_t calibration;
static int ready;

static void delay_ms(uint32_t milliseconds)
{
    while (milliseconds-- != 0U)
    {
        uint32_t start = DWT->CYCCNT;
        while ((uint32_t)(DWT->CYCCNT - start) < SystemCoreClock / 1000U);
    }
}

static int wait_flag(uint32_t flag, FlagStatus expected)
{
    uint32_t start = DWT->CYCCNT;
    while (I2C_GetFlagStatus(I2C1, flag) != expected)
    {
        if (I2C_GetFlagStatus(I2C1, I2C_FLAG_AF) == SET ||
            (uint32_t)(DWT->CYCCNT - start) > SystemCoreClock / 10U)
        {
            I2C_GenerateSTOP(I2C1, ENABLE);
            I2C_ClearFlag(I2C1, I2C_FLAG_AF);
            I2C_AcknowledgeConfig(I2C1, ENABLE);
            return 0;
        }
    }
    return 1;
}

static void clear_addr(void)
{
    /* 32 位访问同时兼容标准库的 uint16_t 成员和 Wokwi 寄存器模型。 */
    (void)*(__IO uint32_t *)&I2C1->SR1;
    (void)*(__IO uint32_t *)&I2C1->SR2;
}

static int select_register(uint8_t reg)
{
    if (!wait_flag(I2C_FLAG_BUSY, RESET)) return 0;
    I2C_NACKPositionConfig(I2C1, I2C_NACKPosition_Current);
    I2C_AcknowledgeConfig(I2C1, ENABLE);
    I2C_GenerateSTART(I2C1, ENABLE);
    if (!wait_flag(I2C_FLAG_SB, SET)) return 0;
    I2C_Send7bitAddress(I2C1, BMP180_ADDRESS << 1, I2C_Direction_Transmitter);
    if (!wait_flag(I2C_FLAG_ADDR, SET)) return 0;
    clear_addr();
    I2C_SendData(I2C1, reg);
    return wait_flag(I2C_FLAG_BTF, SET);
}

static int write_register(uint8_t reg, uint8_t value)
{
    if (!select_register(reg)) return 0;
    I2C_SendData(I2C1, value);
    if (!wait_flag(I2C_FLAG_BTF, SET)) return 0;
    I2C_GenerateSTOP(I2C1, ENABLE);
    return wait_flag(I2C_FLAG_BUSY, RESET);
}

/* BMP180 保留寄存器指针，可在写地址和读数据之间 STOP。
 * 分成两次事务也兼容 Wokwi 的 STM32 I2C 模型。 */
static int start_read(uint8_t reg)
{
    if (!select_register(reg)) return 0;
    I2C_GenerateSTOP(I2C1, ENABLE);
    if (!wait_flag(I2C_FLAG_BUSY, RESET)) return 0;
    I2C_GenerateSTART(I2C1, ENABLE);
    if (!wait_flag(I2C_FLAG_SB, SET)) return 0;
    I2C_Send7bitAddress(I2C1, BMP180_ADDRESS << 1, I2C_Direction_Receiver);
    return wait_flag(I2C_FLAG_ADDR, SET);
}

static int read_u8(uint8_t reg, uint8_t *value)
{
    uint32_t saved_primask;
    if (!start_read(reg)) return 0;
    saved_primask = __get_PRIMASK();
    __disable_irq();
    I2C_AcknowledgeConfig(I2C1, DISABLE);
    clear_addr();
    I2C_GenerateSTOP(I2C1, ENABLE);
    __set_PRIMASK(saved_primask);
    if (!wait_flag(I2C_FLAG_RXNE, SET)) return 0;
    *value = I2C_ReceiveData(I2C1);
    if (!wait_flag(I2C_FLAG_BUSY, RESET)) return 0;
    I2C_AcknowledgeConfig(I2C1, ENABLE);
    return 1;
}

static int read_u16(uint8_t reg, uint16_t *value)
{
    uint8_t msb, lsb;
    /* 校准值固定，UT 也会保持到下一次测量，因此可分两次单字节读取。 */
    if (!read_u8(reg, &msb) || !read_u8((uint8_t)(reg + 1U), &lsb)) return 0;
    *value = ((uint16_t)msb << 8) | lsb;
    return 1;
}

static int read_pressure_raw(uint32_t *value)
{
    uint32_t saved_primask, msb, lsb, xlsb;
    if (!start_read(0xF6U)) return 0;
    clear_addr();
    /* 连续接收三字节：前两个 ACK，最后一个 NACK。 */
    if (!wait_flag(I2C_FLAG_BTF, SET)) return 0;
    saved_primask = __get_PRIMASK();
    __disable_irq();
    I2C_AcknowledgeConfig(I2C1, DISABLE);
    msb = I2C_ReceiveData(I2C1);
    I2C_GenerateSTOP(I2C1, ENABLE);
    lsb = I2C_ReceiveData(I2C1);
    __set_PRIMASK(saved_primask);
    if (!wait_flag(I2C_FLAG_RXNE, SET)) return 0;
    xlsb = I2C_ReceiveData(I2C1);
    if (!wait_flag(I2C_FLAG_BUSY, RESET)) return 0;
    I2C_AcknowledgeConfig(I2C1, ENABLE);
    *value = ((msb << 16) | (lsb << 8) | xlsb) >> 8; // OSS=0。
    return 1;
}

/* Bosch BMP180 数据手册第 15 页的整数补偿流程，OSS=0。
 * 使用 int64_t 中间值避免乘法溢出，负数乘法代替负数左移。 */
static int compensate(uint16_t ut, uint32_t up, int32_t *temperature, uint32_t *pressure)
{
    int64_t x1, x2, x3, b3, b5, b6, b4, b7, p;
    x1 = (((int64_t)ut - calibration.ac6) * calibration.ac5) >> 15;
    if (x1 + calibration.md == 0) return 0;
    x2 = ((int64_t)calibration.mc * 2048) / (x1 + calibration.md);
    b5 = x1 + x2;
    *temperature = (int32_t)((b5 + 8) >> 4);
    b6 = b5 - 4000;
    x1 = (calibration.b2 * ((b6 * b6) >> 12)) >> 11;
    x2 = ((int64_t)calibration.ac2 * b6) >> 11;
    x3 = x1 + x2;
    b3 = (calibration.ac1 * 4 + x3 + 2) >> 2;
    x1 = ((int64_t)calibration.ac3 * b6) >> 13;
    x2 = (calibration.b1 * ((b6 * b6) >> 12)) >> 16;
    x3 = (x1 + x2 + 2) >> 2;
    b4 = ((int64_t)calibration.ac4 * (x3 + 32768)) >> 15;
    b7 = ((int64_t)up - b3) * 50000;
    if (b4 <= 0 || b7 < 0) return 0;
    p = b7 < 0x80000000LL ? (b7 * 2) / b4 : (b7 / b4) * 2;
    x1 = (p >> 8) * (p >> 8);
    x1 = (x1 * 3038) >> 16;
    x2 = (-7357 * p) >> 16;
    p += (x1 + x2 + 3791) >> 4;
    if (p <= 0 || p > 200000) return 0;
    *pressure = (uint32_t)p;
    return 1;
}

int bmp180_init(void)
{
    GPIO_InitTypeDef gpio;
    I2C_InitTypeDef i2c;
    uint8_t id;
    uint16_t values[11];
    ready = 0;
    SystemCoreClockUpdate();
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
    GPIO_StructInit(&gpio);
    gpio.GPIO_Mode = GPIO_Mode_AF_OD;
    gpio.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio);
    I2C_DeInit(I2C1);
    I2C_StructInit(&i2c);
    i2c.I2C_ClockSpeed = 100000;
    i2c.I2C_Ack = I2C_Ack_Enable;
    I2C_Init(I2C1, &i2c);
    I2C_Cmd(I2C1, ENABLE);
    delay_ms(10U);
    if (!read_u8(0xD0U, &id) || id != 0x55U) return 0;
    for (uint8_t i = 0; i < 11U; ++i)
    {
        if (!read_u16((uint8_t)(0xAAU + i * 2U), &values[i]) ||
            values[i] == 0U || values[i] == 0xFFFFU) return 0;
    }
    calibration.ac1 = (int16_t)values[0];
    calibration.ac2 = (int16_t)values[1];
    calibration.ac3 = (int16_t)values[2];
    calibration.ac4 = values[3];
    calibration.ac5 = values[4];
    calibration.ac6 = values[5];
    calibration.b1 = (int16_t)values[6];
    calibration.b2 = (int16_t)values[7];
    calibration.mb = (int16_t)values[8];
    calibration.mc = (int16_t)values[9];
    calibration.md = (int16_t)values[10];
    ready = 1;
    return 1;
}

int bmp180_read(int32_t *temperature_deci_c, uint32_t *pressure_pa)
{
    uint16_t ut;
    uint32_t up;
    if (!ready || temperature_deci_c == 0 || pressure_pa == 0) return 0;
    if (!write_register(0xF4U, 0x2EU)) return 0; // 触发温度转换。
    delay_ms(5U);
    if (!read_u16(0xF6U, &ut)) return 0;
    if (!write_register(0xF4U, 0x34U)) return 0; // 触发气压转换，OSS=0。
    delay_ms(5U);
    if (!read_pressure_raw(&up)) return 0;
    return compensate(ut, up, temperature_deci_c, pressure_pa);
}
