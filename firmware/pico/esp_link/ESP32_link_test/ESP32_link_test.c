#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/stdio_usb.h"
#include "hardware/uart.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

// ---------- ESP32 link (UART1) ----------
#define ESP_UART uart1
#define ESP_TX   8          // Pico GP8 -> ESP32 pin 0 (RX)
#define ESP_RX   9          // Pico GP9 <- ESP32 pin 1 (TX)
#define BAUD     115200

// ---------- LCD (ST7789 on SPI0) ----------
#define LCD_SPI    spi0
#define LCD_SCK    2
#define LCD_MOSI   3
#define LCD_CS     26
#define LCD_DC     7
#define LCD_RES    6
#define LCD_W      240
#define LCD_H      320
#define LCD_INVERT 1        // set to 0 if the colours look inverted

#define BLACK  0x0000
#define WHITE  0xFFFF
#define YELLOW 0xFFE0
#define GREEN  0x07E0
#define RED    0xF800

// ---------- Knob and buttons ----------
#define ENC_A      4
#define ENC_B      5
#define BTN_SELECT 17        // KO button = select (and back, from any other screen)
#define BTN_BACK   16        // knob press (not used, the knob does not press down)
#define ENC_STEPS_PER_CLICK 2   // set to 4 if one click moves the menu by two items

// ---------- 5x7 font, ASCII 32..126 ----------
static const uint8_t font5x7[95][5] = {
 {0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x5F,0x00,0x00},{0x00,0x07,0x00,0x07,0x00},{0x14,0x7F,0x14,0x7F,0x14},
 {0x24,0x2A,0x7F,0x2A,0x12},{0x23,0x13,0x08,0x64,0x62},{0x36,0x49,0x55,0x22,0x50},{0x00,0x05,0x03,0x00,0x00},
 {0x00,0x1C,0x22,0x41,0x00},{0x00,0x41,0x22,0x1C,0x00},{0x08,0x2A,0x1C,0x2A,0x08},{0x08,0x08,0x3E,0x08,0x08},
 {0x00,0x50,0x30,0x00,0x00},{0x08,0x08,0x08,0x08,0x08},{0x00,0x60,0x60,0x00,0x00},{0x20,0x10,0x08,0x04,0x02},
 {0x3E,0x51,0x49,0x45,0x3E},{0x00,0x42,0x7F,0x40,0x00},{0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4B,0x31},
 {0x18,0x14,0x12,0x7F,0x10},{0x27,0x45,0x45,0x45,0x39},{0x3C,0x4A,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},
 {0x36,0x49,0x49,0x49,0x36},{0x06,0x49,0x49,0x29,0x1E},{0x00,0x36,0x36,0x00,0x00},{0x00,0x56,0x36,0x00,0x00},
 {0x00,0x08,0x14,0x22,0x41},{0x14,0x14,0x14,0x14,0x14},{0x41,0x22,0x14,0x08,0x00},{0x02,0x01,0x51,0x09,0x06},
 {0x32,0x49,0x79,0x41,0x3E},{0x7E,0x11,0x11,0x11,0x7E},{0x7F,0x49,0x49,0x49,0x36},{0x3E,0x41,0x41,0x41,0x22},
 {0x7F,0x41,0x41,0x22,0x1C},{0x7F,0x49,0x49,0x49,0x41},{0x7F,0x09,0x09,0x01,0x01},{0x3E,0x41,0x41,0x51,0x32},
 {0x7F,0x08,0x08,0x08,0x7F},{0x00,0x41,0x7F,0x41,0x00},{0x20,0x40,0x41,0x3F,0x01},{0x7F,0x08,0x14,0x22,0x41},
 {0x7F,0x40,0x40,0x40,0x40},{0x7F,0x02,0x04,0x02,0x7F},{0x7F,0x04,0x08,0x10,0x7F},{0x3E,0x41,0x41,0x41,0x3E},
 {0x7F,0x09,0x09,0x09,0x06},{0x3E,0x41,0x51,0x21,0x5E},{0x7F,0x09,0x19,0x29,0x46},{0x46,0x49,0x49,0x49,0x31},
 {0x01,0x01,0x7F,0x01,0x01},{0x3F,0x40,0x40,0x40,0x3F},{0x1F,0x20,0x40,0x20,0x1F},{0x7F,0x20,0x18,0x20,0x7F},
 {0x63,0x14,0x08,0x14,0x63},{0x03,0x04,0x78,0x04,0x03},{0x61,0x51,0x49,0x45,0x43},{0x00,0x00,0x7F,0x41,0x41},
 {0x02,0x04,0x08,0x10,0x20},{0x41,0x41,0x7F,0x00,0x00},{0x04,0x02,0x01,0x02,0x04},{0x40,0x40,0x40,0x40,0x40},
 {0x00,0x01,0x02,0x04,0x00},{0x20,0x54,0x54,0x54,0x78},{0x7F,0x48,0x44,0x44,0x38},{0x38,0x44,0x44,0x44,0x20},
 {0x38,0x44,0x44,0x48,0x7F},{0x38,0x54,0x54,0x54,0x18},{0x08,0x7E,0x09,0x01,0x02},{0x08,0x14,0x54,0x54,0x3C},
 {0x7F,0x08,0x04,0x04,0x78},{0x00,0x44,0x7D,0x40,0x00},{0x20,0x40,0x44,0x3D,0x00},{0x00,0x7F,0x10,0x28,0x44},
 {0x00,0x41,0x7F,0x40,0x00},{0x7C,0x04,0x18,0x04,0x78},{0x7C,0x08,0x04,0x04,0x78},{0x38,0x44,0x44,0x44,0x38},
 {0x7C,0x14,0x14,0x14,0x08},{0x08,0x14,0x14,0x18,0x7C},{0x7C,0x08,0x04,0x04,0x08},{0x48,0x54,0x54,0x54,0x20},
 {0x04,0x3F,0x44,0x40,0x20},{0x3C,0x40,0x40,0x20,0x7C},{0x1C,0x20,0x40,0x20,0x1C},{0x3C,0x40,0x30,0x40,0x3C},
 {0x44,0x28,0x10,0x28,0x44},{0x0C,0x50,0x50,0x50,0x3C},{0x44,0x64,0x54,0x4C,0x44},{0x00,0x08,0x36,0x41,0x00},
 {0x00,0x00,0x7F,0x00,0x00},{0x00,0x41,0x36,0x08,0x00},{0x02,0x01,0x02,0x04,0x02}
};

