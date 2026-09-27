/* usart1 - Using the USART1 on the OpenM128 board
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

#include "usart1.h"

static uint8_t usart1_echo_enabled = 0;

void usart1_put(unsigned char c) {
	while(!(UCSR1A & 0x20));
	UDR1 = c;
}

int usart1_get(void) {
	if(!(UCSR1A & 0x80))
		return _FDEV_EOF;

	uint8_t status = UCSR1A;
	uint8_t data   = UDR1;

	if (status & 0x1C)
		return _FDEV_ERR;

	if (usart1_echo_enabled) {
		usart1_put(data);
		if (data == '\r')
			usart1_put('\n');
	}

	return data;
}

int usart1_get_wait(void) {
	int c;
	while ((c = usart1_get()) == _FDEV_EOF);

	return c;
}


static int uart1_putchar(char c, FILE *stream) {
	if (c == '\n')
		uart1_putchar('\r', stream);

	usart1_put(c);
	return 1;
}

static int uart1_getchar(FILE *stream) {
	int data = _FDEV_EOF;
	while (data == _FDEV_EOF)
		data = usart1_get();

	if (data == '\r')
		data = '\n';

	return data;
}

static FILE uart1_stdout_stdin = FDEV_SETUP_STREAM(uart1_putchar, uart1_getchar, _FDEV_SETUP_RW);

void usart1_init(void) {
	// USART1 initialization
	// Communication Parameters: 8 Data, 1 Stop, No Parity
	// USART1 Receiver: On
	// USART1 Transmitter: On
	// USART1 Mode: Asynchronous
	// USART1 Baud Rate: 9600
	UCSR1A = 0x00;
	UCSR1B = 0x98;
	UCSR1C = 0x06;
	UBRR1H = 0x00;
	UBRR1L = 0x19;

	stdout = &uart1_stdout_stdin;
	stdin  = &uart1_stdout_stdin;
}

void usart1_enable_echo(void) {
	usart1_echo_enabled = 1;
}

void usart1_disable_echo(void) {
	usart1_echo_enabled = 0;
}

