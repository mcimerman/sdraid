#ifndef _UART_H
#define _UART_H

#include "var.h"

#define BAUD 115200UL

#define UBRR_VALUE (((F_CPU) + 8UL * (BAUD)) / (16UL * (BAUD)) - 1UL)


void uart_init(void);
char uart_rx(void);

#endif
