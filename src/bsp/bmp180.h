#ifndef BSP_BMP180_H
#define BSP_BMP180_H

#include "stm32f10x.h"

#define BMP180_ADDRESS 0x77U

/* 初始化 I2C1，检查 ID 并读取出厂校准值；成功返回 1。 */
int bmp180_init(void);

/* 触发温度、气压测量并补偿。输出 0.1 摄氏度和 Pa；失败返回 0。 */
int bmp180_read(int32_t *temperature_deci_c, uint32_t *pressure_pa);

#endif