// ---------- LCD driver ----------
static void lcd_cmd(uint8_t cmd) {
    gpio_put(LCD_DC, 0);
    gpio_put(LCD_CS, 0);
    spi_write_blocking(LCD_SPI, &cmd, 1);
    gpio_put(LCD_CS, 1);
}

static void lcd_data(const uint8_t *data, size_t len) {
    gpio_put(LCD_DC, 1);
    gpio_put(LCD_CS, 0);
    spi_write_blocking(LCD_SPI, data, len);
    gpio_put(LCD_CS, 1);
}

static void lcd_cmd_arg(uint8_t cmd, uint8_t arg) {
    lcd_cmd(cmd);
    lcd_data(&arg, 1);
}

// Choose the rectangle that the next pixels will fill
static void lcd_window(int x0, int y0, int x1, int y1) {
    uint8_t col[4] = { x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF };
    uint8_t row[4] = { y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF };
    lcd_cmd(0x2A); lcd_data(col, 4);
    lcd_cmd(0x2B); lcd_data(row, 4);
    lcd_cmd(0x2C);                       // start writing pixels
}

static void lcd_fill(int x, int y, int w, int h, uint16_t color) {
    uint8_t row[LCD_W * 2];
    for (int i = 0; i < w; i++) {
        row[2 * i]     = color >> 8;
        row[2 * i + 1] = color & 0xFF;
    }
    lcd_window(x, y, x + w - 1, y + h - 1);
    for (int j = 0; j < h; j++) lcd_data(row, w * 2);
}

static void lcd_init(void) {
    spi_init(LCD_SPI, 10 * 1000 * 1000);
    gpio_set_function(LCD_SCK, GPIO_FUNC_SPI);
    gpio_set_function(LCD_MOSI, GPIO_FUNC_SPI);
    gpio_init(LCD_CS);  gpio_set_dir(LCD_CS, GPIO_OUT);  gpio_put(LCD_CS, 1);
    gpio_init(LCD_DC);  gpio_set_dir(LCD_DC, GPIO_OUT);
    gpio_init(LCD_RES); gpio_set_dir(LCD_RES, GPIO_OUT);

    gpio_put(LCD_RES, 0); sleep_ms(50);   // hardware reset
    gpio_put(LCD_RES, 1); sleep_ms(120);

    lcd_cmd(0x01); sleep_ms(150);         // software reset
    lcd_cmd(0x11); sleep_ms(120);         // wake up
    lcd_cmd_arg(0x3A, 0x55);              // 16-bit colour
    lcd_cmd_arg(0x36, 0x00);              // portrait orientation
#if LCD_INVERT
    lcd_cmd(0x21);                        // colour inversion on
#endif
    lcd_cmd(0x13);                        // normal mode
    lcd_cmd(0x29); sleep_ms(20);          // display on
    lcd_fill(0, 0, LCD_W, LCD_H, BLACK);
}

