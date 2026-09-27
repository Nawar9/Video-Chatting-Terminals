# Video-Chatting Terminals

**Video-Chatting Terminals** is an embedded systems project that turns a live camera feed into a real-time ASCII art video stream, rendered directly in your terminal, no graphical display required.

A PC captures video from a V4L2 webcam, downsamples each frame to terminal resolution, and streams the brightness values over a 1 Mbaud serial link to an ATmega1284P microcontroller, which forwards the characters to a terminal (or back to the PC) at up to 160×48 characters. A standalone PC-only version renders ASCII video directly and adapts to any terminal size.

## Demo

A single frame as rendered by the system:

```
                                        .,+:;c!0?bc;bccc;,-,.
                                    -?23543444444544445554443320:,
                                .+122110011111123221212233332110?0140-
                            .,b2410b:::;+:;;;;b!b;;;cbbba!!a;;,...,+a010,
                          :a2442b+-,....,..,,,=+=-,,,,---=,,.       .-c?10,
                          1332!;-,............,,............          .-a01?.
                        ,3442!+,..                                       =!10:
                        2552c.                                            .b01b
                       ?440;,.....                                         .b01:
                      ,341a+,......                                         -!11.
                      253?b:-.......                                        -b01c
                     .4540b=,..........                                     ,c?20
                     -5550;,.,,+caaaa012221!b:-.      .,=b!?1222110?aa;=.    ,a21
                     ,466b..+021?!?033444430ac::,.. ..,+ab!1111?!ab:==++:;.   b20
                      267:.,-...,-=+:;ccc:++,,,,,..    .,-+cbb!!ab;+,..       :3!
                     .+28-.. ..=b?a;!885$!:a!bb;=.     .=;ba:=5735c -b;-      :3+
                     -,,5,  ..,=;:+-,=;a;+cbbbbc=.     ..,+;;+-==,.....       :b
                      ,.2. ...,,,,,,,,,--===+++=,.         .......            :.
                      .-?, ..............,,,,,-,,,                            c.
                       .:c   ...............,,,.                             .,
                        ,1,.      ..........,,.                              +
                         -;,.     .      ...  .,-      =;.                  ..
                          .-...  . . ......,--+;c,.    .,.,.               .=
                           :,,...  ..,-+::;;b?230a;-,=ca!!c:=,,-,.        .-
                           .=,-,,..=cba!?00?111?a;caa?!;;bb!??01?!a;.    ,+-
                            ,=;:+,,=:cba!!a:=-......,,......,=baaac;,...-+=
                             :c!a:=-==-----+::::=--,,,,,,...   .,++-,..:;+;
                              210b;::+-,..........,,-+:-,.       .,+:::ba.
                              =54??0?b;:-....   .,+;?!;=,.     .,+??!?0!
                              ,+27621100!b=,....,+;+++=,,,....+:b?0232;
                              ,,-c268532220ab:::::::=+-++:::bcb!?134!.
                             .,,,,-;!3995221?cbc=-:cba:=:+ba0?11251-
                           .,--,,,,,=;b!37887452011021?a123445551:.
                      .=c?:---=,,,,..,-=:b?02445577867754553221:.
                 .-:;baaac-,----,,,......,=:;cb!ccbabc!!ca!!b+,            b..
            .=;c;cccbbba!a=,----,,,...........,,-==-,,,-=+=,.              .c,......,.
       .,+cbbbcccccccbba!!;-----,,.....                                   ?-,,,..,..,-,.....
   .=;cc;:ccbabbcccbbcbb!!b=----,,,.....                                 aa,,,,.,,,..--,....,,....
 cccccccbccbbbbbbccbaccaa!a;=,,-,,,......                               =1-,,,..,--..,--,...,-,,,,,.
 !??!!bbccbbbbbbbcbbbbbbbbaa;-,,,,,......                              +0c,,,,..,,,...,,,. .,=+,,,,,.
 ;ca!!abbbbccccbbcbbbbcccccbc+,,.,,.......                           .-a0=,,-,....,-..,,,....=+=-,,,,
```

## How It Works

```
 USB Webcam ──V4L2/YUYV──▶ PC ──USART @ 1 Mbaud──▶ ATmega1284P ──USART1──▶ Terminal
(320×180)              capture + downsample      (OpenM128)                (80×24)
```

