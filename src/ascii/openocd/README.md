# Openocd

Openocd can be used to program the Atmega via the FT2232. You have to deactivate it, to use different programming over UART.


## Programming 

To program the Atmega1284P use the following code in your shell. Openocd has to be installed:
<code>		 openocd -f FT2232HL.cfg -f atmega1284p.cfg -c "program FILENAME.elf reset exit" </code>

### MPLAB

In MPLAB is an Configuration named "openocd", which executes this script after build. 

### Deactivate FT2232HL

to deactivate the FT2232 use: 
<code>openocd -f FT2232HL_deactivate.cfg -f atmega1284p.cfg -c "program Sample.elf reset exit"</code>
