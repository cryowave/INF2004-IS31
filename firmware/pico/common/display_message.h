#ifndef DISPLAY_MESSAGE_H
#define DISPLAY_MESSAGE_H

typedef enum {
    MENU_ITEM_WIFI_SCAN = 0,
    MENU_ITEM_PEER_STATUS,
    MENU_ITEM_SYSTEM_INFO,
    MENU_ITEM_COUNT
} MenuItem;

typedef enum {
    DISPLAY_EVENT_BOOT = 0,
    DISPLAY_EVENT_SHOW_MENU
} DisplayEvent;

typedef struct {
    DisplayEvent event;
    MenuItem menu_item;
} DisplayMessage;

#endif
