/*
  Set a baud rate to 115200bps over BLE

  Adding descriptor
  https://github.com/espressif/arduino-esp32/blob/master/libraries/BLE/src/BLECharacteristic.h
  https://gist.github.com/heiko-r/f284d95141871e12ca0164d9070d61b4
  Roughly working Characteristic descriptor: https://github.com/espressif/arduino-esp32/issues/1038

  Float to string: https://iotbyhvm.ooo/esp32-ble-tutorials/

  Custom UUIDs? https://www.bluetooth.com/specifications/assigned-numbers/

  ESP32 with chrome: https://github.com/kpatel122/ESP32-Web-Bluetooth-Terminal/blob/master/ESP32-BLE/ESP32-BLE.ino
*/
#include <string>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#include "ESP32_BLE_Config.h"



#define LED_BUILTIN 13
// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define kTargetServiceUUID  "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define kTargetServiceName  "ESP32 App"
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Property Creation and Management
//
// Dynamically create and manage properties
//--------------------------------------------------------------------------------------

// BLE Codes for our service
#define kBLEDescCharNameUUID            0x2901
#define kBLEDescSFEPropTypeUUID         0xA101
#define kBLEDescSFEPropRangeMinUUID     0xA110
#define kBLEDescSFEPropRangeMaxUUID     0xA111

// Property type codes - sent as a value of the char descriptor 
#define kSFEPropTypeBool       0x1
#define kSFEPropTypeInt        0x2
#define kSFEPropTypeRange      0x3
#define kSFEPropTypeText       0x4
#define kSFEPropTypeDate       0x5
#define kSFEPropTypeTime       0x6
#define kSFEPropTypeFloat       0x7


// Our Characteristic UUIDs - and yes, just made these up
#define kCharacteristicBaudUUID     "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define kCharacteristicEnabledUUID  "beb5483e-36e1-4688-b7f5-ea07361b26a9"
#define kCharacteristicMessageUUID  "beb5483e-36e1-4688-b7f5-ea07361b26aa"
#define kCharacteristicSampleUUID   "beb5483e-36e1-4688-b7f5-ea07311b260b"
#define kCharacteristicDateUUID     "beb5483e-36e1-4688-b7f5-ea07311b260c"
#define kCharacteristicTimeUUID     "beb5483e-36e1-4688-b7f5-ea07311b260d"
#define kCharacteristicOffsetUUID     "beb5483e-36e1-4688-b7f5-ea07311b260e"


// PROPERTY Data/Local Variables
uint32_t baudRate = 115200;

bool deviceEnabled = true;

// message prop
std::string strMessage("Welcome");

// sample rate prop (a range)
uint32_t sampleRate = 123;
const uint32_t sampleRateMin = 10;
const uint32_t sampleRateMax = 240;

// Date prop - date is a string "YYYY-MM-DD"
std::string strDate("2021-03-01");

// Time prop - date is a string "HH:MM" - 
std::string strTime("2:5"); // test value - will be parsed as 02:05

// A float property - "offset value"
float offsetValue = 4.124;

bool deviceConnected = false;
bool newConfig = true;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      Serial.println("Server Connect!");
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      Serial.println("Server Disconnect");
      deviceConnected = false;
    }
};

//4 bytes come in but they are little endian. Flip them around.
//Convert a std string to a int
int32_t stringToValue(std::string myString)
{
  int newValue = 0;
  for (int i = myString.length() ; i > 0 ; i--)
  {
    newValue <<= 8;
    newValue |= (myString[i - 1]);
  }

  return (newValue);
}

//---------------------------------------------------------------------------------
// Enabled Characterisitic 
//
// A Bool Characterisitic (property) example

