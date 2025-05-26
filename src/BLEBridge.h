#pragma once

#include <Arduino.h>
#include <NimBLEDevice.h>
#include "esp_task_wdt.h"

#define FIRMWARE_VERSION "BLE_BRIDGE_V1.0.0"

class BLEBridge {
public:
    BLEBridge(int rxPin, int txPin);

    void begin();
    void loop();

private:
    String deviceName = "ESP32-BLE";
    String serviceUUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
    String characteristicUUID_RX = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
    String characteristicUUID_TX = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";

    NimBLEServer* pServer = nullptr;
    NimBLECharacteristic* pTxCharacteristic = nullptr;
    NimBLECharacteristic* pRxCharacteristic = nullptr;
    NimBLEAdvertising* pAdvertising = nullptr;
    bool deviceConnected = false;

    int rxPin;
    int txPin;

    void setupBLE();
    void processCommand(const String& input);
    void checkSerialCommands();
    void keepAdvertising();
    
    void setupWatchdog();
    void feedWatchdog();

    class ServerCallbacks;
    class RxCallback;
};