// One character, 12 x 16 pixels (5x7 font drawn at 2x size)
static void lcd_char(int x, int y, char c, uint16_t fg, uint16_t bg) {
    if (c < 32 || c > 126) c = '?';
    const uint8_t *glyph = font5x7[c - 32];
    uint8_t buf[12 * 16 * 2];
    int n = 0;
    for (int py = 0; py < 16; py++) {
        for (int px = 0; px < 12; px++) {
            int col = px / 2, row = py / 2;
            bool on = (col < 5) && (row < 7) && ((glyph[col] >> row) & 1);
            uint16_t color = on ? fg : bg;
            buf[n++] = color >> 8;
            buf[n++] = color & 0xFF;
        }
    }
    lcd_window(x, y, x + 11, y + 15);
    lcd_data(buf, sizeof buf);
}

static void lcd_text(int x, int y, const char *s, uint16_t fg, uint16_t bg) {
    for (; *s && x + 12 <= LCD_W; s++, x += 12) lcd_char(x, y, *s, fg, bg);
}

// ---------- ESP32 link helpers ----------
// Read one line from the ESP32. Returns false if nothing arrives in time.
static bool read_line(char *buf, int size, uint32_t timeout_ms) {
    int n = 0;
    absolute_time_t end = make_timeout_time_ms(timeout_ms);
    while (!time_reached(end)) {
        if (uart_is_readable(ESP_UART)) {
            char c = uart_getc(ESP_UART);
            if (c == '\n') { buf[n] = 0; return true; }
            if (c != '\r' && n < size - 1) buf[n++] = c;
        }
    }
    return false;
}

// Throw away leftover bytes (e.g. the junk character at start-up)
static void drain_uart(void) {
    while (uart_is_readable(ESP_UART)) uart_getc(ESP_UART);
}

// ---------- Buttons ----------
typedef struct {
    unsigned pin;
    bool stable;
    bool last;
    int count;
} Button;

static void button_init(Button *b, unsigned pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_up(pin);
    b->pin = pin;
    b->stable = b->last = gpio_get(pin);
    b->count = 0;
}

// Returns true once each time the button is pressed (debounced)
static bool button_pressed(Button *b) {
    bool v = gpio_get(b->pin);
    if (v == b->last) {
        if (b->count < 5) b->count++;
    } else {
        b->last = v;
        b->count = 0;
    }
    if (b->count >= 5 && b->stable != b->last) {
        b->stable = b->last;
        b->count = 0;
        return !b->stable;               // pulled low = pressed
    }
    return false;
}

// ---------- Screens ----------
static const char *menu_items[3] = { "WIFI SCAN", "PEER STATUS", "SYSTEM INFO" };

static void draw_menu(int selected) {
    lcd_fill(0, 0, LCD_W, LCD_H, BLACK);
    lcd_text(84, 30, "OTTIE", YELLOW, BLACK);
    for (int i = 0; i < 3; i++) {
        int y = 100 + i * 40;
        lcd_text(18, y, i == selected ? ">" : " ", WHITE, BLACK);
        lcd_text(42, y, menu_items[i], WHITE, BLACK);
    }
}

static void draw_status(const char *title, const char *status) {
    lcd_fill(0, 0, LCD_W, LCD_H, BLACK);
    lcd_text(18, 95, title, WHITE, BLACK);
    lcd_text(18, 150, status, WHITE, BLACK);
}

