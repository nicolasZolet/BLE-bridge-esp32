#include "BLEBridge.h"
#include <Arduino.h>


bool BLEBridge::deviceConnected = false;
bool BLEBridge::oldDeviceConnected = false;
bool BLEBridge::isShutDown = false;

BLEBridge::BLEBridge(int rxPin, int txPin) : rxPin(rxPin), txPin(txPin) {}

// class BLEBridge::ServerCallbacks : public NimBLEServerCallbacks {
//     BLEBridge* parent;
// public:
//     ServerCallbacks(BLEBridge* parent) : parent(parent) {}
//     void onConnect(NimBLEServer* pServer) override {
//         parent->deviceConnected = true;
//         Serial2.println("STATUS:BLE_CONNECTED");
//     }
//     void onDisconnect(NimBLEServer* pServer) override {
//         parent->deviceConnected = false;
//         Serial2.println("STATUS:BLE_DISCONNECTED");
//     }
// };

// /* Server Callbacks */
class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) override {
        BLEBridge::deviceConnected = true;
        Serial2.println("STATUS:BLE_CONNECTED");
    }

    void onDisconnect(NimBLEServer* pServer) override {
        BLEBridge::deviceConnected = false;
        Serial2.println("STATUS:BLE_DISCONNECTED");

        if(BLEBridge::isShutDown == false){
            NimBLEDevice::startAdvertising();
            pServer->getAdvertising()->start();
        }
    }
};



/* Characteristic Callbacks */
class CharacteristicCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic) override {
        std::string rxValue = pCharacteristic->getValue();
        String _rxValue = rxValue.c_str();
        
        Serial.println("[BLEBridge::RxCallback] RECEIVED over BLE and SEND over SERIAL2 -> " + _rxValue);
        Serial2.println(_rxValue);
    }
};

// class BLEBridge::RxCallback : public NimBLECharacteristicCallbacks {
//     BLEBridge* parent;
// public:
//     RxCallback(BLEBridge* parent) : parent(parent) {}
//     void onWrite(NimBLECharacteristic* pCharacteristic) override {

//         std::string rxValue = pCharacteristic->getValue();
//         if (!rxValue.empty()) {
//             String _rxValue = rxValue.c_str();

//             Serial.println("[BLEBridge::RxCallback] RECEIVED over BLE and SEND over SERIAL2 -> " + _rxValue);
//             Serial2.println(_rxValue);

//             // Serial2.write((uint8_t*)rxValue.data(), rxValue.length());
//         }
//     }
// };

void BLEBridge::begin() {
    Serial2.begin(9600, SERIAL_8N1, rxPin, txPin);
    Serial2.println("BLE_BRIDGE_STARTED");

    loadConfig();
    setupBLE();
    setupWatchdog();
}

void BLEBridge::setupBLE() {
    NimBLEDevice::init(deviceName.c_str());
    NimBLEDevice::setPower(ESP_PWR_LVL_N12); //manter?

    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    NimBLEService* pService = pServer->createService(serviceUUID.c_str());

    pTxCharacteristic = pService->createCharacteristic(
        characteristicUUID_TX.c_str(),
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::INDICATE
    );

    pRxCharacteristic = pService->createCharacteristic(
        characteristicUUID_RX.c_str(),
        NIMBLE_PROPERTY::WRITE
    );

    // pRxCharacteristic->setCallbacks(new RxCallback(this));
    pRxCharacteristic->setCallbacks(new CharacteristicCallbacks());
    pService->start();

    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(serviceUUID.c_str());
    pAdvertising->setScanResponse(true);
    pAdvertising->start();

    // pAdvertising = NimBLEDevice::getAdvertising();
    // pAdvertising->addServiceUUID(serviceUUID.c_str());
    // pAdvertising->setScanResponse(true);
    // pAdvertising->start();

    Serial2.println("OK:BLE_ADVERTISING_STARTED");
}

void BLEBridge::setupWatchdog() {
    const int WDT_TIMEOUT = 8;
    esp_task_wdt_init(WDT_TIMEOUT, true);
    esp_task_wdt_add(NULL);
}

