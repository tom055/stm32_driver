#include "dog.h"

void dog_init(void)
{
    // 初始化 delay_ms() 使用的 Cortex-M3 周期计数器。
    SystemCoreClockUpdate();
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    // 先开启看门狗使用的 LSI 时钟
    RCC_LSICmd(ENABLE);
    while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET);

    // 允许修改分频和重装值
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
    IWDG_SetPrescaler(IWDG_Prescaler_64);
    IWDG_SetReload(2499);

    // 等待配置更新完成
    while (IWDG_GetFlagStatus(IWDG_FLAG_PVU) != RESET);
    while (IWDG_GetFlagStatus(IWDG_FLAG_RVU) != RESET);

    IWDG_ReloadCounter();
    IWDG_Enable();
}

uint32_t ResetReason_Check(void)
{
    uint32_t reason = 0;

    // 必须在清除复位标志之前读取
    if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET)
        reason |= RESET_REASON_IWDG;

    if (RCC_GetFlagStatus(RCC_FLAG_WWDGRST) != RESET)
        reason |= RESET_REASON_WWDG;

    if (RCC_GetFlagStatus(RCC_FLAG_SFTRST) != RESET)
        reason |= RESET_REASON_SOFT;

    if (RCC_GetFlagStatus(RCC_FLAG_PORRST) != RESET)
        reason |= RESET_REASON_POWER;

    // 读取完毕后清除复位标志
    RCC_ClearFlag();

    return reason;
}

bool System_IsHealthy(void)
{
    // Check if the system is healthy
    // This is a placeholder for actual health check logic
    return true; // 假设系统总是健康的
}
