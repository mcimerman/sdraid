#include <avr/io.h>

#include <stdio.h>

#include "uart.h"

static int putchar_(char, FILE *);

static FILE uart_output = FDEV_SETUP_STREAM(putchar_, NULL, _FDEV_SETUP_WRITE);

static int
putchar_(char c, FILE *f)
{
	(void)f;
	while (!(UCSR0A & (1 << UDRE0)))
		;

	UDR0 = c;

	return (c);
}

void
uart_init(void)
{
	UBRR0 = UBRR_VALUE;
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);

	stdout = &uart_output;
}

uint8_t
uart_rx(void)
{
	while (!(UCSR0A & (1 << RXC0)))
		;

	return (UDR0);
}

uint8_t
uart_tx(char c)
{
	while (!(UCSR0A & (1 << UDRE0)))
		;

	UDR0 = c;

	return (c);
}

uint32_t
rx_uint32_t(void)
{
	uint32_t retval;

	retval = (uint32_t)uart_rx();
	retval |= ((uint32_t)uart_rx() << 8);
	retval |= ((uint32_t)uart_rx() << 16);
	retval |= ((uint32_t)uart_rx() << 24);

	return (retval);
}

void
tx_uint32_t(uint32_t val)
{
	uart_tx(val & 0xff);
	uart_tx((val >> 8) & 0xff);
	uart_tx((val >> 16) & 0xff);
	uart_tx((val >> 24) & 0xff);
}
