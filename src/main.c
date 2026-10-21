#include <stdio.h>
#include <string.h>
#include "stm32f10x.h"
#include "bsp/lcd.h"
#include "bsp/bmp180.h"

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

int main(void)
{
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
