#include <string.h>

#include <tk/tkernel.h>
#include <tk/syslib.h>
#include <tm/tmonitor.h>
#include <sys/sysdef.h>

#if TK_SUPPORT_SMP
#include <tk/smp.h>
#endif

#include "esp_link.h"

#define ESP_TX_PIN          8U
#define ESP_RX_PIN          9U
#define ESP_BAUD_IBRD       67U
#define ESP_BAUD_FBRD       52U

#define ESP_TASK_STACK_SIZE 3072
#define ESP_QUEUE_COUNT     2
#define ESP_LINE_SIZE       128
#define ESP_LINE_TIMEOUT_MS 8000

#define UART1_BASE          0x40038000UL
#define UART_DR             (UART1_BASE + 0x000U)
#define UART_FR             (UART1_BASE + 0x018U)
#define UART_IBRD           (UART1_BASE + 0x024U)
#define UART_FBRD           (UART1_BASE + 0x028U)
#define UART_LCR_H          (UART1_BASE + 0x02CU)
#define UART_CR             (UART1_BASE + 0x030U)
#define UART_ICR            (UART1_BASE + 0x044U)

#define UART_FR_TXFF        (1U << 5)
#define UART_FR_RXFE        (1U << 4)

#define UART_LCR_H_WLEN_8   (3U << 5)
#define UART_LCR_H_FEN      (1U << 4)

#define UART_CR_RXE         (1U << 9)
#define UART_CR_TXE         (1U << 8)
#define UART_CR_UARTEN      (1U << 0)

typedef enum {
    ESP_COMMAND_SCAN = 0
} EspCommand;

static ID esp_mbf_id;

static UW esp_mbf_buffer[
    ((ESP_QUEUE_COUNT * sizeof(EspCommand)) / sizeof(UW))
    + (ESP_QUEUE_COUNT * 4U)
];

static T_CMBF esp_mbf_config = {
    .mbfatr = TA_TFIFO,
    .bufsz = sizeof(esp_mbf_buffer),
    .maxmsz = sizeof(EspCommand),
    .bufptr = esp_mbf_buffer,
};

static void esp_uart_init(void)
{
    /* UART1 is not released by the normal board startup, so do it here. */
    clr_w(RESETS_RESET, RESETS_RESET_UART1);

    while ((in_w(RESETS_RESET_DONE) & RESETS_RESET_UART1) == 0U) {
    }

    /*
     * GP8/GP9 are used as UART1 for the ESP32.
     * This replaces the sample I2C pin function set during board startup.
     */
    out_w(GPIO_CTRL(ESP_TX_PIN), GPIO_CTRL_FUNCSEL_UART);
    out_w(GPIO_CTRL(ESP_RX_PIN), GPIO_CTRL_FUNCSEL_UART);

    out_w(
        GPIO(ESP_TX_PIN),
        GPIO_IE | GPIO_DRIVE_4MA | GPIO_SHEMITT
    );
    out_w(
        GPIO(ESP_RX_PIN),
        GPIO_IE | GPIO_DRIVE_4MA | GPIO_SHEMITT
    );

    out_w(UART_CR, 0U);

    /* 115200 baud with the RP2040 peripheral clock used by this port. */
    out_w(UART_IBRD, ESP_BAUD_IBRD);
    out_w(UART_FBRD, ESP_BAUD_FBRD);

    out_w(
        UART_LCR_H,
        UART_LCR_H_WLEN_8 | UART_LCR_H_FEN
    );

    out_w(UART_ICR, 0x7FFU);

    out_w(
        UART_CR,
        UART_CR_RXE | UART_CR_TXE | UART_CR_UARTEN
    );
}

static BOOL esp_uart_readable(void)
{
    return (in_w(UART_FR) & UART_FR_RXFE) == 0U;
}

static void esp_uart_putc(char c)
{
    while ((in_w(UART_FR) & UART_FR_TXFF) != 0U) {
        tk_dly_tsk(1);
    }

    out_w(UART_DR, (UW)(UB)c);
}

