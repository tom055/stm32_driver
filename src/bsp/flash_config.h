#ifndef __FLASH_CONFIG_H
#define __FLASH_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct
{
    uint32_t magic;       // 配置标识，占 4 字节
    uint16_t version;     // 配置版本，占 2 字节
    uint16_t threshold;   // ADC 报警阈值，占 2 字节
    uint16_t crc;         // CRC16 校验值，占 2 字节
} FlashConfig;

#endif /* __FLASH_CONFIG_H */