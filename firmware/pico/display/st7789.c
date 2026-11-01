#include <tk/tkernel.h>
#include <tk/syslib.h>
#include <sys/sysdef.h>
#include <bsp/libbsp.h>

#include "st7789.h"

/* LCD pins we are using */
#define LCD_PIN_SCK    2U
#define LCD_PIN_MOSI   3U
#define LCD_PIN_RST    6U
#define LCD_PIN_DC     7U
#define LCD_PIN_CS     26U

/* SPI0 registers on the RP2040 */
#define SPI0_BASE      0x4003C000U
#define SSPCR0         (SPI0_BASE + 0x000U)
#define SSPCR1         (SPI0_BASE + 0x004U)
#define SSPDR          (SPI0_BASE + 0x008U)
#define SSPSR          (SPI0_BASE + 0x00CU)
#define SSPCPSR        (SPI0_BASE + 0x010U)

#define SSPSR_TNF      (1U << 1)
#define SSPSR_RNE      (1U << 2)
#define SSPSR_BSY      (1U << 4)
#define SSPCR1_SSE     (1U << 1)

/* Commands needed to start and write to the ST7789 */
#define CMD_SWRESET    0x01U
#define CMD_SLPOUT     0x11U
#define CMD_NORON      0x13U
#define CMD_INVON      0x21U
#define CMD_DISPON     0x29U
#define CMD_CASET      0x2AU
#define CMD_RASET      0x2BU
#define CMD_RAMWR      0x2CU
#define CMD_MADCTL     0x36U
#define CMD_COLMOD     0x3AU

static void spi0_reset(void)
{
    clr_w(RESETS_RESET, RESETS_RESET_SPI0);

    /* wait until SPI0 is ready */
    while ((in_w(RESETS_RESET_DONE) & RESETS_RESET_SPI0) == 0U) {
    }
}

static void spi0_init_lcd(void)
{
    spi0_reset();

    /* GP2 and GP3 are used for SPI0 */
    clr_w(GPIO(LCD_PIN_SCK), GPIO_OD);
    set_w(GPIO(LCD_PIN_SCK), GPIO_IE);
    out_w(GPIO_CTRL(LCD_PIN_SCK), GPIO_CTRL_FUNCSEL_SPI);

    clr_w(GPIO(LCD_PIN_MOSI), GPIO_OD);
    set_w(GPIO(LCD_PIN_MOSI), GPIO_IE);
    out_w(GPIO_CTRL(LCD_PIN_MOSI), GPIO_CTRL_FUNCSEL_SPI);

    out_w(SSPCR1, 0U);
    out_w(SSPCR0, 7U | (15U << 8));
    out_w(SSPCPSR, 2U);
    out_w(SSPCR1, SSPCR1_SSE);
}

static UB spi0_write_byte(UB value)
{
    while ((in_w(SSPSR) & SSPSR_TNF) == 0U) {
    }

    out_w(SSPDR, (UW)value);

    while ((in_w(SSPSR) & SSPSR_RNE) == 0U) {
    }

    return (UB)in_w(SSPDR);
}

static void spi0_wait(void)
{
    /* make sure the SPI transfer finished */
    while ((in_w(SSPSR) & SSPSR_BSY) != 0U) {
    }
}

static void lcd_select(BOOL selected)
{
    gpio_set_val(LCD_PIN_CS, selected ? 0U : 1U);
}

static void lcd_command(UB command)
{
    lcd_select(TRUE);
    gpio_set_val(LCD_PIN_DC, 0U);

    (void)spi0_write_byte(command);

    spi0_wait();
    lcd_select(FALSE);
}

static void lcd_data(const UB *data, INT length)
{
    INT i;

    lcd_select(TRUE);
    gpio_set_val(LCD_PIN_DC, 1U);

    for (i = 0; i < length; i++) {
        (void)spi0_write_byte(data[i]);
    }

    spi0_wait();
    lcd_select(FALSE);
}

static void lcd_u16(UH value)
{
    UB data[2];

    /* ST7789 expects the high byte first */
    data[0] = (UB)(value >> 8);
    data[1] = (UB)(value & 0xFFU);

    lcd_data(data, 2);
}

static void lcd_reset(void)
{
    gpio_set_val(LCD_PIN_RST, 0U);
    tk_dly_tsk(50);

    gpio_set_val(LCD_PIN_RST, 1U);
    tk_dly_tsk(120);
}

static void lcd_window(UH x0, UH y0, UH x1, UH y1)
{
    /* choose the part of the screen we want to write to */
    lcd_command(CMD_CASET);
    lcd_u16(x0);
    lcd_u16(x1);

    lcd_command(CMD_RASET);
    lcd_u16(y0);
    lcd_u16(y1);

    lcd_command(CMD_RAMWR);
}

ER st7789_init(void)
{
    UB value;

    if (gpio_set_pin(LCD_PIN_CS, GPIO_MODE_OUT) < E_OK) return E_SYS;
    if (gpio_set_pin(LCD_PIN_DC, GPIO_MODE_OUT) < E_OK) return E_SYS;
    if (gpio_set_pin(LCD_PIN_RST, GPIO_MODE_OUT) < E_OK) return E_SYS;

    gpio_set_val(LCD_PIN_CS, 1U);
    gpio_set_val(LCD_PIN_DC, 1U);
    gpio_set_val(LCD_PIN_RST, 1U);

    spi0_init_lcd();
    lcd_reset();

    /* basic ST7789 start-up sequence */
    lcd_command(CMD_SWRESET);
    tk_dly_tsk(150);

    lcd_command(CMD_SLPOUT);
    tk_dly_tsk(120);

    value = 0x55U;
    lcd_command(CMD_COLMOD);
    lcd_data(&value, 1);

    value = 0x00U;
    lcd_command(CMD_MADCTL);
    lcd_data(&value, 1);

    lcd_command(CMD_INVON);
    lcd_command(CMD_NORON);
    lcd_command(CMD_DISPON);
    tk_dly_tsk(20);

    st7789_fill(ST7789_BLACK);

    return E_OK;
}

void st7789_fill_rect(UH x, UH y, UH width, UH height, UH color)
{
    UW pixels;
    UB high;
    UB low;

    if (width == 0U || height == 0U) {
        return;
    }

    if (x >= ST7789_WIDTH || y >= ST7789_HEIGHT) {
        return;
    }

    /* stop the rectangle from going outside the screen */
    if ((UW)x + width > ST7789_WIDTH) {
        width = ST7789_WIDTH - x;
    }

    if ((UW)y + height > ST7789_HEIGHT) {
        height = ST7789_HEIGHT - y;
    }

    high = (UB)(color >> 8);
    low = (UB)(color & 0xFFU);

    lcd_window(x, y, x + width - 1U, y + height - 1U);

    lcd_select(TRUE);
    gpio_set_val(LCD_PIN_DC, 1U);

    pixels = (UW)width * (UW)height;

    /* send the same colour for every pixel in the area */
    while (pixels-- > 0U) {
        (void)spi0_write_byte(high);
        (void)spi0_write_byte(low);
    }

    spi0_wait();
    lcd_select(FALSE);
}

void st7789_fill(UH color)
{
    st7789_fill_rect(0U, 0U, ST7789_WIDTH, ST7789_HEIGHT, color);
}
