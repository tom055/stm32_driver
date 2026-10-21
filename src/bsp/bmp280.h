#ifndef BSP_BMP280_H
#define BSP_BMP280_H

#include "stm32f10x.h"

/* BMP280 I2C 地址 */
#define BMP280_ADDRESS 0x76

/* BMP280 寄存器地址 */
#define BMP280_CHIP_ID     0xD0
#define BMP280_RESET       0xE0
#define BMP280_STATUS      0xF3
#define BMP280_CTRL_MEAS   0xF4
#define BMP280_CONFIG      0xF5
#define BMP280_PRESS_MSB   0xF7
#define BMP280_PRESS_LSB   0xF8
#define BMP280_PRESS_XLSB  0xF9
#define BMP280_TEMP_MSB    0xFA
#define BMP280_TEMP_LSB    0xFB
#define BMP280_TEMP_XLSB   0xFC

/* 寄存器写入：先调用 bmp280_init()。 */
void bmp280_write_register(uint8_t reg, uint8_t value);

/* 初始化 BMP280 */
void bmp280_init(void);

/* 读取 0xF7~0xF9，返回未补偿的 20 位气压 ADC 原始值（不是 Pa）。 */
int32_t bmp280_read_pressure(void);

/* 读取 0xFA~0xFC，返回未补偿的 20 位温度 ADC 原始值（不是摄氏度）。 */
int32_t bmp280_read_temperature(void);

#endif /* BSP_BMP280_H */
