#include "BLEBridge.h"
#include <Arduino.h>

bool BLEBridge::deviceConnected = false;
bool BLEBridge::oldDeviceConnected = false;
bool BLEBridge::isShutDown = false;

BLEBridge::BLEBridge(int rxPin, int txPin) : rxPin(rxPin), txPin(txPin) {}

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

void BLEBridge::setupWatchdog() {
    const int WDT_TIMEOUT = 8;
    esp_task_wdt_init(WDT_TIMEOUT, true);
    esp_task_wdt_add(NULL);
}

void BLEBridge::feedWatchdog() {
    esp_task_wdt_reset();
}

void BLEBridge::begin() {
    Serial2.begin(9600, SERIAL_8N1, rxPin, txPin);

    loadConfig();
    delay(100);


    NimBLEDevice::init(deviceName.c_str());
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);

    initServer();
    initServices();
    startAdvertising();

    setupWatchdog();
}


void BLEBridge::initServer() {
    Serial.println("initServer()");
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());
}

void BLEBridge::initServices() {
    Serial.println("initServices()");

    NimBLEService* pService = pServer->createService(serviceUUID.c_str());

    pTxCharacteristic = pService->createCharacteristic(
        characteristicUUID_TX.c_str(),
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::INDICATE // Adicionado `READ` e `INDICATE`
    
    );

    pRxCharacteristic = pService->createCharacteristic(
        characteristicUUID_RX.c_str(),
        NIMBLE_PROPERTY::WRITE
    );

    pRxCharacteristic->setCallbacks(new CharacteristicCallbacks());
    pService->start();
}

void BLEBridge::startAdvertising() {
    Serial.println("startAdvertising()");

    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(serviceUUID.c_str());
    pAdvertising->setScanResponse(true);
    pAdvertising->start();
    Serial.println("OK:BLE_ADVERTISING_STARTED");
    Serial2.println("OK:BLE_ADVERTISING_STARTED");
}


void BLEBridge::stopAdvertising() {
    NimBLEDevice::getAdvertising()->stop();
    Serial.println("OK:BLE_ADVERTISING_STOPPED");
    delay(10);
}

bool BLEBridge::isConnected() {
    return deviceConnected;
}

void BLEBridge::notifyClients(String data) {
    if (deviceConnected && pTxCharacteristic) {
        pTxCharacteristic->setValue(std::string(data.c_str()));

        if (pTxCharacteristic->getSubscribedCount() > 0) {  // Verifica se há clientes inscritos
            pTxCharacteristic->notify();
            Serial.println("SEND DATA OVER BLE -> " + data);
        } else {
            Serial.println("{NimBLEServerApp} -> Nenhum cliente inscrito para notificações.");
        }
    }
}


void BLEBridge::removeServices() {
    if (!pServer) {
        Serial.println("BLE -> Nenhum servidor BLE ativo para remover serviços.");
        return;
    }

    Serial.println("BLE -> Removendo serviços BLE...");

    NimBLEService* pService = pServer->getServiceByUUID(serviceUUID.c_str());
    if (pService) {
        Serial.println("BLE -> Parando serviço BLE...");

        Serial.println("BLE -> Removendo características...");
        pService->removeCharacteristic(pTxCharacteristic);
        pService->removeCharacteristic(pRxCharacteristic);

        Serial.println("BLE -> Removendo serviço...");
        pServer->removeService(pService);
    } else {
        Serial.println("BLE -> Serviço não encontrado.");
    }

    pTxCharacteristic = nullptr;
    pRxCharacteristic = nullptr;

    Serial.println("BLE -> Serviços BLE removidos com sucesso.");
    delay(10);
}

void BLEBridge::shutdown(){
    BLEBridge::isShutDown = true;
    Serial.println("{NimBLEServerApp} -> Shutting down BLE");

    stopAdvertising();
    removeServices();
    NimBLEDevice::deinit();
    delay(100);

    Serial.println("{NimBLEServerApp} -> BLE completely disabled");
}



void BLEBridge::processCommand(const String& input) {
    if (input.startsWith("CONFIG:")) {
        String params = input.substring(7);

        int nameIndex = params.indexOf("NAME=");
        int svcIndex = params.indexOf("SVC=");
        int rxIndex = params.indexOf("RX=");
        int txIndex = params.indexOf("TX=");

        if (nameIndex == -1 || svcIndex == -1 || rxIndex == -1 || txIndex == -1) {
            return;
        }

        String name = params.substring(nameIndex + 5, params.indexOf(';', nameIndex));
        String svc = params.substring(svcIndex + 4, params.indexOf(';', svcIndex));
        String rx = params.substring(rxIndex + 3, params.indexOf(';', rxIndex));
        String tx = params.substring(txIndex + 3);

        saveConfig(name, svc, rx, tx);
        delay(100);
        Serial2.println("OK:CONFIG_SAVED_RESTARTING");
        delay(100);
        Serial.println("OK:CONFIG_SAVED_RESTARTING");
        delay(100);

        ESP.restart();
    }

    else if (input == "VERSION") {
        Serial2.println("OK:FIRMWARE:" FIRMWARE_VERSION);
    }

    else {
        if (deviceConnected) {
            Serial.println("[BLEBridge::processCommand()]: SEND over BLE -> " + input);
            notifyClients(input);
        } else {
            // Serial2.println("ERROR:BLE_NOT_CONNECTED");
        }
    }
}

static String input = "";
static bool receiving = false;

void BLEBridge::checkSerialCommands() {
    if (Serial2.available() > 0) {
        char c = Serial2.read();

        if (c == '[') {
            receiving = true;
            input = "";
        } else if (c == ']' && receiving) {
            receiving = false;
            input.trim();

            Serial.println("[BLEBridge::checkSerialCommands()]: RECEIVED over SERIAL 2 → " + input);

            if (input.length() > 0) {
                processCommand(input);
            }

            input = "";
        } else if (receiving) {
            input += c;

            if (input.length() > 256) {
                Serial.println("ERROR:INPUT_OVERFLOW");
                input = "";
                receiving = false;
            }
        }
    }
}

void BLEBridge::loop() {
    checkSerialCommands();
    feedWatchdog();
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
    deviceName = preferences.getString("name", "ESP32 BLE");
    serviceUUID = preferences.getString("svc", "6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
    characteristicUUID_RX = preferences.getString("rx", "6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
    characteristicUUID_TX = preferences.getString("tx", "6E400003-B5A3-F393-E0A9-E50E24DCCA9E");
    preferences.end();

    Serial.printf("[BLEBridge::loadConfig()]: Name=%s, ServiceUUID=%s, RXUUID=%s, TXUUID=%s\n", 
                   deviceName.c_str(), serviceUUID.c_str(), characteristicUUID_RX.c_str(), characteristicUUID_TX.c_str());
}
