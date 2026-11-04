#include <Arduino.h>
#include <WiFi.h>

const int UART_RX = 0;   // from Pico
const int UART_TX = 1;   // to Pico
const int LED_PIN = 8;
const int MAX_APS = 10;

bool scanning = false;           // is a scan running right now?
unsigned long scanStartMs = 0;
Stream *scanReplyTo = nullptr;   // who asked for the scan (Pico or USB)

String usbBuf, picoBuf;

void sendResults(Stream &out, int n) {
  int count = n < MAX_APS ? n : MAX_APS;
  for (int i = 0; i < count; i++) {
    String ssid = WiFi.SSID(i);
    ssid.replace(',', ' ');
    out.print("AP,");
    out.print(ssid);
    out.print(",");
    out.print(WiFi.RSSI(i));
    out.print(",");
    out.println(WiFi.channel(i));
  }
  out.print("END,");
  out.println(count);
}

// Start a scan in the background (does NOT freeze the ESP32)
void startScan(Stream &out) {
  if (scanning) {
    out.println("ERR,BUSY");
    return;
  }
  WiFi.scanNetworks(true, false, true, 250);  // async, passive, 250 ms/channel
  scanning = true;
  scanStartMs = millis();
  scanReplyTo = &out;
}

// Called every loop: has the background scan finished?
void checkScan() {
  if (!scanning) return;
  int n = WiFi.scanComplete();
  if (n == WIFI_SCAN_RUNNING) return;       // still scanning
  scanning = false;
  unsigned long took = millis() - scanStartMs;
  if (n < 0) {
    scanReplyTo->println("ERR,SCAN_FAILED");
  } else {
    sendResults(*scanReplyTo, n);
  }
  Serial.printf("[scan took %lu ms, found %d]\n", took, n);
  WiFi.scanDelete();
}

void handleCommand(String cmd, Stream &out) {
  cmd.trim();
  if (cmd.length() == 0) return;            // ignore empty lines
  if (cmd == "PING")      out.println("PONG");
  else if (cmd == "SCAN") startScan(out);
  else                    out.println("ERR,UNKNOWN");
}

void readLines(Stream &in, String &buf) {
  while (in.available()) {
    char c = in.read();
    if (c == '\n') { handleCommand(buf, in); buf = ""; }
    else if (c != '\r') buf += c;
  }
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, UART_RX, UART_TX);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  Serial1.println("READY");                 // tell the Pico we just started
  Serial.println("ESP32 ready. Type PING or SCAN.");
}

void loop() {
  readLines(Serial, usbBuf);
  readLines(Serial1, picoBuf);
  checkScan();

  static unsigned long lastBlink = 0;
  if (millis() - lastBlink >= 1000) {
    lastBlink = millis();
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }
}