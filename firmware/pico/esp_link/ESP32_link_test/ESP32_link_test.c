#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "pico/stdio_usb.h"

#define ESP_UART uart1
#define ESP_TX   8      // Pico GP8 -> ESP32 pin 0 (RX)
#define ESP_RX   9      // Pico GP9 <- ESP32 pin 1 (TX)
#define BAUD     115200

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

int main() {
    stdio_init_all();
    uart_init(ESP_UART, BAUD);
    gpio_set_function(ESP_TX, GPIO_FUNC_UART);
    gpio_set_function(ESP_RX, GPIO_FUNC_UART);

    while (!stdio_usb_connected()) sleep_ms(100);   // wait for PuTTY to open
    printf("Pico ready\n");

    char line[128];

    // Test 1: PING -> PONG
    uart_puts(ESP_UART, "PING\n");
    if (read_line(line, sizeof line, 2000)) printf("PING reply: %s\n", line);
    else                                    printf("PING: no reply\n");

    // Test 2: SCAN every 10 seconds
    while (true) {
        printf("\nSending SCAN...\n");
        absolute_time_t t0 = get_absolute_time();
        uart_puts(ESP_UART, "SCAN\n");

        while (read_line(line, sizeof line, 8000)) {
            printf("%s\n", line);
            if (strncmp(line, "END", 3) == 0 || strncmp(line, "ERR", 3) == 0) break;
        }
        printf("[round trip %lld ms]\n", absolute_time_diff_us(t0, get_absolute_time()) / 1000);
        sleep_ms(10000);
    }
}