void BLEBridge::feedWatchdog() {
    esp_task_wdt_reset();
}

void BLEBridge::processCommand(const String& input) {
    if (input.startsWith("CONFIG:")) {
        String params = input.substring(7);

        //CONFIG:NAME=Scheer Firetech;SVC=30e3d633-a01d-4115-90f5-0754c9c5891f;RX=c5b922f2-f58d-4026-beb3-cdf0c83a5a41;TX=db6da158-884e-4977-ace1-a3af822bae6d
        int nameIndex = params.indexOf("NAME=");
        int svcIndex = params.indexOf("SVC=");
        int rxIndex = params.indexOf("RX=");
        int txIndex = params.indexOf("TX=");

        if (nameIndex == -1 || svcIndex == -1 || rxIndex == -1 || txIndex == -1) {
            Serial2.println("ERROR:INVALID_CONFIG_FORMAT");
            return;
        }

        String name = params.substring(nameIndex + 5, params.indexOf(';', nameIndex));
        String svc = params.substring(svcIndex + 4, params.indexOf(';', svcIndex));
        String rx = params.substring(rxIndex + 3, params.indexOf(';', rxIndex));
        String tx = params.substring(txIndex + 3);

        saveConfig(name, svc, rx, tx);

        Serial2.println("OK:CONFIG_SAVED_RESTARTING");
        Serial.println("OK:CONFIG_SAVED_RESTARTING");
        delay(200);
        ESP.restart();
    }

    else if (input == "VERSION") {
        Serial2.println("OK:FIRMWARE:" FIRMWARE_VERSION);
    }

    else {
        if (deviceConnected) {
            Serial.println("[BLEBridge::processCommand()]: SEND over BLE -> " + input);
            pTxCharacteristic->setValue(input.c_str());
            pTxCharacteristic->notify();
        } else {
            Serial2.println("ERROR:BLE_NOT_CONNECTED");
        }
    }
}

static String input = "";
static bool receiving = false;

void BLEBridge::checkSerialCommands() {
    if (Serial2.available()) {
        char c = Serial2.read();

        if (c == '[') {
            receiving = true;
            input = "";
        } else if (c == ']' && receiving) {
            receiving = false;
            input.trim();

            if (input.length() > 0) {
                processCommand(input);
            }

            Serial.println("[BLEBridge::checkSerialCommands()]: RECEIVED over SERIAL 2 → " + input);
            input = "";
        } else if (receiving) {
            input += c;
            // Serial.println("Serial → BLE: " + input);

            if (input.length() > 256) {
                Serial.println("ERROR:INPUT_OVERFLOW");
                Serial2.println("ERROR:INPUT_OVERFLOW");
                input = "";
                receiving = false;
            }
        }
    }
}


// void BLEBridge::keepAdvertising() {
//     if (!pAdvertising->isAdvertising()) {
//         pAdvertising->start();
//         Serial2.println("WARNING:ADVERTISING_RESTARTED");
//     }
// }

void BLEBridge::loop() {
    checkSerialCommands();
    feedWatchdog();
    // keepAdvertising();
}

void BLEBridge::saveConfig(String name, String svc, String rx, String tx) {
    preferences.begin("BLEBridge", false);
    preferences.putString("name", name);
    preferences.putString("svc", svc);
    preferences.putString("rx", rx);
    preferences.putString("tx", tx);
    preferences.end();
}

void BLEBridge::loadConfig() {
    preferences.begin("BLEBridge", true);
    deviceName = preferences.getString("name", "ESP32-BLE");
    serviceUUID = preferences.getString("svc", "6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
    characteristicUUID_RX = preferences.getString("rx", "6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
    characteristicUUID_TX = preferences.getString("tx", "6E400003-B5A3-F393-E0A9-E50E24DCCA9E");
    preferences.end();

    Serial.printf("[BLEBridge::loadConfig()]: Name=%s, ServiceUUID=%s, RXUUID=%s, TXUUID=%s\n", 
                   deviceName.c_str(), serviceUUID.c_str(), characteristicUUID_RX.c_str(), characteristicUUID_TX.c_str());
}
