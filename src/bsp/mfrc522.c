#include "mfrc522.h"

void mfrc522_init(void)
{
    //PA4: cs PA5: sck PA6: miso PA7: mosi
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_SPI1, ENABLE);
    GPIO_InitTypeDef gpio;

    // PA5、PA7：SPI复用推挽输出
    gpio.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    // PA6：SPI输入
    gpio.GPIO_Pin = GPIO_Pin_6;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);

    // PA4：软件控制CS
    gpio.GPIO_Pin = GPIO_Pin_4;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    GPIO_SetBits(GPIOA, GPIO_Pin_4);

    SPI_InitTypeDef spi;
    SPI_StructInit(&spi);

    spi.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    spi.SPI_Mode = SPI_Mode_Master;
    spi.SPI_DataSize = SPI_DataSize_8b;
    spi.SPI_CPOL = SPI_CPOL_Low;
    spi.SPI_CPHA = SPI_CPHA_1Edge;
    spi.SPI_NSS = SPI_NSS_Soft;
    spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
    spi.SPI_FirstBit = SPI_FirstBit_MSB;

    SPI_Init(SPI1, &spi);

    // 启动 SPI1
    SPI_Cmd(SPI1, ENABLE);

}

// SPI 全双工：发送一个字节，同时读取接收到的一个字节。
static uint8_t mfrc522_spi_transfer(uint8_t data)
{
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(SPI1, data);
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
    return (uint8_t)SPI_I2S_ReceiveData(SPI1);
}

void mfrc522_write(const uint8_t *data, size_t len)
{
    if (len == 0U || data == NULL)
    {
        return;
    }

    GPIO_ResetBits(GPIOA, GPIO_Pin_4);

    // 固定写 FIFODataReg：bit7=0，bit6:1 为寄存器地址，bit0=0。
    (void)mfrc522_spi_transfer((uint8_t)((MFRC522_REG_FIFO << 1U) & 0x7EU));
    for (size_t i = 0; i < len; i++)
    {
        // 写入时也要读走接收数据，避免接收溢出。
        (void)mfrc522_spi_transfer(data[i]);
    }

    // 等待最后一位传输结束，再释放片选。
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);
    GPIO_SetBits(GPIOA, GPIO_Pin_4);
}

void mfrc522_read(uint8_t *buf, size_t len)
{
    if (len == 0U || buf == NULL)
    {
        return;
    }

    // 固定读 FIFODataReg：bit7=1，其余地址位与写入时相同。
    uint8_t address = (uint8_t)(((MFRC522_REG_FIFO << 1U) & 0x7EU) | 0x80U);
    GPIO_ResetBits(GPIOA, GPIO_Pin_4);

    // 首个地址字节对应的接收值无效，丢弃。
    (void)mfrc522_spi_transfer(address);
    for (size_t i = 0; i < len; i++)
    {
        // 连续读取时发送下一次读地址；接收最后一个字节时发送0x00结束。
        uint8_t next = (i == len - 1U) ? 0x00U : address;
        buf[i] = mfrc522_spi_transfer(next);
    }

    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);
    GPIO_SetBits(GPIOA, GPIO_Pin_4);
}