// Ask the ESP32 for a Wi-Fi scan and show the result on the LCD
static void do_wifi_scan(void) {
    draw_status("WIFI SCAN", "SCANNING...");

    uart_puts(ESP_UART, "\n");          // flush any junk the ESP32 received
    sleep_ms(300);
    drain_uart();
    absolute_time_t t0 = get_absolute_time();
    uart_puts(ESP_UART, "SCAN\n");

    char line[128];
    char aps[10][64];
    int count = 0;
    bool ok = false;

    // receive everything first (drawing is slow and would lose bytes)
    while (read_line(line, sizeof line, 8000)) {
        printf("%s\n", line);
        if (strncmp(line, "AP,", 3) == 0) {
            if (count < 10) {
                strncpy(aps[count], line + 3, 63);
                aps[count][63] = 0;
                count++;
            }
        } else if (strncmp(line, "END", 3) == 0) {
            ok = true;
            break;
        } else if (strncmp(line, "ERR", 3) == 0) {
            break;
        }
    }
    int ms = (int)(absolute_time_diff_us(t0, get_absolute_time()) / 1000);

    lcd_fill(0, 0, LCD_W, LCD_H, BLACK);
    lcd_text(0, 4, "WIFI SCAN", WHITE, BLACK);

    for (int i = 0; i < count; i++) {
        // stored as: <ssid>,<rssi>,<channel>
        char *ssid = aps[i];
        char *c1 = strchr(ssid, ',');
        if (!c1) continue;
        *c1 = 0;
        char *rssi = c1 + 1;
        char *c2 = strchr(rssi, ',');
        if (!c2) continue;
        *c2 = 0;
        char *chan = c2 + 1;

        char text[40];
        snprintf(text, sizeof text, "%-12.12s %4s %2s", ssid, rssi, chan);
        lcd_text(0, 30 + i * 22, text, WHITE, BLACK);
    }

    char foot[40];
    if (ok) {
        snprintf(foot, sizeof foot, "%d found, %d ms", count, ms);
        lcd_text(0, 260, foot, GREEN, BLACK);
    } else {
        lcd_text(0, 260, "Scan failed", RED, BLACK);
    }
    lcd_text(0, 292, "KO = menu", YELLOW, BLACK);
    printf("[esp] scan finished, ok=%d, networks=%d, %d ms\n", ok, count, ms);
}

static int encoder_state(void) {
    return (gpio_get(ENC_A) << 1) | gpio_get(ENC_B);
}

int main() {
    stdio_init_all();
    uart_init(ESP_UART, BAUD);
    gpio_set_function(ESP_TX, GPIO_FUNC_UART);
    gpio_set_function(ESP_RX, GPIO_FUNC_UART);
    lcd_init();

    gpio_init(ENC_A); gpio_set_dir(ENC_A, GPIO_IN); gpio_pull_up(ENC_A);
    gpio_init(ENC_B); gpio_set_dir(ENC_B, GPIO_IN); gpio_pull_up(ENC_B);
    Button select_btn, back_btn;
    button_init(&select_btn, BTN_SELECT);
    button_init(&back_btn, BTN_BACK);

    // Optional: give PuTTY up to 3 s to connect (the menu works without it)
    for (int i = 0; i < 30 && !stdio_usb_connected(); i++) sleep_ms(100);
    printf("Ottie menu demo started\n");

    // +1 / -1 for each valid knob step, 0 for no change or a bounce
    static const int8_t table[16] = { 0,-1, 1, 0,  1, 0, 0,-1, -1, 0, 0, 1,  0, 1,-1, 0 };

    int selected = 0;
    bool on_menu = true;
    int last = encoder_state();
    int acc = 0;
    draw_menu(selected);

    while (true) {
        int now = encoder_state();
        if (now != last) {
            acc += table[(last << 2) | now];
            last = now;
            if (!on_menu) {
                acc = 0;
            } else if (acc >= ENC_STEPS_PER_CLICK) {
                acc = 0;
                selected = (selected + 1) % 3;
                printf("[input] next\n");
                draw_menu(selected);
            } else if (acc <= -ENC_STEPS_PER_CLICK) {
                acc = 0;
                selected = (selected + 2) % 3;
                printf("[input] previous\n");
                draw_menu(selected);
            }
        }

        if (button_pressed(&select_btn)) {
            printf("[input] select\n");
            if (on_menu) {
                on_menu = false;
                if (selected == 0) do_wifi_scan();
                else               draw_status(menu_items[selected], "NOT READY");
                last = encoder_state();
                acc = 0;
            } else {
                on_menu = true;                 // press again to go back to the menu
                draw_menu(selected);
            }
        }

        if (button_pressed(&back_btn)) {
            printf("[input] back\n");
            if (!on_menu) {
                on_menu = true;
                draw_menu(selected);
            }
        }

        sleep_ms(2);
    }
}