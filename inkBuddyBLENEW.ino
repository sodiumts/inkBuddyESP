#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "esp_ota_ops.h"

std::string FWVersion = "v0.0.1";
std::string HWVersion = "v0.1";

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
BLECharacteristic* pCharacteristic1 = NULL;
BLECharacteristic* pCharacteristicOTA = NULL;
BLECharacteristic* pCharacteristicFW = NULL;
BLECharacteristic* pCharacteristicHW = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;
bool updateFlag = false;
esp_ota_handle_t otaHandler = 0;

#define SERVICE_UUID_GENERIC    "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID0    "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC_UUID1    "b6a5eeef-8f30-46dd-8689-3b874c0fda71"

#define SERVICE_UUID_OTA "ddd633a2-1d8b-498f-af91-5406399e87fd"
#define CHARACTERISTIC_UUID_RECEIVE "5b277b7f-f2e2-438c-9041-a7cda588269c"

#define SERVICE_UUID_DEVICE         "0000180a-0000-1000-8000-00805f9b34fb" // UART service UUID
#define CHARACTERISTIC_UUID_FW      "00002a26-0000-1000-8000-00805f9b34fb"
#define CHARACTERISTIC_UUID_HW      "00002a27-0000-1000-8000-00805f9b34fb"

class MyServerCallBacks: public BLEServerCallbacks{
  void onConnect(BLEServer* pServer){
    deviceConnected = true;
  };
  void onDisconnect(BLEServer* pServer){
    deviceConnected = false;
  }
};

class SecondCharacteristicCallbacks: public BLECharacteristicCallbacks{
  void onWrite(BLECharacteristic *pCharacteristic){
    Serial.println("Data received");
    uint8_t* pData;
    std::string value = pCharacteristic->getValue();
    int len = value.length();
    pData = pCharacteristic->getData();
    for(int i=0;i<len;i++){
      Serial.println(pData[i]);
    }
    delay(5000);
    Serial.println("Send notify");
    pCharacteristic->notify();
  }
};

class OTACallbacks: public BLECharacteristicCallbacks{
  void onWrite(BLECharacteristic *pCharacteristic){
    std::string rData = pCharacteristic->getValue();
    if(!updateFlag){
      Serial.println("Begin OTA");
      esp_ota_begin(esp_ota_get_next_update_partition(NULL),OTA_SIZE_UNKNOWN,&otaHandler);
      updateFlag = true;
    }
    if(rData.length()>0){
      esp_ota_write(otaHandler,rData.c_str(),rData.length());
      if(rData.length() != 512){
        esp_ota_end(otaHandler);
        Serial.println("End OTA");
        if(ESP_OK == esp_ota_set_boot_partition(esp_ota_get_next_update_partition(NULL))){
          delay(2000);
          esp_restart();
        }
        else{
          Serial.println("Upload Error");
        }
      }
    }
    uint8_t tData[5] = {1,2,3,4,5};
    pCharacteristic->setValue((uint8_t*)tData,5);
    pCharacteristic->notify();
  }
};

void setup(){
  Serial.begin(115200);
  BLEDevice::init("inkBuddy");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallBacks());

  BLEService *pService = pServer -> createService(SERVICE_UUID_GENERIC);
  BLEService *pServiceOTA = pServer ->createService(SERVICE_UUID_OTA);
  BLEService *pServiceDevice = pServer ->createService(SERVICE_UUID_DEVICE);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID0,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE_NR  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
  );

  pCharacteristic1 = pService->createCharacteristic(
                      CHARACTERISTIC_UUID1,
                      BLECharacteristic::PROPERTY_WRITE
  );
  pCharacteristicOTA = pServiceOTA->createCharacteristic(
                      CHARACTERISTIC_UUID_RECEIVE,
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY
  );
  pCharacteristicFW = pServiceDevice->createCharacteristic(
                      CHARACTERISTIC_UUID_FW,
                      BLECharacteristic::PROPERTY_READ
  );
  pCharacteristicHW = pServiceDevice->createCharacteristic(
                      CHARACTERISTIC_UUID_HW,
                      BLECharacteristic::PROPERTY_READ
  );

  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic1->setCallbacks(new SecondCharacteristicCallbacks);

  pCharacteristicOTA->addDescriptor(new BLE2902());
  pCharacteristicOTA->setCallbacks(new OTACallbacks());

  pServiceDevice->start();
  pService->start();
  pServiceOTA->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID_GENERIC);
  pAdvertising->addServiceUUID(SERVICE_UUID_OTA);
  pAdvertising->addServiceUUID(SERVICE_UUID_DEVICE);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();

  // pCharacteristicHW->setValue("v0.1");
  // pCharacteristicFW->setValue("v0.0.1");
  pCharacteristicHW->setValue(HWVersion);
  pCharacteristicFW->setValue(FWVersion);
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
    // notify changed value
    if (deviceConnected) {
        pCharacteristic->setValue((uint8_t*)&value, 4);
        pCharacteristic->notify();
        value++;
        delay(3); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
}