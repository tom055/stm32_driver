#include "foc.h"

uint16_t duty_cycle = 1800;

void foc_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;
    TIM_BDTRInitTypeDef TIM_BDTRInitStructure;

    // 1. 开启时钟 (只开 GPIOA, GPIOB, TIM1，不需要 AFIO)
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_TIM1, ENABLE);

    // 2. 配置 GPIO 为复用推挽输出
    // PA8, PA9, PA10 (U, V, W 上桥臂)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // PB13, PB14, PB15 (U, V, W 下桥臂)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // 3. 配置 TIM1 时基
    TIM_TimeBaseStructure.TIM_Period = PWM_ARR;
    TIM_TimeBaseStructure.TIM_Prescaler = PWM_PSC;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0; // 高级定时器特有
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

    // 4. 配置 TIM1 通道 (PWM模式1，主通道和互补通道都使能)
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0; // 初始占空比为0
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High; // 互补通道高电平有效
    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset; 
    TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;

    TIM_OC1Init(TIM1, &TIM_OCInitStructure); // U相
    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_OC2Init(TIM1, &TIM_OCInitStructure); // V相
    TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_OC3Init(TIM1, &TIM_OCInitStructure); // W相
    TIM_OC3PreloadConfig(TIM1, TIM_OCPreload_Enable);

    // 5. 配置死区与刹车 (关键)
    TIM_BDTRInitStructure.TIM_OSSRState = TIM_OSSRState_Enable;
    TIM_BDTRInitStructure.TIM_OSSIState = TIM_OSSIState_Enable;
    TIM_BDTRInitStructure.TIM_LOCKLevel = TIM_LOCKLevel_1;
    TIM_BDTRInitStructure.TIM_DeadTime = DEAD_TIME; 
    TIM_BDTRInitStructure.TIM_Break = TIM_Break_Disable;
    TIM_BDTRInitStructure.TIM_BreakPolarity = TIM_BreakPolarity_High;
    TIM_BDTRInitStructure.TIM_AutomaticOutput = TIM_AutomaticOutput_Enable;
    TIM_BDTRConfig(TIM1, &TIM_BDTRInitStructure);


    //TIM2定时
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    TIM_TimeBaseStructure.TIM_Period = FOC_STEP_ARR;
    TIM_TimeBaseStructure.TIM_Prescaler = FOC_STEP_PSC;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM2, ENABLE);


    TIM1->CCER = 0; // 强制关闭所有通道输出
    TIM1->CCR1 = 0;
    TIM1->CCR2 = 0;
    TIM1->CCR3 = 0;
    // 6. 使能主输出(MOE)并启动定时器
    TIM_CtrlPWMOutputs(TIM1, ENABLE);//合闸
    TIM_Cmd(TIM1, ENABLE);

    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
}

void foc_step(uint8_t  step){
     uint16_t ccer_val = TIM1->CCER;

    // 第一步：先关闭所有 6 个通道的输出，防止换相瞬间直通短路
    ccer_val &= ~(TIM_CCER_CC1E | TIM_CCER_CC1NE | 
                  TIM_CCER_CC2E | TIM_CCER_CC2NE | 
                  TIM_CCER_CC3E | TIM_CCER_CC3NE);
    TIM1->CCER = ccer_val;

    switch(step)
    {
        case 0: // 第1步: U+ V-，W悬空 (A+, B-)
            TIM1->CCR1 = duty_cycle; // U相上管PWM
            TIM1->CCR2 = 0;          // V相上管关，下管常开
            ccer_val |= (TIM_CCER_CC1E | TIM_CCER_CC1NE | TIM_CCER_CC2E | TIM_CCER_CC2NE);
            break;

        case 1: // 第2步: U+ W-，V悬空 (A+, C-)
            TIM1->CCR1 = duty_cycle;
            TIM1->CCR3 = 0;
            ccer_val |= (TIM_CCER_CC1E | TIM_CCER_CC1NE | TIM_CCER_CC3E | TIM_CCER_CC3NE);
            break;

        case 2: // 第3步: V+ W-，U悬空 (B+, C-)
            TIM1->CCR2 = duty_cycle;
            TIM1->CCR3 = 0;
            ccer_val |= (TIM_CCER_CC2E | TIM_CCER_CC2NE | TIM_CCER_CC3E | TIM_CCER_CC3NE);
            break;

        case 3: // 第4步: V+ U-，W悬空 (B+, A-)
            TIM1->CCR2 = duty_cycle;
            TIM1->CCR1 = 0;
            ccer_val |= (TIM_CCER_CC2E | TIM_CCER_CC2NE | TIM_CCER_CC1E | TIM_CCER_CC1NE);
            break;

        case 4: // 第5步: W+ U-，V悬空 (C+, A-)
            TIM1->CCR3 = duty_cycle;
            TIM1->CCR1 = 0;
            ccer_val |= (TIM_CCER_CC3E | TIM_CCER_CC3NE | TIM_CCER_CC1E | TIM_CCER_CC1NE);
            break;

        case 5: // 第6步: W+ V-，U悬空 (C+, B-)
            TIM1->CCR3 = duty_cycle;
            TIM1->CCR2 = 0;
            ccer_val |= (TIM_CCER_CC3E | TIM_CCER_CC3NE | TIM_CCER_CC2E | TIM_CCER_CC2NE);
            break;
    }
    TIM_GenerateEvent(TIM1, TIM_EventSource_Update);
    // 写入 CCER 寄存器，应用新的通道状态
    TIM1->CCER = ccer_val;
}

void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        // 清除中断标志
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);

        // 在这里执行定时器中断处理代码
        static uint8_t step = 0;
        foc_step(step);
        step = (step + 1) % 6; // 循环步进 0~5
    }
}