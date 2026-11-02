#ifndef DISPLAY_MESSAGE_H
#define DISPLAY_MESSAGE_H

typedef enum {
    DISPLAY_EVENT_BOOT = 0
} DisplayEvent;

typedef struct {
    DisplayEvent event;
} DisplayMessage;

#endif
