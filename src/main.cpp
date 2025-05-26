#include <Arduino.h>
#include "BLEBridge.h"

#define RXD2 16
#define TXD2 17

BLEBridge bleBridge(RXD2, TXD2);

void setup() {
    Serial.begin(115200);
    Serial.println("Starting BLE Bridge");
    bleBridge.begin();
}

void loop() {
    bleBridge.loop();
}