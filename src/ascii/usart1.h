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

#ifndef USART_H_1
#define USART_H_1

#include <stdio.h>

/** Initialize the USART1.
 *
 *  Note: This also binds stdout and stdin to the USART1, so that stdio functions can be used.
 *        For example, printf() will print to the USART1.
 */
void usart1_init(void);

/** Send a byte through the USART1. */
void usart1_put(unsigned char c);

/** Receive a byte through the USART1.
 *
 *  @return The byte read as an unsigned char cast to an int, _FDEV_ERR on error,
            or _FDEV_EOF is no byte is available to be read.
 */
int usart1_get(void);

/** Receive a byte through the USART1, blockingly.
 *
 *  Blocks until a byte has been read or an error occured.
 *
 *  @return The byte read as an unsigned char cast to an int or _FDEV_ERR on error.
 */
int usart1_get_wait(void);

/** Enable echo mode: All read characters will be send back through the USART1. */
void usart1_enable_echo(void);

/** Disable echo mode (default). */
void usart1_disable_echo(void);

#endif /* USART_H_1*/
