#ifndef MFRC522_H
#define MFRC522_H

#include "stm32f10x.h"
#include <stdint.h>
#include <stddef.h>

#define MFRC522_REG_FIFO     0x09U

void mfrc522_init(void);

// 调用前先执行 mfrc522_init()；这两个方法只访问 FIFODataReg。
// 多字节操作时寄存器地址保持不变，数据依次写入或读出 FIFO。
void mfrc522_write(const uint8_t *data, size_t len);
void mfrc522_read(uint8_t *buf, size_t len);

#endif /* MFRC522_H */
