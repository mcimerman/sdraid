#ifndef _UART_H
#define _UART_H

#include "var.h"

#define BAUD 9600UL

#define UBRR_VALUE (((F_CPU) + 8UL * (BAUD)) / (16UL * (BAUD)) - 1UL)


void uart_init(void);
uint8_t uart_rx(void);
uint8_t uart_tx(char);
uint32_t rx_uint32_t(void);
void tx_uint32_t(uint32_t);

#endif
