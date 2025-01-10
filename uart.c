#include <avr/io.h>

#include <stdio.h>

#include "uart.h"

static int putchar_(char, FILE *);

static FILE uart_output = FDEV_SETUP_STREAM(putchar_, NULL, _FDEV_SETUP_WRITE);

void
uart_init(void)
{
	UBRR0 = UBRR_VALUE;
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);

	stdout = &uart_output;
}

char
uart_rx(void)
{
	while (!(UCSR0A & (1 << RXC0)))
		;

	return (UDR0);
}

static int
putchar_(char c, FILE *)
{
	while (!(UCSR0A & (1 << UDRE0)))
		;

	UDR0 = c;

	return (c);
}
