#ifndef _UART_H
#define _UART_H

#include "var.h"

#ifndef BAUDRATE
#error "BAUDRATE not set"
#endif

#ifdef DOUBLE_SPEED_MODE
#define UBRR_VALUE (((F_CPU) + 4UL * (BAUDRATE)) / (8UL * (BAUDRATE)) - 1UL)
#else
#define UBRR_VALUE (((F_CPU) + 8UL * (BAUDRATE)) / (16UL * (BAUDRATE)) - 1UL)
#endif

void uart_init(void);
uint8_t uart_rx(void);
uint8_t uart_tx(char);
uint32_t rx_uint32_t(void);
void tx_uint32_t(uint32_t);

#endif
