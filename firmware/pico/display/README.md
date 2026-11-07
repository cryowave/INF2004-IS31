# Display Module

Reusable ST7789 display support for the Ottie eBadge.

Current support:
- dedicated uT-Kernel Display Task
- main Ottie menu and feature screens
- Wi-Fi scanning status and completed result list
- SSID, RSSI and channel output
- ESP OK / ESP LOST status badge
- specific Wi-Fi error screens for ESP lost, busy, scan failure and timeout
- scan duration shown on the result screen
- serial timing log for how long the LCD result update takes

Only the Display Task directly accesses the LCD. Other tasks send display
messages through the display message buffer.

Wi-Fi scan replies are buffered by the ESP Link Task first, then the completed
result is rendered. This keeps slow LCD drawing out of the UART receive path.
