#ifndef DOG_H
#define DOG_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "stm32f10x.h"

#define RESET_REASON_IWDG  (1U << 0)
#define RESET_REASON_WWDG  (1U << 1)
#define RESET_REASON_SOFT  (1U << 2)
#define RESET_REASON_POWER (1U << 3)

void dog_init(void);
uint32_t ResetReason_Check(void);
bool System_IsHealthy(void);


#endif /* DOG_H */