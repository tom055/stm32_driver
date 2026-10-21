#include "lcd.h"

#define LCD_ADDRESS 0x27
#define LCD_RS 0x01U
#define LCD_ENABLE 0x04U
#define LCD_BACKLIGHT 0x08U

/* 使用 Cortex-M3 周期计数器延时，不占用 SysTick 或定时器中断。 */
static void lcd_delay_us(uint32_t microseconds)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t cycles = (SystemCoreClock / 1000000U) * microseconds;

    while ((uint32_t)(DWT->CYCCNT - start) < cycles);
}

static void lcd_i2c_start(void)
{
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) == SET);
    I2C_GenerateSTART(I2C1, ENABLE);
    /* 单独检查 SR1 标志，避免依赖模拟器未完整实现的 SR2 组合事件。 */
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_SB) == RESET);
    /* 标准库接口需要左移后的地址，最低位由传输方向决定。 */
    I2C_Send7bitAddress(I2C1, LCD_ADDRESS << 1, I2C_Direction_Transmitter);
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_ADDR) == RESET);
    /* 依次读取 SR1、SR2，清除 ADDR 并释放 SCL。 */
    /* 32 位访问也兼容 Wokwi；旧版标准库的成员类型为 uint16_t。 */
    (void)*(__IO uint32_t *)&I2C1->SR1;
    (void)*(__IO uint32_t *)&I2C1->SR2;
}

/* 每个 I2C 数据字节控制 PCF8574 的 8 个输出引脚。 */
static void lcd_write_expander(uint8_t value)
{
    I2C_SendData(I2C1, value);
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BTF) == RESET);
}

static void lcd_write_nibble(uint8_t value, uint8_t rs)
{
    /* P7~P4 = D7~D4，P3 = 背光，P2 = E，P1 = R/W，P0 = RS。
       RS=0 发送指令，RS=1 发送字符；R/W 始终为 0。 */
    uint8_t output = (value & 0xF0U) | LCD_BACKLIGHT | rs;

    lcd_write_expander(output);              // E=0：先稳定数据
    lcd_write_expander(output | LCD_ENABLE); // E=1
    lcd_write_expander(output);              // E=0：LCD 锁存半字节
}

static void lcd_write_byte(uint8_t value, uint8_t rs)
{
    lcd_i2c_start();
    lcd_write_nibble(value, rs);
    lcd_write_nibble((uint8_t)(value << 4), rs);
    I2C_GenerateSTOP(I2C1, ENABLE);
    lcd_delay_us(50U);
}

static void lcd_write_command(uint8_t command)
{
    lcd_write_byte(command, 0U);
    if (command == 0x01U || command == 0x02U)
    {
        /* 清屏和归位耗时较长，不能只等待普通指令的 50 us。 */
        lcd_delay_us(2000U);
    }
}

/* 上电同步阶段只发送一个半字节，此时还不能按完整字节发送。 */
static void lcd_write_init_nibble(uint8_t value)
{
    lcd_i2c_start();
    lcd_write_nibble(value, 0U);
    I2C_GenerateSTOP(I2C1, ENABLE);
}

void lcd_init(void)
{
    SystemCoreClockUpdate();
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
    GPIO_InitTypeDef gpioDef;
    GPIO_StructInit(&gpioDef);
    gpioDef.GPIO_Mode = GPIO_Mode_AF_OD;
    gpioDef.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7; // PB6=SCL，PB7=SDA
    gpioDef.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpioDef);

    I2C_DeInit(I2C1);
    I2C_InitTypeDef i2c;
    I2C_StructInit(&i2c);
    i2c.I2C_Ack = I2C_Ack_Enable;
    i2c.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    i2c.I2C_ClockSpeed = 100000;
    i2c.I2C_Mode = I2C_Mode_I2C;
    i2c.I2C_DutyCycle = I2C_DutyCycle_2;
    i2c.I2C_OwnAddress1 = 0x00;
    I2C_Init(I2C1, &i2c);
    I2C_Cmd(I2C1, ENABLE);

    // 等待 LCD 电源稳定，并将 E、R/W、RS 拉低。
    lcd_delay_us(50000U);
    lcd_i2c_start();
    lcd_write_expander(LCD_BACKLIGHT);
    I2C_GenerateSTOP(I2C1, ENABLE);

    // HD44780 上电同步：三次 0x3，再用 0x2 切换到四位接口。
    lcd_write_init_nibble(0x30U);
    lcd_delay_us(5000U);
    lcd_write_init_nibble(0x30U);
    lcd_delay_us(5000U);
    lcd_write_init_nibble(0x30U);
    lcd_delay_us(150U);
    lcd_write_init_nibble(0x20U);
    lcd_delay_us(150U);

    lcd_write_command(0x28U); // 四位接口、两行、5x8 字符
    lcd_write_command(0x08U); // 暂时关闭显示
    lcd_write_command(0x01U); // 清屏，光标回到首行首列
    lcd_write_command(0x06U); // 写入后光标向右移动，画面不滚动
    lcd_write_command(0x0CU); // 开启显示，关闭光标和闪烁
    lcd_write_command(0x80U); // 首行首列 DDRAM 地址
}

void lcd_write_char(unsigned char character)
{
    lcd_write_byte(character, LCD_RS);
}

void lcd_send(const uint8_t line,const uint8_t col,const char* data)
{
    if (data == 0)
    {
        return;
    }

    
    if(line == 0)
    {
        lcd_write_command(0x80U + col);
    }
    else if(line == 1)
    {
        lcd_write_command(0xC0U + col);
    }
    lcd_i2c_start();
    while (*data != '\0')
    {
        uint8_t i=(unsigned char)*data++;

        lcd_write_nibble(i, LCD_RS);
        lcd_write_nibble((uint8_t)(i << 4), LCD_RS);
    }
    
    I2C_GenerateSTOP(I2C1, ENABLE);
}
