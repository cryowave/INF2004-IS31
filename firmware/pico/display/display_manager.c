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

/* space used by the uT-Kernel message buffer */
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

static void display_task(INT stacd, void *exinf)
{
    DisplayMessage message;
    INT size;
    ER er;

    (void)stacd;
    (void)exinf;

    /* only this task starts and writes to the LCD */
    er = st7789_init();

    if (er < E_OK) {
        tm_printf((UB *)"[display] lcd init failed: %d\n", er);
        tk_slp_tsk(TMO_FEVR);
    }

    while (1) {
        /* wait here until another part of the program sends a display message */
        size = tk_rcv_mbf(display_mbf_id, &message, TMO_FEVR);

        if (size != (INT)sizeof(DisplayMessage)) {
            continue;
        }

        if (message.event == DISPLAY_EVENT_BOOT) {
            show_boot_screen();
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

    return tk_snd_mbf(
        display_mbf_id,
        &message,
        (INT)sizeof(DisplayMessage),
        100
    );
}
