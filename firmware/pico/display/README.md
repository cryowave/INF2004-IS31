# Display Module

Basic ST7789 LCD driver for the Pico.

Current files:
- `st7789.c`
- `st7789.h`

LCD pins:
- GP2: SCK
- GP3: MOSI
- GP26: CS
- GP7: DC
- GP6: RESET

Current support:
- initialise the LCD
- fill the full screen with one colour
- fill a rectangle with one colour
- draw 5x7 characters
- draw basic text strings

The text renderer currently supports uppercase letters and numbers.
Menu drawing will be added after this.
