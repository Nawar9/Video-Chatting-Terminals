/* usart0 - Using the USART0 on the OpenM128 board
 *
 * Written in 2013 by Sven Hesse <drmccoy@drmccoy.de>.
 *
 * To the extent possible under law, the author(s) have dedicated all copyright and related and
 * neighboring rights to this software to the public domain worldwide.
 * This software is distributed without any warranty.
 *
 * You should have received a copy of the CC0 Public Domain Dedication along with this software.
 * If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
 */

#include <avr/io.h>

#include "usart0.h"

static uint8_t usart0_echo_enabled = 0;

void usart0_put(unsigned char c) {
	while(!(UCSR0A & 0x20));
	UDR0 = c;
}

int usart0_get(void) {
	if(!(UCSR0A & 0x80))
		return _FDEV_EOF;

	uint8_t status = UCSR0A;
	uint8_t data   = UDR0;

	if (status & 0x1C)
		return _FDEV_ERR;

	if (usart0_echo_enabled) {
		usart0_put(data);
		if (data == '\r')
			usart0_put('\n');
	}

	return data;
}

int usart0_get_wait(void) {
	int c;
	while ((c = usart0_get()) == _FDEV_EOF);

	return c;
}


static int uart0_putchar(char c, FILE *stream) {
	if (c == '\n')
		uart0_putchar('\r', stream);

	usart0_put(c);
	return 0;
}

static int uart0_getchar(FILE *stream) {
	int data = _FDEV_EOF;
	while (data == _FDEV_EOF)
		data = usart0_get();

	if (data == '\r')
		data = '\n';

	return data;
}

static FILE uart0_stdout_stdin = FDEV_SETUP_STREAM(uart0_putchar, uart0_getchar, _FDEV_SETUP_RW);

void usart0_init(void) {
	// USART0 initialization
	// Communication Parameters: 8 Data, 1 Stop, No Parity
	// USART0 Receiver: On
	// USART0 Transmitter: On
	// USART0 Mode: Asynchronous
	// USART0 Baud Rate: 9600
	UCSR0A = 0x02;
	UCSR0B = 0x98;
	UCSR0C = 0x06;
	UBRR0H = 0x00;
	UBRR0L = 0x00;

	stdout = &uart0_stdout_stdin;
	stdin  = &uart0_stdout_stdin;
}

void usart0_enable_echo(void) {
	usart0_echo_enabled = 1;
}

void usart0_disable_echo(void) {
	usart0_echo_enabled = 0;
}
