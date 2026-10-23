#ifndef BSP_USART2_H
#define BSP_USART2_H

#include "stm32f10x.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define USART2_RX_BUFFER_SIZE 256U

void usart2_init(void);
void send_data(uint8_t *data, size_t len);

char *get_received_line(void);

#endif /* BSP_USART2_H */
