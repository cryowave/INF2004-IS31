#ifndef WIFI_SCAN_RESULT_H
#define WIFI_SCAN_RESULT_H

#define WIFI_MAX_APS       10
#define WIFI_MAX_SSID_LEN  32

typedef enum {
    WIFI_SCAN_ERROR_NONE = 0,
    WIFI_SCAN_ERROR_ESP_LOST,
    WIFI_SCAN_ERROR_BUSY,
    WIFI_SCAN_ERROR_SCAN_FAILED,
    WIFI_SCAN_ERROR_TIMEOUT,
    WIFI_SCAN_ERROR_PROTOCOL
} WifiScanError;

typedef struct {
    char ssid[WIFI_MAX_SSID_LEN + 1];
    int rssi;
    int channel;
} WifiAccessPoint;

typedef struct {
    int success;
    int count;
    int scan_ms;
    WifiScanError error;
    WifiAccessPoint aps[WIFI_MAX_APS];
} WifiScanResult;

#endif
