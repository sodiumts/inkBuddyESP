#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "esp_ota_ops.h"

float _vs[101];
void initPercArr(){
    _vs[0] = 3.200; 
    _vs[1] = 3.250; _vs[2] = 3.300; _vs[3] = 3.350; _vs[4] = 3.400; _vs[5] = 3.450;
    _vs[6] = 3.500; _vs[7] = 3.550; _vs[8] = 3.600; _vs[9] = 3.650; _vs[10] = 3.700;
    _vs[11] = 3.703; _vs[12] = 3.706; _vs[13] = 3.710; _vs[14] = 3.713; _vs[15] = 3.716;
    _vs[16] = 3.719; _vs[17] = 3.723; _vs[18] = 3.726; _vs[19] = 3.729; _vs[20] = 3.732;
    _vs[21] = 3.735; _vs[22] = 3.739; _vs[23] = 3.742; _vs[24] = 3.745; _vs[25] = 3.748;
    _vs[26] = 3.752; _vs[27] = 3.755; _vs[28] = 3.758; _vs[29] = 3.761; _vs[30] = 3.765;
    _vs[31] = 3.768; _vs[32] = 3.771; _vs[33] = 3.774; _vs[34] = 3.777; _vs[35] = 3.781;
    _vs[36] = 3.784; _vs[37] = 3.787; _vs[38] = 3.790; _vs[39] = 3.794; _vs[40] = 3.797;
    _vs[41] = 3.800; _vs[42] = 3.805; _vs[43] = 3.811; _vs[44] = 3.816; _vs[45] = 3.821;
    _vs[46] = 3.826; _vs[47] = 3.832; _vs[48] = 3.837; _vs[49] = 3.842; _vs[50] = 3.847;
    _vs[51] = 3.853; _vs[52] = 3.858; _vs[53] = 3.863; _vs[54] = 3.868; _vs[55] = 3.874;
    _vs[56] = 3.879; _vs[57] = 3.884; _vs[58] = 3.889; _vs[59] = 3.895; _vs[60] = 3.900;
    _vs[61] = 3.906; _vs[62] = 3.911; _vs[63] = 3.917; _vs[64] = 3.922; _vs[65] = 3.928;
    _vs[66] = 3.933; _vs[67] = 3.939; _vs[68] = 3.944; _vs[69] = 3.950; _vs[70] = 3.956;
    _vs[71] = 3.961; _vs[72] = 3.967; _vs[73] = 3.972; _vs[74] = 3.978; _vs[75] = 3.983;
    _vs[76] = 3.989; _vs[77] = 3.994; _vs[78] = 4.000; _vs[79] = 4.008; _vs[80] = 4.015;
    _vs[81] = 4.023; _vs[82] = 4.031; _vs[83] = 4.038; _vs[84] = 4.046; _vs[85] = 4.054;
    _vs[86] = 4.062; _vs[87] = 4.069; _vs[88] = 4.077; _vs[89] = 4.085; _vs[90] = 4.092;
    _vs[91] = 4.100; _vs[92] = 4.111; _vs[93] = 4.122; _vs[94] = 4.133; _vs[95] = 4.144;
    _vs[96] = 4.156; _vs[97] = 4.167; _vs[98] = 4.178; _vs[99] = 4.189; _vs[100] = 4.200;
};

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

int xPosLast = 0;
int yPosLast = 0;
int cursorXLast = 0;
int textSizeLast = 0;
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

#define ADC_PIN 35
#define ADC_EN 34
long startTime = 0;
int vref = 1100;
//Code for calculating battery percentage
int checkBatLVL(double volt){
  int idx = 50;
  int prev = 0;
  int half = 0;
  if (volt >= 4.2){
    return 100;
  }
  if (volt <= 3.2){
    return 0;
  }
  while(true){
    half = abs(idx - prev) / 2;
    prev = idx;
    if(volt >= _vs[idx]){
      idx = idx + half;
    }else{
      idx = idx - half;
    }
    if (prev == idx){
      break;
    }
  }
  return idx;
}

void Voltage(){
  static uint64_t timeStamp = 0;
    timeStamp = millis();
    uint16_t v = analogRead(ADC_PIN);
    double battery_voltage =((double)v /4095.0) *2.0 *3.3 *(vref/1000.0);

    String a = "V: " +String(battery_voltage)+ "V";
    Serial.println(a);
    Serial.println("before");
    int perc = checkBatLVL((double)battery_voltage);
    Serial.print(perc);
    
    pCharacteristic->setValue(std::to_string(perc));
    pCharacteristic->notify();

    display.fillRect(0,0,GxEPD_WIDTH,GxEPD_HEIGHT,GxEPD_WHITE);
    // display.updateWindow(0,0,100,80,true);
    display.setCursor(0, 0);
    display.print(perc);

    display.println(a.c_str());
    display.updateWindow(0,0,180,80,true);
}

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
    int textSize = 2;
    // Serial.print(xPos); Serial.print(yPos); Serial.print("\n");

    uint8_t mainData[len-1] = {};
    // Serial.println(len-4);
    for(int i=4;i<len;i++){
      // Serial.println(pData[i]);
      mainData[i-4] = pData[i];
    }

    // String str = (char*)mainData;
    Serial.println((char*)mainData);
    display.setRotation(1);
    display.setCursor(xPos,yPos);
    display.setTextSize(textSize);
    display.setTextColor(GxEPD_BLACK);

    if(textSizeLast!=0){
      display.fillRect(xPosLast,yPosLast,cursorXLast,textSizeLast*10,GxEPD_WHITE);
      display.updateWindow(xPosLast,yPosLast,cursorXLast,textSizeLast*10,true);
    }

    display.print((char*)mainData);
    int cursorX = display.getCursorX();
    // int cursorY = display.getCursorY();
    // Serial.println(cursorX);
    // Serial.println(cursorY);
    display.updateWindow(xPos,yPos,cursorX,textSize*10,true);

    xPosLast = xPos;
    yPosLast = yPos;
    cursorXLast = cursorX;
    textSizeLast = textSize;
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
  // display.fillRect(xPosLast,yPosLast,cursorXLast,textSizeLast*10,GxEPD_WHITE);
  SPI.begin(EPD_SCLK, EPD_MISO, EPD_MOSI);
  display.init();
  display.fillScreen(GxEPD_WHITE);
  display.update();
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
                      BLECharacteristic::PROPERTY_NOTIFY 
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
  display.setRotation(1);
  display.setCursor(0,0);
  display.setTextSize(2);
  display.setTextColor(GxEPD_BLACK);
}
int counter = 0;
void loop() {
    // disconnecting
    
    if (deviceConnected){
      if(millis()-startTime >1000){
          startTime = millis();
          Voltage();
          counter++;
        }
    }
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        
        oldDeviceConnected = deviceConnected;
    }
    
}