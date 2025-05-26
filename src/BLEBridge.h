#pragma once

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <Preferences.h>
#include "esp_task_wdt.h"

#define FIRMWARE_VERSION "BLE_BRIDGE_V1.0.0"

class BLEBridge {
public:
    BLEBridge(int rxPin, int txPin);

    void begin();
    void loop();
    
    static bool deviceConnected;
    static bool oldDeviceConnected;
    static bool isShutDown;

private:
    String deviceName;
    String serviceUUID;
    String characteristicUUID_RX;
    String characteristicUUID_TX;


    // NimBLEServer* pServer = nullptr;
    // NimBLECharacteristic* pTxCharacteristic = nullptr;
    // NimBLECharacteristic* pRxCharacteristic = nullptr;

    NimBLEServer *pServer;
    NimBLECharacteristic *pTxCharacteristic;
    NimBLECharacteristic *pRxCharacteristic;

    // NimBLEAdvertising* pAdvertising = nullptr;
    // bool deviceConnected = false;

    int rxPin;
    int txPin;

    Preferences preferences;

    void setupBLE();
    void processCommand(const String& input);
    void checkSerialCommands();
    void keepAdvertising();

    void setupWatchdog();
    void feedWatchdog();

    void loadConfig();
    void saveConfig(String name, String svc, String rx, String tx);

    // class ServerCallbacks;
    // class RxCallback;
};
