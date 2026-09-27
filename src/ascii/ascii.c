#include <avr/io.h>
#define F_CPU 8000000UL
#include <util/delay.h>
#include "usart0.h"
#include "usart1.h"


const uint16_t FRAME_WIDTH = 320;
const uint16_t FRAME_HEIGHT = 180;

const uint16_t NUM_COLS_PC = 160;
const uint16_t NUM_ROWS_PC = 48;

const uint16_t NUM_COLS_TERMINAL = 80;
const uint16_t NUM_ROWS_TERMINAL = 24;

uint16_t num_cols;
uint16_t num_rows;


const char ascii_chars[] = "@%#*+=-:.     ";
                            

enum Commands {
    TEST = 1,
    SET_OUTPUT,
    SET_MODE,
    SET_WIDTH,
    SET_HEIGHT,
    SET_NUM_COLS,
    SET_NUM_ROWS,
    CAPTURE_FRAME,
    ACK = 0xFE,
    RESET = 0xFF,
};

enum Output {
    PC,
    TERMINAL,
};

enum Mode {
    PIC,
    CAM,
};


uint8_t output = PC;
uint8_t mode = PIC;


void send_byte(uint8_t byte) {
	while(!(UCSR0A & 0x20));
    
	UDR0 = byte;
}

uint8_t receive_byte() {
	while(!(UCSR0A & 0x80));

	return UDR0;
}

void test_camera() {
    uint8_t temp = 0;

    while(1) {
        send_byte(TEST);
        send_byte(0x36);
        temp = receive_byte();

        if (temp != 0x36){
            _delay_ms(1000);
        } else {
            return;
        }
    }
}

uint8_t check_keys() {
    if (!(PIND & (1 << PORT6))) {
        output = (output + 1) % 2;
        
        if (output == PC) {
            num_cols = NUM_COLS_PC;
            num_rows = NUM_ROWS_PC;
        } else if (output == TERMINAL) {
            num_cols = NUM_COLS_TERMINAL;
            num_rows = NUM_ROWS_TERMINAL;
        }
        return 1;
    }

    if (!(PIND & (1 << PORT7))) {
        mode = (mode + 1) % 2;
        return 1;
    }
    
    return 0;
}

void print_pixel() {
    uint32_t brightness = (uint32_t)receive_byte() * (uint32_t)(sizeof(ascii_chars) / sizeof(ascii_chars[0]) - 1) / (uint32_t)255;

    if (output == PC) {
        send_byte(ascii_chars[brightness]);
    } else if (output == TERMINAL) {
        send_byte(ACK);
        usart1_put(ascii_chars[brightness]);
    }
}

uint8_t print_frame() {
    send_byte(CAPTURE_FRAME);

    for (uint16_t y = 0; y < num_rows; y++) {
        for (uint16_t x = num_cols; x != 0; x--) {
            if (check_keys()) {
                return 0;
            }
            print_pixel();
        }

        if (output == TERMINAL && y < num_rows - 1) {
            usart1_put('\r');
            usart1_put('\n');
        }
    }
    
    return 1;
}

void start_streaming() {    
    while (1) {
        if (output == TERMINAL) {
            printf("\033[H");
        }
        
        if (!print_frame()) {
            return;
        }
    }
}

int main(void) {
    usart0_init();
    usart1_init();
    num_cols = NUM_COLS_PC;
    num_rows = NUM_ROWS_PC;
    
    while(1) {
        send_byte(RESET);
        test_camera();
        
        send_byte(SET_OUTPUT);
        send_byte(output);
        
        send_byte(SET_MODE);
        send_byte(mode);

        send_byte(SET_WIDTH);
        send_byte(FRAME_WIDTH >> 8);
        send_byte(FRAME_WIDTH & 0xFF);

        send_byte(SET_HEIGHT);
        send_byte(FRAME_HEIGHT >> 8);
        send_byte(FRAME_HEIGHT & 0xFF);
        
        send_byte(SET_NUM_COLS);
        send_byte(num_cols >> 8);
        send_byte(num_cols & 0xFF);
        
        send_byte(SET_NUM_ROWS);
        send_byte(num_rows >> 8);
        send_byte(num_rows & 0xFF);

        start_streaming();
    }
    
    return 0;
}