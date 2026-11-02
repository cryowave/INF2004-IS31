#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <tk/tkernel.h>
#include "display_message.h"

ER display_manager_init(void);
ER display_send(DisplayEvent event);

#endif
