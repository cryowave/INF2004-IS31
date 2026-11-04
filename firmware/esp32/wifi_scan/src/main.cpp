#include <Arduino.h>
#include <WiFi.h>

const int UART_RX = 0;    // receives from Pico GP8
const int UART_TX = 1;    // sends to Pico GP9
const int LED_PIN = 8;    // onboard LED (on when LOW)
const int MAX_APS = 10;

String lineUsb;           // line typed in the serial monitor
String linePico;          // line received from the Pico
unsigned long lastBeat = 0;

// Run a scan and send the result to "out" (Serial1 = Pico, Serial = USB)
void doScan(Stream &out) {
  unsigned long t0 = millis();
  int n = WiFi.scanNetworks(false, false, true, 250);   // passive scan, listens 150 ms per channel
  unsigned long took = millis() - t0;

  if (n < 0) {
    out.println("ERR,SCAN_FAILED");
    return;
  }
  int count = (n < MAX_APS) ? n : MAX_APS;   // results are strongest first
  for (int i = 0; i < count; i++) {
    String ssid = WiFi.SSID(i);
    ssid.replace(",", " ");                  // commas would break the format
    out.println("AP," + ssid + "," + String(WiFi.RSSI(i)) + "," + String(WiFi.channel(i)));
  }
  out.println("END," + String(count));
  WiFi.scanDelete();

  Serial.println("[scan took " + String(took) + " ms, found " + String(n) + "]");
}

void handleCommand(String cmd, Stream &reply) {
  cmd.trim();
  if (cmd.length() == 0) return;
  Serial.println("Command: " + cmd);
  if (cmd == "PING")      reply.println("PONG");
  else if (cmd == "SCAN") doScan(reply);
  else                    reply.println("ERR,UNKNOWN");
}

// Collect characters until newline, then run the command
void readLines(Stream &in, String &buf, Stream &reply) {
  while (in.available()) {
    char c = in.read();
    if (c == '\n') { handleCommand(buf, reply); buf = ""; }
    else           { buf += c; }
  }
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, UART_RX, UART_TX);
  pinMode(LED_PIN, OUTPUT);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.println("ESP32 ready. Type PING or SCAN.");
}

void loop() {
  if (millis() - lastBeat >= 1000) {
    lastBeat = millis();
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }
  readLines(Serial,  lineUsb,  Serial);    // typed in the monitor, reply to the monitor
  readLines(Serial1, linePico, Serial1);   // from the Pico, reply to the Pico
}