void onEnabledUpdate(bool newValue){

    Serial.print("Update Enabled Value: "); 
    Serial.println(deviceEnabled);
    deviceEnabled = newValue;
    digitalWrite(LED_BUILTIN, (deviceEnabled ? HIGH : LOW));    
}
void setupEnabledCharacteristic(BLEService *pService){

    BLECharacteristic *pCharBaud;

    pCharBaud = pService->createCharacteristic(
                            kCharacteristicEnabledUUID,
                            BLECharacteristic::PROPERTY_READ  |
                            BLECharacteristic::PROPERTY_WRITE |
                            BLECharacteristic::PROPERTY_NOTIFY );

    // Set the value in the callbacks 
    pCharBaud->setCallbacks(new BoolPropertyValueCB(&deviceEnabled, onEnabledUpdate));

    // Add descriptor to that service (char user description)
    BLEDescriptor * pDesc = new BLEDescriptor((uint16_t)kBLEDescCharNameUUID);
    std::string descStr = "Device Enabled";
    pDesc->setValue(descStr);
    pCharBaud->addDescriptor(pDesc);

    pDesc = new BLEDescriptor((uint16_t)kBLEDescSFEPropTypeUUID);  // Property type
    uint8_t data=kSFEPropTypeBool;
    pDesc->setValue(&data,1);
    pCharBaud->addDescriptor(pDesc);

}


//---------------------------------------------------------------------------------
// Baud Rate Characterisitic 
//
// A Integer Characterisitic (property) example

void onBaudUpdate(int32_t newValue){

    Serial.print("Update Baud Value: "); 
    Serial.println(baudRate);
}
void setupBaudCharacteristic(BLEService *pService){

    BLECharacteristic *pCharBaud;

    pCharBaud = pService->createCharacteristic(
                            kCharacteristicBaudUUID,
                            BLECharacteristic::PROPERTY_READ  |
                            BLECharacteristic::PROPERTY_WRITE |
                            BLECharacteristic::PROPERTY_NOTIFY );

    // Set the value in the callbacks 
    pCharBaud->setCallbacks(new IntPropertyValueCB((int32_t*)&baudRate, onBaudUpdate));

    // Add descriptor to that service (char user description)
    BLEDescriptor * pDesc = new BLEDescriptor((uint16_t)kBLEDescCharNameUUID);
    std::string descStr = "Output Baud Value";
    pDesc->setValue(descStr);
    pCharBaud->addDescriptor(pDesc);

    pDesc = new BLEDescriptor((uint16_t)kBLEDescSFEPropTypeUUID);  // Property type
    uint8_t data=kSFEPropTypeInt;
    pDesc->setValue(&data,1);
    pCharBaud->addDescriptor(pDesc);

}
//---------------------------------------------------------------------------------
// Message Characterisitic 
//
// A String Characterisitic (property) example

void onMessageUpdate(std::string &  newValue){

    Serial.print("Update Message Value: "); 
    Serial.println(newValue.c_str());
}
void setupMessageCharacteristic(BLEService *pService){

    BLECharacteristic *pCharBaud;

    pCharBaud = pService->createCharacteristic(
                            kCharacteristicMessageUUID,
                            BLECharacteristic::PROPERTY_READ  |
                            BLECharacteristic::PROPERTY_WRITE |
                            BLECharacteristic::PROPERTY_NOTIFY );

    // Set the value in the callbacks 
    pCharBaud->setCallbacks(new TextPropertyValueCB(strMessage, onMessageUpdate));

    // Add descriptor to that service (char user description)
    BLEDescriptor * pDesc = new BLEDescriptor((uint16_t)kBLEDescCharNameUUID);
    std::string descStr = "Device Message";
    pDesc->setValue(descStr);
    pCharBaud->addDescriptor(pDesc);

    pDesc = new BLEDescriptor((uint16_t)kBLEDescSFEPropTypeUUID);  // Property type
    uint8_t data=kSFEPropTypeText;
    pDesc->setValue(&data,1);
    pCharBaud->addDescriptor(pDesc);

}


//---------------------------------------------------------------------------------
// Sample Rate Characterisitic 
//
// A Integer Range Characterisitic (property) example