static void esp_uart_write(const char *text)
{
    while (*text != '\0') {
        esp_uart_putc(*text);
        text++;
    }
}

static void esp_uart_drain(void)
{
    while (esp_uart_readable()) {
        (void)in_w(UART_DR);
    }
}

static BOOL esp_read_line(char *buffer, INT size, INT timeout_ms)
{
    INT count = 0;
    INT waited = 0;

    while (waited < timeout_ms) {
        while (esp_uart_readable()) {
            char c = (char)(in_w(UART_DR) & 0xFFU);

            if (c == '\n') {
                buffer[count] = '\0';
                return TRUE;
            }

            if (c != '\r' && count < size - 1) {
                buffer[count] = c;
                count++;
            }
        }

        tk_dly_tsk(1);
        waited++;
    }

    buffer[count] = '\0';
    return FALSE;
}

static void run_scan(void)
{
    char line[ESP_LINE_SIZE];
    INT ap_count = 0;
    BOOL finished = FALSE;

    tm_printf((UB *)"[esp] scan requested\n");

    /*
     * Justin's working prototype sends a blank line first so any partial
     * command left at start-up is completed, then clears old reply bytes.
     */
    esp_uart_write("\n");
    tk_dly_tsk(300);
    esp_uart_drain();

    esp_uart_write("SCAN\n");

    while (esp_read_line(line, ESP_LINE_SIZE, ESP_LINE_TIMEOUT_MS)) {
        if (strncmp(line, "AP,", 3) == 0) {
            ap_count++;
            tm_printf((UB *)"[esp] %s\n", line);
            continue;
        }

        if (strncmp(line, "END", 3) == 0) {
            tm_printf(
                (UB *)"[esp] scan complete: %d APs\n",
                ap_count
            );
            finished = TRUE;
            break;
        }

        if (strncmp(line, "ERR", 3) == 0) {
            tm_printf((UB *)"[esp] %s\n", line);
            finished = TRUE;
            break;
        }

        if (line[0] != '\0') {
            tm_printf((UB *)"[esp] %s\n", line);
        }
    }

    if (!finished) {
        tm_printf((UB *)"[esp] scan timed out\n");
    }
}

static void esp_task(INT stacd, void *exinf)
{
    EspCommand command;
    INT size;

    (void)stacd;
    (void)exinf;

    tm_printf((UB *)"[esp] UART1 ready on GP8/GP9\n");

    while (1) {
        size = tk_rcv_mbf(
            esp_mbf_id,
            &command,
            TMO_FEVR
        );

        if (size != (INT)sizeof(EspCommand)) {
            continue;
        }

        if (command == ESP_COMMAND_SCAN) {
            run_scan();
        }
    }
}

static T_CTSK esp_task_config = {
    .itskpri = 7,
    .stksz = ESP_TASK_STACK_SIZE,
    .task = esp_task,
#if TK_SUPPORT_SMP
    .tskatr = TA_HLNG | TA_RNG3 | TA_ASSPRC,
    .assprc = TP_PRC2,
#else
    .tskatr = TA_HLNG | TA_RNG3,
#endif
};

ER esp_link_init(void)
{
    ID task_id;

    esp_uart_init();

    esp_mbf_id = tk_cre_mbf(&esp_mbf_config);

    if (esp_mbf_id <= E_OK) {
        return (ER)esp_mbf_id;
    }

    task_id = tk_cre_tsk(&esp_task_config);

    if (task_id <= E_OK) {
        return (ER)task_id;
    }

    return tk_sta_tsk(task_id, 0);
}

ER esp_link_request_scan(void)
{
    EspCommand command = ESP_COMMAND_SCAN;

    return tk_snd_mbf(
        esp_mbf_id,
        &command,
        (INT)sizeof(EspCommand),
        100
    );
}
