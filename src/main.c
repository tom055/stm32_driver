#include <stdio.h>
#include <string.h>
#include "stm32f10x.h"
#include "bsp/lcd.h"
#include "bsp/bmp180.h"
#include "bsp/mfrc522.h"
#include "bsp/usart2.h"
#include "bsp/dog.h"
#include "bsp/flash.h"
#include "bsp/flash_config.h"

/* lcd_init() 已启用 DWT；按 CPU 周期计数，避免空循环随优化变化。 */
static void delay_ms(uint32_t milliseconds)
{
    while (milliseconds-- != 0U)
    {
        uint32_t start = DWT->CYCCNT;
        while ((uint32_t)(DWT->CYCCNT - start) < SystemCoreClock / 1000U);
    }
}

/* 每次覆盖整行，数值变短时不会留下上次的字符。 */
static void display_line(uint8_t row, const char *text)
{
    char line[17];
    size_t length = strlen(text);
    if (length > 16U) length = 16U;
    memset(line, ' ', 16U);
    memcpy(line, text, length);
    line[16] = '\0';
    lcd_send(row, 0, line);
}
static void main_init_mfrc522(void)
{
    const uint8_t tx_data[] = {'A', '1', 'b'};
    uint8_t rx_data[sizeof(tx_data)];
    char rx_chars[sizeof(tx_data)];
    char text[32];

    lcd_init();
    mfrc522_init();
    display_line(0, "MFRC522 FIFO");
    display_line(1, "Starting...");
    delay_ms(50U);

    while (1)
    {
        /* 仅验证芯片内部 FIFO，不向 RFID 卡片写入数据。
         * 每次写入和读出相同数量的字节，正常情况下 FIFO 会重新变空。 */
        memset(rx_data, 0, sizeof(rx_data));
        mfrc522_write(tx_data, sizeof(tx_data));
        mfrc522_read(rx_data, sizeof(rx_data));
        int matched = memcmp(tx_data, rx_data, sizeof(tx_data)) == 0;

        /* 异常接收值用 '?' 显示，避免控制字符截断 LCD 上的状态文字。 */
        for (size_t i = 0; i < sizeof(rx_data); i++)
        {
            rx_chars[i] = (rx_data[i] >= 0x20U && rx_data[i] <= 0x7EU) ? (char)rx_data[i] : '?';
        }

        snprintf(text, sizeof(text), "TX:%c %c %c",
                 (int)tx_data[0], (int)tx_data[1], (int)tx_data[2]);
        display_line(0, text);
        snprintf(text, sizeof(text), "RX:%c %c %c %s",
                 (int)rx_chars[0], (int)rx_chars[1], (int)rx_chars[2],
                 matched ? "OK" : "FAIL");
        display_line(1, text);

        delay_ms(1000U);
    }
}

void main_init_bmp(){
    char text[32];
    int32_t temperature;
    uint32_t pressure;

    lcd_init();
    if (!bmp180_init())
    {
        display_line(0, "BMP180 error");
        display_line(1, "Check I2C 0x77");
        while (1);
    }
    while (1)
    {
        /* 驱动触发测量、等待转换并补偿，输出 0.1 摄氏度和 Pa。 */
        if (bmp180_read(&temperature, &pressure))
        {
            uint32_t magnitude = (uint32_t)(temperature < 0 ? -temperature : temperature);
            /* 不使用浮点 printf，也不把 UTF-8 的度数符号发送给 LCD。 */
            snprintf(text, sizeof(text), "Temp:%s%lu.%lu C",
                     temperature < 0 ? "-" : "", (unsigned long)(magnitude / 10U),
                     (unsigned long)(magnitude % 10U));
            display_line(0, text);
            snprintf(text, sizeof(text), "Pres:%lu.%02lu hPa",
                     (unsigned long)(pressure / 100U), (unsigned long)(pressure % 100U));
            display_line(1, text);
        }
        else
        {
            display_line(0, "BMP180 data err");
             display_line(1, "Check sensor");
        }
        delay_ms(1000U);
    }
}


void main_init_usart2(void)
{
    char text[32];
    lcd_init();
    usart2_init();
    display_line(0, "USART2 RX");
    display_line(1, "Waiting...");
    while (1)
    {
        char *line = get_received_line();
        if (line[0] != '\0')
        {
            snprintf(text, sizeof(text), "RX: %s", line);
            display_line(1, text);
        }
        delay_ms(100U);
    }
}

void main_dog(void){
    uint32_t reson=ResetReason_Check();
    dog_init();
    while(1){
        // 在这里执行其他任务
        // 定期喂狗，防止系统复位
         if (System_IsHealthy())
        {
            IWDG_ReloadCounter();
        }
        delay_ms(1000U); // 延时 1 秒
    }
}

void main_flash(void){
    lcd_init();
    FlashConfig config = {
        .magic = 0x12345678,
        .version = 1,
        .threshold = 100,
        .crc = 0xABCD
    };

    Flash_WriteConfig(&config);

    // 从 Flash 读取配置
    FlashConfig read_config={};
    Flash_ReadConfig(&read_config);
    char text[17];
    snprintf(text, sizeof(text), "CRC:%04X",(unsigned int)read_config.crc);
    display_line(0, text);
    while(1){
        delay_ms(1000U);
    }
}

int main(void)
{
    main_flash();
}
