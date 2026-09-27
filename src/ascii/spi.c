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

#include <avr/io.h>
#include <avr/interrupt.h>

#include "spi.h"


void SPI_init(void){

    // set CS, MOSI and SCK to output
    SPI_DDR |= (1 << CS) | (1 << MOSI) | (1 << SCK);
    SPI_DDR &= ~(1 << MISO);
    // enable SPI, set as master, and clock to fosc/128
    SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR1) | (1 << SPR0);
    PORTB |= (1 << CS);
}


void SPI_send_byte(uint8_t data){
    // load data into register
    SPDR = data;

    // Wait for transmission complete
    while(!(SPSR & (1 << SPIF)));
    // return slave select to high
}

void SPI_send(uint8_t *data, uint8_t length){
    for(int i = 0; i < length ; i++){
        SPI_send_byte(data[i]);
    }
}

void SPI_start_transmit(){
    PORTB &= ~(1 << CS);
}

void SPI_end_transmit(){
    PORTB |= (1 << CS);
}



uint8_t SPI_receive_byte(void)
{
    
    // transmit dummy byte
    SPDR = 0;

    // Wait for reception complete
    while(!(SPSR & (1 << SPIF)));

    // return Data Register
    return SPDR;
    
}


void SPI_receive(uint8_t *data, uint8_t length){
    for(int i = 0; i < length ; i++){
        data[i] = SPI_receive_byte();
    }
}

