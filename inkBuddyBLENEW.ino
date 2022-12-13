#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "esp_ota_ops.h"

//For firmware and hardware versions
std::string FWVersion = "v0.0.1";
std::string HWVersion = "v0.1";
//For initializing the display requirements
#define LILYGO_T5_V213

#include <boards.h>
#include <GxEPD.h>
#include <GxDEPG0213BN/GxDEPG0213BN.h>  
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

GxIO_Class io(SPI,  EPD_CS, EPD_DC,  EPD_RSET);
GxEPD_Class display(io, EPD_RSET, EPD_BUSY);


//            for initializing BLE.        //
BLEServer* pServer = NULL;
//characteristic definitions

//General service characteristics
BLECharacteristic* pCharacteristic = NULL;
BLECharacteristic* pCharacteristic1 = NULL;
//OTA characteristics
BLECharacteristic* pCharacteristicOTA = NULL;
//Device info characteristics
BLECharacteristic* pCharacteristicFW = NULL;
BLECharacteristic* pCharacteristicHW = NULL;


bool deviceConnected = false;
bool oldDeviceConnected = false;
bool updateFlag = false;
esp_ota_handle_t otaHandler = 0;


//UUIDs for services and characteristics

//General service uuids
#define SERVICE_UUID_GENERIC        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID0        "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC_UUID1        "b6a5eeef-8f30-46dd-8689-3b874c0fda71"
//OTA service uuids
#define SERVICE_UUID_OTA            "0000FE59-0000-1000-8000-00805f9b34fb"
#define CHARACTERISTIC_UUID_RECEIVE "8ec90003-f315-4f60-9fb8-838830daea50"
//Device info service uuids
#define SERVICE_UUID_DEVICE         "0000180a-0000-1000-8000-00805f9b34fb" 
#define CHARACTERISTIC_UUID_FW      "00002a26-0000-1000-8000-00805f9b34fb"
#define CHARACTERISTIC_UUID_HW      "00002a27-0000-1000-8000-00805f9b34fb"

//Main callbacks for BLE server
class MyServerCallBacks: public BLEServerCallbacks{
  void onConnect(BLEServer* pServer){
    deviceConnected = true;
  };
  void onDisconnect(BLEServer* pServer){
    deviceConnected = false;
  }
};

//Callbacks for general service characteristic
class SecondCharacteristicCallbacks: public BLECharacteristicCallbacks{
  void onWrite(BLECharacteristic *pCharacteristic){
    Serial.println("Data received");
    uint8_t* pData = pCharacteristic->getData();
    std::string pValue = pCharacteristic->getValue();
    int len = pValue.length();
    // Serial.println(len);
    int xPos = pData[0];
    int yPos = pData[1];
    int width = pData[2];
    int height = pData[3];
    int textSize = 1;
    // Serial.print(xPos); Serial.print(yPos); Serial.print("\n");

    uint8_t mainData[len-1] = {};
    // Serial.println(len-4);
    for(int i=4;i<len;i++){
      // Serial.println(pData[i]);
      mainData[i-4] = pData[i];
    }

    // String str = (char*)mainData;
    Serial.println((char*)mainData);
    // display.setRotation(0);
    // display.setCursor(xPos,yPos);
    // display.setTextSize(textSize);
    // display.println((char *) mainData);
    // // int cursorX = display.getCursorX();
    // display.updateWindow(0,0,GxEPD_WIDTH,GxEPD_HEIGHT,true);
    Serial.println("Send notify");
    pCharacteristic->notify();
  }
};

//Callbacks for OTA updates
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
  Serial.println("Start setup");
  //Initializing the EPD display
  SPI.begin(EPD_SCLK, EPD_MISO, EPD_MOSI);
  display.init();
  display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT,false);


  //Starting BLE server
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
  pAdvertising->setMinPreferred(0x0); 
  BLEDevice::startAdvertising();

  pCharacteristicHW->setValue(HWVersion);
  pCharacteristicFW->setValue(FWVersion);
  Serial.println("Finished startup");
}

void loop() {
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