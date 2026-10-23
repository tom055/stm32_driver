#include "usart2.h"

static uint8_t rx_buffer[USART2_RX_BUFFER_SIZE];
static uint8_t current_buffer[USART2_RX_BUFFER_SIZE];
static size_t rx_index;
static uint8_t previous_received;

void usart2_init(void)
{
    rx_index = 0U;
    previous_received = 0U;
    memset(rx_buffer, 0, sizeof(rx_buffer));
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitTypeDef gpio;
    USART_InitTypeDef usart;

    // 2. 配置PA2为复用推挽输出（TX）
    gpio.GPIO_Pin = GPIO_Pin_2;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    // 3. 配置PA3为浮空输入（RX）
    gpio.GPIO_Pin = GPIO_Pin_3;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);
    // 4. 配置USART2参数
    USART_StructInit(&usart);
    // 波特率115200
    usart.USART_BaudRate = 115200;
    // 8位数据
    usart.USART_WordLength = USART_WordLength_8b;
    // 1位停止位
    usart.USART_StopBits = USART_StopBits_1;
    // 不使用奇偶校验
    usart.USART_Parity = USART_Parity_No;
    // 不使用硬件流控
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    // 同时开启发送和接收
    usart.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    // 5. 初始化USART2
    USART_Init(USART2, &usart);
    // 6. 使能USART2
    USART_Cmd(USART2, ENABLE);


    NVIC_InitTypeDef nvic;
    nvic.NVIC_IRQChannel = USART2_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 2;
    nvic.NVIC_IRQChannelSubPriority = 0;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
    // 开启USART2接收非空中断
    USART_ITConfig(USART2,USART_IT_RXNE,ENABLE);
}


void send_data(uint8_t *data,size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        // 等待发送缓冲区空
        while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
        // 发送字符
        USART_SendData(USART2, (uint16_t)data[i]);
    }
}

void USART2_IRQHandler(void)
{
    // 检查是否是接收中断
    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
    {
        // 读取接收到的数据
        uint8_t received = (uint8_t)USART_ReceiveData(USART2);
        // 只有连续收到\r\n时，才清空缓冲区并重新开始接收。
        if (previous_received == '\r' && received == '\n')
        {
            
            memset(current_buffer, 0, sizeof(current_buffer));
            memcpy(current_buffer, rx_buffer, rx_index);

            memset(rx_buffer, 0, sizeof(rx_buffer));
            rx_index = 0U;
        }
        else if (rx_index < sizeof(rx_buffer) - 1U)
        {
            rx_buffer[rx_index++] = received;
            rx_buffer[rx_index] = '\0';
        }
        previous_received = received;
    }
}

char *get_received_line(void){
    return (char *)current_buffer;
}
