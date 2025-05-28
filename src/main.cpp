#include <Arduino.h>
#include "BLEBridge.h"

#define RXD2 4
#define TXD2 16

BLEBridge bleBridge(RXD2, TXD2);

void setup() {
    Serial.begin(115200);
    Serial.println("Starting BLE Bridge");
    bleBridge.begin();
}

unsigned long timerSend = 0;

void loop() {
    bleBridge.loop();

    if(millis() - timerSend > 5000) {
        timerSend = millis();
        Serial2.println("STATUS:LOOP_RUNNING");
    }
}