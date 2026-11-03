#include <tk/tkernel.h>
#include <tm/tmonitor.h>

#include "display_manager.h"
#include "menu_controller.h"
#include "user_input.h"

EXPORT INT usermain(void)
{
    ER er;

    er = display_manager_init();

    if (er < E_OK) {
        tm_printf((UB *)"[app] display init failed: %d\n", er);
        return 1;
    }

    er = menu_controller_init();

    if (er < E_OK) {
        tm_printf((UB *)"[app] menu init failed: %d\n", er);
        return 1;
    }

    er = display_send(DISPLAY_EVENT_BOOT);

    if (er < E_OK) {
        tm_printf((UB *)"[app] display message failed: %d\n", er);
        return 1;
    }

    tk_dly_tsk(1000);

    er = display_show_menu(MENU_ITEM_WIFI_SCAN);

    if (er < E_OK) {
        tm_printf((UB *)"[app] menu display failed: %d\n", er);
        return 1;
    }

    er = user_input_init();

    if (er < E_OK) {
        tm_printf((UB *)"[app] input init failed: %d\n", er);
        return 1;
    }

    tm_printf((UB *)"[app] Ottie navigation started\n");

    tk_slp_tsk(TMO_FEVR);

    return 0;
}
