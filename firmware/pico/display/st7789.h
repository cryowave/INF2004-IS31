#ifndef OTTIE_ST7789_H
#define OTTIE_ST7789_H

#include <tk/tkernel.h>

#define ST7789_WIDTH   240
#define ST7789_HEIGHT  320

#define ST7789_BLACK   0x0000U
#define ST7789_RED     0xF800U
#define ST7789_GREEN   0x07E0U
#define ST7789_BLUE    0x001FU
#define ST7789_WHITE   0xFFFFU

ER st7789_init(void);
void st7789_fill(UH color);
void st7789_fill_rect(UH x, UH y, UH width, UH height, UH color);
void st7789_draw_char(UH x, UH y, char c, UH color, UH background, UB scale);
void st7789_draw_text(UH x, UH y, const char *text, UH color, UH background, UB scale);

#endif
