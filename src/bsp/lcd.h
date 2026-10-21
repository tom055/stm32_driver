#ifndef BSP_LCD_H
#define BSP_LCD_H

#include "stm32f10x.h"

/* 初始化 I2C1 和 LCD1602，完成后可直接发送字符。 */
void lcd_init(void);
/* 先调用 lcd_init()；本函数自行处理 I2C START/地址/STOP。 */
void lcd_write_char(unsigned char character);
void lcd_send(const uint8_t line,const uint8_t col,const char* data);

#endif /* BSP_LCD_H */
