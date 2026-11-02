# Display Module

Reusable ST7789 display module for the Pico.

Current files:
- `st7789.c`
- `st7789.h`
- `display_manager.c`
- `display_manager.h`

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
- run the LCD through a dedicated uT-Kernel Display Task
- receive display events through a uT-Kernel message buffer
- show the basic Ottie menu

Only the Display Task should directly control the LCD.
