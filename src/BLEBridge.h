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

    void shutdown();
    void notifyClients(String data);
    void startAdvertising();
    void stopAdvertising();
    bool isConnected();
    
    static bool deviceConnected;
    static bool oldDeviceConnected;
    static bool isShutDown;

private:

    NimBLEServer *pServer;
    NimBLECharacteristic *pTxCharacteristic;
    NimBLECharacteristic *pRxCharacteristic; 

    void initServer();
    void initServices();
    void removeServices();

    String deviceName;
    String serviceUUID;
    String characteristicUUID_RX;
    String characteristicUUID_TX;

    int rxPin;
    int txPin;

    Preferences preferences;

    void processCommand(const String& input);
    void checkSerialCommands();
    void keepAdvertising();

    void setupWatchdog();
    void feedWatchdog();

    void loadConfig();
    void saveConfig(String name, String svc, String rx, String tx);
};
