#ifndef FLASH_H
#define FLASH_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "flash_config.h"
#include "stm32f10x.h"

#define USER_FLASH_ADDR  0x0800FC00U

uint8_t Flash_WriteConfig(const FlashConfig *config);
bool Flash_ReadConfig(FlashConfig *config);

#endif /* FLASH_H */