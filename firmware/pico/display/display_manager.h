#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <tk/tkernel.h>
#include "display_message.h"

ER display_manager_init(void);
ER display_send(DisplayEvent event);
ER display_show_menu(MenuItem selected);
ER display_show_wifi_results(const WifiScanResult *result);

#endif