void onSampleRateUpdate(int32_t newValue){

    Serial.print("Update Sample Value: "); 
    Serial.println(sampleRate);
}
void setupSampleRateCharacteristic(BLEService *pService){


    BLECharacteristic *pCharRate;

    pCharRate = pService->createCharacteristic(
                            kCharacteristicSampleUUID,
                            BLECharacteristic::PROPERTY_READ  |
                            BLECharacteristic::PROPERTY_WRITE |
                            BLECharacteristic::PROPERTY_NOTIFY );

    // Set the value in the callbacks - can just use the int callbacks
    pCharRate->setCallbacks(new IntPropertyValueCB((int32_t*)&sampleRate, onSampleRateUpdate));

    // Add descriptor to that service (char user description)
    BLEDescriptor * pDesc = new BLEDescriptor((uint16_t)kBLEDescCharNameUUID);
    std::string descStr = "Sample Rate (sec)";
    pDesc->setValue(descStr);
    pCharRate->addDescriptor(pDesc);

    pDesc = new BLEDescriptor((uint16_t)kBLEDescSFEPropTypeUUID);  // Property type
    uint8_t data=kSFEPropTypeRange;
    pDesc->setValue(&data,1);
    pCharRate->addDescriptor(pDesc);

    // set the range of the slider (min and max)

    pDesc = new BLEDescriptor((uint16_t)kBLEDescSFEPropRangeMinUUID);  // min
    pDesc->setValue((uint8_t*)&sampleRateMin, sizeof(sampleRateMin));
    pCharRate->addDescriptor(pDesc);

    pDesc = new BLEDescriptor((uint16_t)kBLEDescSFEPropRangeMaxUUID);  // max
    pDesc->setValue((uint8_t*)&sampleRateMax, sizeof(sampleRateMax));
    pCharRate->addDescriptor(pDesc);

}

//---------------------------------------------------------------------------------
// Date Characterisitic 
//
// A Date Characterisitic (property) example - format is a string "YYYY-MM-DD"

void onDateUpdate(std::string &  newValue){

    Serial.print("Update Date Value: "); 
    Serial.println(newValue.c_str());
}
void setupDateCharacteristic(BLEService *pService){

    BLECharacteristic *pCharDate;

    pCharDate = pService->createCharacteristic(
                            kCharacteristicDateUUID,
                            BLECharacteristic::PROPERTY_READ  |
                            BLECharacteristic::PROPERTY_WRITE |
                            BLECharacteristic::PROPERTY_NOTIFY );

    // Set the value in the callbacks 
    pCharDate->setCallbacks(new TextPropertyValueCB(strDate, onDateUpdate));

    // Add descriptor to that service (char user description)
    BLEDescriptor * pDesc = new BLEDescriptor((uint16_t)kBLEDescCharNameUUID);
    std::string descStr = "Start Date";
    pDesc->setValue(descStr);
    pCharDate->addDescriptor(pDesc);

    pDesc = new BLEDescriptor((uint16_t)kBLEDescSFEPropTypeUUID);  // Property type
    uint8_t data=kSFEPropTypeDate;
    pDesc->setValue(&data,1);
    pCharDate->addDescriptor(pDesc);

}
//---------------------------------------------------------------------------------
// Time Characterisitic 
//
// A Time Characterisitic (property) example - format is a string "HH:MM"

void onTimeUpdate(std::string &  newValue){

    Serial.print("Update Time Value: "); 
    Serial.println(newValue.c_str());
}
void setupTimeCharacteristic(BLEService *pService){

    BLECharacteristic *pCharTime;

    pCharTime = pService->createCharacteristic(
                            kCharacteristicTimeUUID,
                            BLECharacteristic::PROPERTY_READ  |
                            BLECharacteristic::PROPERTY_WRITE |
                            BLECharacteristic::PROPERTY_NOTIFY );

    // Set the value in the callbacks 
    pCharTime->setCallbacks(new TextPropertyValueCB(strTime, onTimeUpdate));

    // Add descriptor to that service (char user description)
    BLEDescriptor * pDesc = new BLEDescriptor((uint16_t)kBLEDescCharNameUUID);
    std::string descStr = "Start Time";
    pDesc->setValue(descStr);
    pCharTime->addDescriptor(pDesc);

    pDesc = new BLEDescriptor((uint16_t)kBLEDescSFEPropTypeUUID);  // Property type
    uint8_t data=kSFEPropTypeTime;
    pDesc->setValue(&data,1);
    pCharTime->addDescriptor(pDesc);

}

