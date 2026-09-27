/* spi - Reading from / writing to the SPI
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

#ifndef SPI_H_
#define SPI_H_

#include "types.h"

#define SPI_DDR DDRB
#define CS      PINB4
#define MOSI    PINB5
#define MISO    PINB6
#define SCK     PINB7



//spi.c spi.h
void SPI_init(void);
void SPI_start_transmit();
void SPI_end_transmit();
void SPI_send_byte(uint8_t data);
void SPI_send(uint8_t *data, uint8_t length);
uint8_t SPI_receive_byte(void);
void SPI_receive(uint8_t *data, uint8_t length);



#endif /* SPI_H_ */
