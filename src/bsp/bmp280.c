#include "bmp280.h"


void bmp280_write_register(uint8_t reg, uint8_t value){
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) == SET);
    I2C_GenerateSTART(I2C1, ENABLE);
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_SB) == RESET);
    I2C_Send7bitAddress(I2C1, BMP280_ADDRESS << 1, I2C_Direction_Transmitter);
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_ADDR) == RESET);
    (void)*(__IO uint32_t *)&I2C1->SR1;
    (void)*(__IO uint32_t *)&I2C1->SR2;
    I2C_SendData(I2C1, reg);
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BTF) == RESET);

    I2C_SendData(I2C1, value);
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BTF) == RESET);

    I2C_GenerateSTOP(I2C1, ENABLE);
}

int32_t bmp280_read_pressure(void)
{
    uint32_t msb;
    uint32_t lsb;
    uint32_t xsb;
    uint32_t saved_primask;

    // 1. 等待总线空闲，开启 ACK，使用正常的 NACK 位置（POS=0）。
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) == SET);
    I2C_NACKPositionConfig(I2C1, I2C_NACKPosition_Current);
    I2C_AcknowledgeConfig(I2C1, ENABLE);

    // 2. 以写方向发送设备地址，再指定气压数据起始寄存器 0xF7。
    I2C_GenerateSTART(I2C1, ENABLE);
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_SB) == RESET);
    I2C_Send7bitAddress(I2C1, BMP280_ADDRESS << 1, I2C_Direction_Transmitter);
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_ADDR) == RESET);
    (void)*(__IO uint32_t *)&I2C1->SR1;
    (void)*(__IO uint32_t *)&I2C1->SR2;

    I2C_SendData(I2C1, BMP280_PRESS_MSB);
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BTF) == RESET);

    // 3. 重复 START，切换为读方向，接收连续的三个字节。
    I2C_GenerateSTART(I2C1, ENABLE);
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_SB) == RESET);
    I2C_Send7bitAddress(I2C1, BMP280_ADDRESS << 1, I2C_Direction_Receiver);
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_ADDR) == RESET);

    (void)*(__IO uint32_t *)&I2C1->SR1;
    (void)*(__IO uint32_t *)&I2C1->SR2;

    // 4. 前两个字节到齐（BTF=1）后，提前为第三个字节准备 NACK。
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BTF) == RESET);
    saved_primask = __get_PRIMASK();
    __disable_irq();
    I2C_AcknowledgeConfig(I2C1, DISABLE);
    msb = I2C_ReceiveData(I2C1);
    I2C_GenerateSTOP(I2C1, ENABLE); // 请求在最后一个字节结束后停止。
    lsb = I2C_ReceiveData(I2C1);
    __set_PRIMASK(saved_primask);

    // 5. 等第三个字节到达并取走，不再等待或丢弃第四个字节。
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) == RESET);
    xsb = I2C_ReceiveData(I2C1);
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) == SET);
    I2C_AcknowledgeConfig(I2C1, ENABLE);

    // 三个字节的高 20 位是原始气压 ADC 值，尚未补偿，单位不是 Pa。
    return (int32_t)(((msb << 16) | (lsb << 8) | xsb) >> 4);
}

int32_t bmp280_read_temperature(void)
{
    uint32_t msb;
    uint32_t lsb;
    uint32_t xsb;
    uint32_t saved_primask;

    // 1. 等待总线空闲，开启 ACK，使用正常的 NACK 位置（POS=0）。
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) == SET);
    I2C_NACKPositionConfig(I2C1, I2C_NACKPosition_Current);
    I2C_AcknowledgeConfig(I2C1, ENABLE);

    // 2. 以写方向发送设备地址，再指定温度数据起始寄存器 0xFA。
    I2C_GenerateSTART(I2C1, ENABLE);
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_SB) == RESET);
    I2C_Send7bitAddress(I2C1, BMP280_ADDRESS << 1, I2C_Direction_Transmitter);
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_ADDR) == RESET);
    (void)*(__IO uint32_t *)&I2C1->SR1;
    (void)*(__IO uint32_t *)&I2C1->SR2;
    I2C_SendData(I2C1, BMP280_TEMP_MSB);
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BTF) == RESET);

    // 3. 重复 START，切换为读方向，接收连续的三个字节。
    I2C_GenerateSTART(I2C1, ENABLE);
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_SB) == RESET);
    I2C_Send7bitAddress(I2C1, BMP280_ADDRESS << 1, I2C_Direction_Receiver);
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_ADDR) == RESET);
    (void)*(__IO uint32_t *)&I2C1->SR1;
    (void)*(__IO uint32_t *)&I2C1->SR2;

    // 4. 前两个字节到齐（BTF=1）后，提前为第三个字节准备 NACK。
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BTF) == RESET);
    saved_primask = __get_PRIMASK();
    __disable_irq();
    I2C_AcknowledgeConfig(I2C1, DISABLE);
    msb = I2C_ReceiveData(I2C1);
    I2C_GenerateSTOP(I2C1, ENABLE); // 请求在最后一个字节结束后停止。
    lsb = I2C_ReceiveData(I2C1);
    __set_PRIMASK(saved_primask);

    // 5. 等第三个字节到达并取走，不再等待或丢弃第四个字节。
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) == RESET);
    xsb = I2C_ReceiveData(I2C1);
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) == SET);
    I2C_AcknowledgeConfig(I2C1, ENABLE);

    // 三个字节的高 20 位是原始温度 ADC 值，尚未补偿，单位不是摄氏度。
    return (int32_t)(((msb << 16) | (lsb << 8) | xsb) >> 4);
}


void bmp280_init(void){
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
}