//---------------------------------------------------------------------------------
// FLoat Characterisitic 
//
// A float Characterisitic (property) example 

void onOffsetUpdate(float newValue){

    Serial.print("Update Offset(float) Value: "); 
    Serial.println(newValue);
}
void setupOffsetCharacteristic(BLEService *pService){

    BLECharacteristic *pCharOff;

    pCharOff = pService->createCharacteristic(
                            kCharacteristicOffsetUUID,
                            BLECharacteristic::PROPERTY_READ  |
                            BLECharacteristic::PROPERTY_WRITE |
                            BLECharacteristic::PROPERTY_NOTIFY );

    // Set the value in the callbacks 
    pCharOff->setCallbacks(new FloatPropertyValueCB(&offsetValue, onOffsetUpdate));

    // Add descriptor to that service (char user description)
    BLEDescriptor * pDesc = new BLEDescriptor((uint16_t)kBLEDescCharNameUUID);
    std::string descStr = "Offset Bias";
    pDesc->setValue(descStr);
    pCharOff->addDescriptor(pDesc);

    pDesc = new BLEDescriptor((uint16_t)kBLEDescSFEPropTypeUUID);  // Property type
    uint8_t data=kSFEPropTypeFloat;
    pDesc->setValue(&data,1);
    pCharOff->addDescriptor(pDesc);

}
//---------------------------------------------------------------------------------

void setup() {
    Serial.begin(115200);
    Serial.println("Starting BLE work!");

    BLEDevice::init(kTargetServiceName);

    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // >>> NOTE <<<
    // When creating a service. The default - when you just pass in a UUID,
    // only creates 15 handles. This isn't enough to support the 4 characteristics 
    // in this demo. Setting this to 20 allows for 4 Characteristics.
    // see: https://github.com/nkolban/ESP32_BLE_Arduino/blob/master/src/BLEServer.cpp
    BLEService *pService = pServer->createService(BLEUUID(kTargetServiceUUID), 35, 1);

    //Setup characterstics
    setupSampleRateCharacteristic(pService);
    setupBaudCharacteristic(pService);
    setupMessageCharacteristic(pService);
    setupEnabledCharacteristic(pService);
    setupDateCharacteristic(pService);
    setupTimeCharacteristic(pService); 
    setupOffsetCharacteristic(pService);   

    pService->start();

    //Begin broadcasting

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    // NOTE: this will broadcast service so it's discoverable before 
    //       device connection. Helps when using BLE scanner
    pAdvertising->addServiceUUID(kTargetServiceUUID);
    pAdvertising->setScanResponse(false);
    pAdvertising->setMinPreferred(0x00);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, (deviceEnabled ? HIGH : LOW));

    Serial.println("BLE Started");
}

void loop() {
  delay(200);

  if (newConfig == true)
  {
    newConfig = false;

    Serial.print("Baud rate:");
    Serial.println(baudRate);
  }

  if (Serial.available())
  {
    byte incoming = Serial.read();

    if (incoming == '1')
    {
      //baudCharacteristic->setValue("Value 1");
      Serial.println("Val set");
    }
    else if (incoming == '2')
    {
      //baudCharacteristic->setValue("Value 2");
      Serial.println("Val set");
    }
    else
    {
      Serial.println("Unknown");
    }

    delay(10);
    while (Serial.available()) Serial.read(); //Clear buffer

  }
}
