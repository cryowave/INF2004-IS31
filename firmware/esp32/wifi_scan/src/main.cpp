#include <Arduino.h>

// ESP32 pins wired to the Pico (see the pin table)
const int UART_RX = 0;   // receives from Pico GP8
const int UART_TX = 1;   // sends to Pico GP9
const int LED_PIN = 8;   // onboard LED (on when LOW)

String line;
unsigned long lastBeat = 0;

void setup() {
  Serial.begin(115200);                                 // USB, for your laptop
  Serial1.begin(115200, SERIAL_8N1, UART_RX, UART_TX);  // UART to the Pico
  pinMode(LED_PIN, OUTPUT);
  Serial.println("ESP32 ready");
}

void loop() {
  // Once a second: print "alive" and blink the LED
  if (millis() - lastBeat >= 1000) {
    lastBeat = millis();
    Serial.println("alive");
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }

  // Read a line from the Pico and answer it
  while (Serial1.available()) {
    char c = Serial1.read();
    if (c == '\n') {
      line.trim();
      Serial.println("From Pico: " + line);
      if (line == "PING") Serial1.println("PONG");
      else                Serial1.println("ERR,UNKNOWN");
      line = "";
    } else {
      line += c;
    }
  }
}