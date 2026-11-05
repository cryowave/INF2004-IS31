#ifndef DISPLAY_MESSAGE_H
#define DISPLAY_MESSAGE_H

#include "wifi_scan_result.h"

typedef enum {
    MENU_ITEM_WIFI_SCAN = 0,
    MENU_ITEM_PEER_STATUS,
    MENU_ITEM_SYSTEM_INFO,
    MENU_ITEM_COUNT
} MenuItem;

typedef enum {
    ESP_STATUS_UNKNOWN = 0,
    ESP_STATUS_OK,
    ESP_STATUS_LOST
} EspStatus;

typedef enum {
    DISPLAY_EVENT_BOOT = 0,
    DISPLAY_EVENT_SHOW_MENU,
    DISPLAY_EVENT_WIFI_SCREEN,
    DISPLAY_EVENT_WIFI_RESULTS,
    DISPLAY_EVENT_PEER_SCREEN,
    DISPLAY_EVENT_SYSTEM_SCREEN,
    DISPLAY_EVENT_ESP_STATUS
} DisplayEvent;

typedef struct {
    DisplayEvent event;
    MenuItem menu_item;
    WifiScanResult wifi_result;
    EspStatus esp_status;
} DisplayMessage;

#endif
