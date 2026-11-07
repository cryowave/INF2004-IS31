# Pico-to-ESP32 Link

Primary Wi-Fi / ESP32 implementation: Justin Nicholas Yang

The final Pico application reuses Justin's existing ESP32-C3 protocol:

- Pico sends `PING`
- ESP32 replies `PONG`
- ESP32 sends `READY` after startup
- Pico sends `SCAN`
- ESP32 replies with `AP,<ssid>,<rssi>,<channel>`
- the scan ends with `END,<count>`
- scan errors include `ERR,BUSY` and `ERR,SCAN_FAILED`

Current uT-Kernel integration:
- one ESP Link Task owns UART1 on processor 2
- idle heartbeat sends PING every 500 ms
- no PONG for 1500 ms marks the ESP32 as lost
- heartbeat continues while the ESP32 performs its asynchronous passive scan
- AP results are buffered completely before LCD drawing
- scan duration is recorded
- BUSY, SCAN_FAILED, timeout and lost-link failures are kept separate
- completed scan results are sent to the Display Task

Keeping one UART1 owner prevents heartbeat and scan parsing from stealing bytes
from one another.
