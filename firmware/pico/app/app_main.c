#include <tk/tkernel.h>
#include <tm/tmonitor.h>

#include "display_manager.h"

EXPORT INT usermain(void)
{
    ER er;

    er = display_manager_init();

    if (er < E_OK) {
        tm_printf((UB *)"[app] display init failed: %d\n", er);
        return 1;
    }

    er = display_send(DISPLAY_EVENT_BOOT);

    if (er < E_OK) {
        tm_printf((UB *)"[app] display message failed: %d\n", er);
        return 1;
    }

    tm_printf((UB *)"[app] Ottie started\n");

    /* usermain must stay alive or the kernel will shut down */
    tk_slp_tsk(TMO_FEVR);

    return 0;
}
