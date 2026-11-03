#include <tk/tkernel.h>
#include <tm/tmonitor.h>

#if TK_SUPPORT_SMP
#include <tk/smp.h>
#endif

#include "menu_controller.h"
#include "display_manager.h"

#define MENU_STACK_SIZE   2048
#define MENU_QUEUE_COUNT  8

typedef enum {
    MENU_SCREEN_MAIN = 0,
    MENU_SCREEN_WIFI,
    MENU_SCREEN_PEER,
    MENU_SCREEN_SYSTEM
} MenuScreen;

static ID menu_mbf_id;

static UW menu_mbf_buffer[
    ((MENU_QUEUE_COUNT * sizeof(InputEvent)) / sizeof(UW))
    + (MENU_QUEUE_COUNT * 4U)
];

static T_CMBF menu_mbf_config = {
    .mbfatr = TA_TFIFO,
    .bufsz = sizeof(menu_mbf_buffer),
    .maxmsz = sizeof(InputEvent),
    .bufptr = menu_mbf_buffer,
};

static MenuItem selected_item = MENU_ITEM_WIFI_SCAN;
static MenuScreen current_screen = MENU_SCREEN_MAIN;

static void move_next(void)
{
    if (selected_item + 1 >= MENU_ITEM_COUNT) {
        selected_item = MENU_ITEM_WIFI_SCAN;
    } else {
        selected_item = (MenuItem)(selected_item + 1);
    }

    display_show_menu(selected_item);
}

static void move_previous(void)
{
    if (selected_item == MENU_ITEM_WIFI_SCAN) {
        selected_item = (MenuItem)(MENU_ITEM_COUNT - 1);
    } else {
        selected_item = (MenuItem)(selected_item - 1);
    }

    display_show_menu(selected_item);
}

static void open_selected(void)
{
    switch (selected_item) {
        case MENU_ITEM_WIFI_SCAN:
            current_screen = MENU_SCREEN_WIFI;
            display_send(DISPLAY_EVENT_WIFI_SCREEN);
            break;

        case MENU_ITEM_PEER_STATUS:
            current_screen = MENU_SCREEN_PEER;
            display_send(DISPLAY_EVENT_PEER_SCREEN);
            break;

        case MENU_ITEM_SYSTEM_INFO:
            current_screen = MENU_SCREEN_SYSTEM;
            display_send(DISPLAY_EVENT_SYSTEM_SCREEN);
            break;

        default:
            break;
    }
}

static void go_back(void)
{
    if (current_screen == MENU_SCREEN_MAIN) {
        return;
    }

    current_screen = MENU_SCREEN_MAIN;
    display_show_menu(selected_item);
}

static void menu_task(INT stacd, void *exinf)
{
    InputEvent event;
    INT size;

    (void)stacd;
    (void)exinf;

    while (1) {
        size = tk_rcv_mbf(menu_mbf_id, &event, TMO_FEVR);

        if (size != (INT)sizeof(InputEvent)) {
            continue;
        }

        switch (event) {
            case INPUT_EVENT_NEXT:
                if (current_screen == MENU_SCREEN_MAIN) {
                    move_next();
                }
                break;

            case INPUT_EVENT_PREVIOUS:
                if (current_screen == MENU_SCREEN_MAIN) {
                    move_previous();
                }
                break;

            case INPUT_EVENT_SELECT:
                if (current_screen == MENU_SCREEN_MAIN) {
                    open_selected();
                }
                break;

            case INPUT_EVENT_BACK:
                go_back();
                break;

            default:
                break;
        }
    }
}

static T_CTSK menu_task_config = {
    .itskpri = 7,
    .stksz = MENU_STACK_SIZE,
    .task = menu_task,
#if TK_SUPPORT_SMP
    .tskatr = TA_HLNG | TA_RNG3 | TA_ASSPRC,
    .assprc = TP_PRC1,
#else
    .tskatr = TA_HLNG | TA_RNG3,
#endif
};

ER menu_controller_init(void)
{
    ID task_id;

    menu_mbf_id = tk_cre_mbf(&menu_mbf_config);

    if (menu_mbf_id <= E_OK) {
        return (ER)menu_mbf_id;
    }

    task_id = tk_cre_tsk(&menu_task_config);

    if (task_id <= E_OK) {
        return (ER)task_id;
    }

    return tk_sta_tsk(task_id, 0);
}

ER menu_controller_send(InputEvent event)
{
    return tk_snd_mbf(
        menu_mbf_id,
        &event,
        (INT)sizeof(InputEvent),
        100
    );
}
