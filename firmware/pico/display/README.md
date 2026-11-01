# Display Module

Owner: Lay How

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

For now the driver can:
- initialise the LCD
- fill the full screen with one colour
- fill a rectangle with one colour

Text and menu drawing will be added after this.
