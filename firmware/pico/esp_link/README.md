# Pico-to-ESP32 Link

Primary Wi-Fi / ESP32 integration: Justin Nicholas Yang

This folder contains the Pico-side integration for Justin's existing ESP32-C3
Wi-Fi scanner.

Hardware:
- Pico GP8 TX -> ESP32 RX
- Pico GP9 RX <- ESP32 TX
- common GND
- UART1 at 115200 baud

Protocol already used by the ESP32:
- Pico sends `SCAN`
- ESP32 replies with `AP,<ssid>,<rssi>,<channel>`
- the scan ends with `END,<count>`
- errors use `ERR,...`

Current uT-Kernel integration:
- UART1 is configured for GP8/GP9
- a dedicated ESP Link Task runs on processor 2
- the Menu Controller can request a scan
- scan replies are received before any LCD result drawing
- AP replies are printed to USB serial for integration testing

The board startup configures GP8/GP9 as sample I2C pins, so this module changes
those two pins to UART1 when the ESP link starts. The current Ottie design does
not use I2C on GP8/GP9.

Push 9 proves the real ESP32 scan path under uT-Kernel. Rendering the parsed
access-point list on the Ottie LCD is handled in the next display integration
stage.
