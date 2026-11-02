#include <tk/tkernel.h>
#include <tm/tmonitor.h>

#if TK_SUPPORT_SMP
#include <tk/smp.h>
#endif

#include "display_manager.h"
#include "st7789.h"

#define DISPLAY_STACK_SIZE   4096
#define DISPLAY_QUEUE_COUNT  8

static ID display_mbf_id;

static UW display_mbf_buffer[
    ((DISPLAY_QUEUE_COUNT * sizeof(DisplayMessage)) / sizeof(UW))
    + (DISPLAY_QUEUE_COUNT * 4U)
];

static T_CMBF display_mbf_config = {
    .mbfatr = TA_TFIFO,
    .bufsz = sizeof(display_mbf_buffer),
    .maxmsz = sizeof(DisplayMessage),
    .bufptr = display_mbf_buffer,
};

static void show_boot_screen(void)
{
    st7789_fill(ST7789_BLACK);

    st7789_draw_text(
        21U,
        145U,
        "OTTIE READY",
        ST7789_WHITE,
        ST7789_BLACK,
        3U
    );
}

static void draw_menu_marker(UH y)
{
    st7789_fill_rect(18U, y + 2U, 4U, 4U, ST7789_WHITE);
    st7789_fill_rect(22U, y + 6U, 4U, 4U, ST7789_WHITE);
    st7789_fill_rect(18U, y + 10U, 4U, 4U, ST7789_WHITE);
}

static void draw_menu_item(UH y, const char *text, BOOL selected)
{
    if (selected) {
        draw_menu_marker(y);
    }

    st7789_draw_text(
        42U,
        y,
        text,
        ST7789_WHITE,
        ST7789_BLACK,
        2U
    );
}

static void show_menu(MenuItem selected)
{
    st7789_fill(ST7789_BLACK);

    st7789_draw_text(
        75U,
        35U,
        "OTTIE",
        ST7789_WHITE,
        ST7789_BLACK,
        3U
    );

    draw_menu_item(105U, "WIFI SCAN", selected == MENU_ITEM_WIFI_SCAN);
    draw_menu_item(145U, "PEER STATUS", selected == MENU_ITEM_PEER_STATUS);
    draw_menu_item(185U, "SYSTEM INFO", selected == MENU_ITEM_SYSTEM_INFO);
}

static void show_status_screen(const char *title, const char *status)
{
    st7789_fill(ST7789_BLACK);

    st7789_draw_text(
        18U,
        95U,
        title,
        ST7789_WHITE,
        ST7789_BLACK,
        2U
    );

    st7789_draw_text(
        18U,
        150U,
        status,
        ST7789_WHITE,
        ST7789_BLACK,
        2U
    );
}

static void show_wifi_screen(void)
{
    show_status_screen("WIFI SCAN", "NOT READY");
}

static void show_peer_screen(void)
{
    show_status_screen("PEER STATUS", "NOT READY");
}

static void show_system_screen(void)
{
    show_status_screen("SYSTEM INFO", "SMP READY");
}

static void display_task(INT stacd, void *exinf)
{
    DisplayMessage message;
    INT size;
    ER er;

    (void)stacd;
    (void)exinf;

    er = st7789_init();

    if (er < E_OK) {
        tm_printf((UB *)"[display] lcd init failed: %d\n", er);
        tk_slp_tsk(TMO_FEVR);
    }

    while (1) {
        size = tk_rcv_mbf(display_mbf_id, &message, TMO_FEVR);

        if (size != (INT)sizeof(DisplayMessage)) {
            continue;
        }

        switch (message.event) {
            case DISPLAY_EVENT_BOOT:
                show_boot_screen();
                break;

            case DISPLAY_EVENT_SHOW_MENU:
                show_menu(message.menu_item);
                break;

            case DISPLAY_EVENT_WIFI_SCREEN:
                show_wifi_screen();
                break;

            case DISPLAY_EVENT_PEER_SCREEN:
                show_peer_screen();
                break;

            case DISPLAY_EVENT_SYSTEM_SCREEN:
                show_system_screen();
                break;

            default:
                break;
        }
    }
}

static T_CTSK display_task_config = {
    .itskpri = 8,
    .stksz = DISPLAY_STACK_SIZE,
    .task = display_task,
#if TK_SUPPORT_SMP
    .tskatr = TA_HLNG | TA_RNG3 | TA_ASSPRC,
    .assprc = TP_PRC1,
#else
    .tskatr = TA_HLNG | TA_RNG3,
#endif
};

ER display_manager_init(void)
{
    ID task_id;

    display_mbf_id = tk_cre_mbf(&display_mbf_config);

    if (display_mbf_id <= E_OK) {
        return (ER)display_mbf_id;
    }

    task_id = tk_cre_tsk(&display_task_config);

    if (task_id <= E_OK) {
        return (ER)task_id;
    }

    return tk_sta_tsk(task_id, 0);
}

ER display_send(DisplayEvent event)
{
    DisplayMessage message;

    message.event = event;
    message.menu_item = MENU_ITEM_WIFI_SCAN;

    return tk_snd_mbf(
        display_mbf_id,
        &message,
        (INT)sizeof(DisplayMessage),
        100
    );
}

ER display_show_menu(MenuItem selected)
{
    DisplayMessage message;

    message.event = DISPLAY_EVENT_SHOW_MENU;
    message.menu_item = selected;

    return tk_snd_mbf(
        display_mbf_id,
        &message,
        (INT)sizeof(DisplayMessage),
        100
    );
}