1. **Capture:** The PC application grabs frames from `/dev/video0` using V4L2 memory-mapped buffers in YUYV format (320×180). A static JPEG image can be used instead for testing without a camera.
2. **Downsample:** Each frame is divided into a grid of cells (160×48 for PC output, 80×24 for terminal output). The average brightness of each cell is mapped to a character from an ASCII brightness ramp (`@%#*+=-:. `).
3. **Stream:** Brightness bytes are sent to the microcontroller over USART at 1,000,000 baud using a small command protocol (`SET_WIDTH`, `SET_HEIGHT`, `SET_NUM_COLS`, `SET_NUM_ROWS`, `CAPTURE_FRAME`, `ACK`, `RESET`, …).
4. **Display:** The ATmega1284P firmware receives each character and forwards it either back to the PC (which prints a 160×48 grid) or to a terminal over USART1 (80×24 grid), using ANSI escape codes to keep the image in place.

## Features

- Real-time ASCII rendering of a live webcam feed
- Live camera mode and static-image (JPEG) mode
- Two output targets: PC terminal (160×48) and microcontroller-driven terminal (80×24)
- Custom serial command protocol with handshake (`ACK`) and reset (`RESET`) handling
- ATmega1284P firmware with dual USARTs: USART0 talks to the PC, USART1 drives the terminal
- Standalone PC-only version that adapts to the actual terminal size at runtime

## Hardware

- **ATmega1284P:** on the OpenM128 board. Runs the display firmware
- **FT2232HL:** programming and debug interface (OpenOCD)
- **V4L2-compatible USB webcam:** video source for the PC-side application
- **USB-to-serial adapter:** 1 Mbaud link between PC and microcontroller

## Software

| Component | Location | Description |
|---|---|---|
| PC application | `src/ascii_pc_side/` | Linux app: V4L2 capture, JPEG decoding (libjpeg), downsampling, serial streaming, threaded circular buffer |
| MCU firmware | `src/ascii/` | ATmega1284P firmware: command protocol, dual-USART forwarding, SPI driver |
| Standalone PC version | `src/ascii_pc_only/` | Renders ASCII video directly to the terminal, no microcontroller required |

## Getting Started

### PC application (Linux)

Requires `libjpeg` and Linux V4L2 headers:

```bash
gcc src/ascii_pc_side/ascii_pc_side.c -o ascii_pc_side -ljpeg -lpthread
./ascii_pc_side
```

The application expects the camera at `/dev/video0`, the serial device at `/dev/ttyUSB0` (or `/dev/ttyUSB1`), and a `pic.jpg` in the working directory (loaded at startup, so it is required even in camera mode).

### Standalone PC version

```bash
gcc src/ascii_pc_only/ascii_video.c -o ascii_video
./ascii_video
```

### Microcontroller firmware

The firmware is an MPLAB X / NetBeans project (`src/ascii/nbproject`). After building, flash it with OpenOCD:

```bash
openocd -f src/ascii/openocd/FT2232HL.cfg -f src/ascii/openocd/atmega1284p.cfg -c "program ascii.elf reset exit"
```

To free the UART pins for normal operation, the FT2232HL can be deactivated afterwards:

```bash
openocd -f src/ascii/openocd/FT2232HL_deactivate.cfg -f src/ascii/openocd/atmega1284p.cfg -c "program src/ascii/openocd/Sample.elf reset exit"
```

## Project Structure

```
.
├── README.md
├── resources/
│   └── ASCII Cam - Presentation.pdf
└── src/
    ├── ascii/                  # ATmega1284P firmware
    │   ├── ascii.c             # main loop, command protocol, display forwarding
    │   ├── types.h             # shared type definitions
    │   ├── usart0.c / .h       # USART0 driver (PC link)
    │   ├── usart1.c / .h       # USART1 driver (terminal)
    │   ├── spi.c / .h          # SPI driver (camera module)
    │   ├── openocd/            # OpenOCD configs for the FT2232HL
    │   └── nbproject/          # MPLAB X project files
    ├── ascii_pc_side/          # PC application (webcam → microcontroller)
    │   └── ascii_pc_side.c
    └── ascii_pc_only/          # Standalone PC application
        └── ascii_video.c
```
