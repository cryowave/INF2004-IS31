#ifndef MENU_CONTROLLER_H
#define MENU_CONTROLLER_H

#include <tk/tkernel.h>
#include "input_message.h"

ER menu_controller_init(void);
ER menu_controller_send(InputEvent event);

#endif
