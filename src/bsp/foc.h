#ifndef FOC_H
#define FOC_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "stm32f10x.h"

#define PWM_ARR 3599      // 72MHz / (3599+1) = 20kHz
#define PWM_PSC 0
#define DEAD_TIME 72      // 约 1us 死区时间 (72 / 72MHz)

// TIM2 换相定时参数，假设定时器时钟为 72MHz
#define FOC_STEP_PSC 71U
#define FOC_STEP_ARR 999U
extern uint16_t duty_cycle; // 50% 占空比 (1800/3600)

// 六步换相函数，step 取值 0 ~ 5
void foc_init(void);
void foc_step(uint8_t  step);


#endif /* FOC_H */