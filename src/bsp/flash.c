#include "flash.h"

uint8_t Flash_WriteConfig(const FlashConfig *cfg)
{
    uint16_t words[5];

    words[0] = (uint16_t)(cfg->magic & 0xFFFF);
    words[1] = (uint16_t)(cfg->magic >> 16);
    words[2] = cfg->version;
    words[3] = cfg->threshold;
    words[4] = cfg->crc;

    FLASH_Unlock();
    // 清除上次操作的状态标志
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);

    FLASH_Status status = FLASH_ErasePage(USER_FLASH_ADDR);

    if (status != FLASH_COMPLETE)
    {
        FLASH_Lock();
        return 0;
    }
    // 6. 逐个写入16位半字
    for (uint16_t i = 0; i < 5; i++)
    {
        status = FLASH_ProgramHalfWord(USER_FLASH_ADDR + i * 2U,words[i]);
        if (status != FLASH_COMPLETE)
        {
            FLASH_Lock();
            return 0;
        }
    }
    FLASH_Lock();
    return 1;
}

bool Flash_ReadConfig(FlashConfig *config)
{
    const FlashConfig *cfg =(const FlashConfig *)USER_FLASH_ADDR;
    memcpy(config, cfg, sizeof(FlashConfig));
    return true; // Placeholder return value
}