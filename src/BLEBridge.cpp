#include "BLEBridge.h"

BLEBridge::BLEBridge(int rxPin, int txPin) : rxPin(rxPin), txPin(txPin) {}

class BLEBridge::ServerCallbacks : public NimBLEServerCallbacks {
    BLEBridge* parent;
public:
    ServerCallbacks(BLEBridge* parent) : parent(parent) {}
    void onConnect(NimBLEServer* pServer) override {
        parent->deviceConnected = true;
        Serial.println("BLE Connected");
        Serial2.println("STATUS:BLE_CONNECTED");
    }
    void onDisconnect(NimBLEServer* pServer) override {
        parent->deviceConnected = false;
        Serial.println("BLE Disconnected");
        Serial2.println("STATUS:BLE_DISCONNECTED");
    }
};

class BLEBridge::RxCallback : public NimBLECharacteristicCallbacks {
    BLEBridge* parent;
public:
    RxCallback(BLEBridge* parent) : parent(parent) {}
    void onWrite(NimBLECharacteristic* pCharacteristic) override {
        std::string rxValue = pCharacteristic->getValue();
        if (!rxValue.empty()) {
            Serial2.write((uint8_t*)rxValue.data(), rxValue.length());
            Serial.print("BLE → Serial: ");
            Serial.println(rxValue.c_str());
        }
    }
};

void BLEBridge::begin() {
    Serial2.begin(115200, SERIAL_8N1, rxPin, txPin);
    Serial2.println("BLE_BRIDGE_STARTED");

    setupBLE();
    setupWatchdog();
}

void BLEBridge::setupBLE() {
    NimBLEDevice::deinit();
    NimBLEDevice::init(deviceName.c_str());
    NimBLEDevice::setPower(ESP_PWR_LVL_P3);

    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks(this));

    NimBLEService* pService = pServer->createService(serviceUUID.c_str());

    pTxCharacteristic = pService->createCharacteristic(
        characteristicUUID_TX.c_str(), 
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::INDICATE
    );

    pRxCharacteristic = pService->createCharacteristic(
        characteristicUUID_RX.c_str(), 
        NIMBLE_PROPERTY::WRITE
    );

    pRxCharacteristic->setCallbacks(new RxCallback(this));
    pService->start();

    pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(serviceUUID.c_str());
    pAdvertising->setScanResponse(true);
    pAdvertising->start();

    Serial.println("BLE Advertising started");
    Serial2.println("OK:BLE_ADVERTISING_STARTED");
}

void BLEBridge::setupWatchdog() {
    const int WDT_TIMEOUT = 8;
    esp_task_wdt_init(WDT_TIMEOUT, true);
    esp_task_wdt_add(NULL);
    Serial.println("Watchdog iniciado");
}

void BLEBridge::feedWatchdog() {
    esp_task_wdt_reset();
}

void BLEBridge::processCommand(const String& input) {
    if (input.startsWith("NAME:")) {
        deviceName = input.substring(5);
        setupBLE();
        Serial2.println("OK:NAME_UPDATED:" + deviceName);
    } else if (input.startsWith("SVC:")) {
        serviceUUID = input.substring(4);
        setupBLE();
        Serial2.println("OK:SERVICE_UUID_UPDATED:" + serviceUUID);
    } else if (input.startsWith("RX:")) {
        characteristicUUID_RX = input.substring(3);
        setupBLE();
        Serial2.println("OK:SERVICE_UUID_RX_UPDATED:" + characteristicUUID_RX);
    } else if (input.startsWith("TX:")) {
        characteristicUUID_TX = input.substring(3);
        setupBLE();
        Serial2.println("OK:SERVICE_UUID_TX_UPDATED:" + characteristicUUID_TX);
    } else if (input == "BOND:CLEAR") {
        NimBLEDevice::deleteAllBonds();
        Serial2.println("OK:BONDS_CLEARED");
        Serial.println("All bonded devices removed.");
    } else if (input == "RESET") {
        Serial2.println("OK:RESETTING");
        delay(100);
        ESP.restart();
    } else if (input == "VERSION") {
        Serial2.println("OK:FIRMWARE:" FIRMWARE_VERSION);
    } else {
        if (deviceConnected) {
            pTxCharacteristic->setValue(input.c_str());
            pTxCharacteristic->notify();
            Serial.print("Serial → BLE: ");
            Serial.println(input);
        } else {
            Serial2.println("ERROR:BLE_NOT_CONNECTED");
        }
    }
}

void BLEBridge::checkSerialCommands() {
    static String input = "";
    const int maxBytesPerLoop = 32;
    int bytesProcessed = 0;

    while (Serial2.available() && bytesProcessed < maxBytesPerLoop) {
        char c = Serial2.read();
        bytesProcessed++;

        if (c == '\n') {
            input.trim();
            if (input.length() > 0) {
                processCommand(input);
            }
            input = "";
        } else {
            input += c;

            // Proteção contra overflow:
            if (input.length() > 256) {
                Serial2.println("ERROR:INPUT_OVERFLOW");
                Serial.println("ERROR:INPUT_OVERFLOW");
                input = "";
                break; // Opcional: pode parar o processamento neste ciclo
            }
        }
    }
}

void BLEBridge::keepAdvertising() {
    if (!pAdvertising->isAdvertising()) {
        pAdvertising->start();
        Serial2.println("WARNING:ADVERTISING_RESTARTED");
    }
}

void BLEBridge::loop() {
    checkSerialCommands();
    feedWatchdog();
    keepAdvertising();
}