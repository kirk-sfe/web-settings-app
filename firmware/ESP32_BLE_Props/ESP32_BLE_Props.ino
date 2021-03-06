/*
 *
 * SparkFun ESP32 BLE Settings App Example
 * =======================================
 *
 *  ESP32 example of the  SparkFun BLE Property system.
 *
 *  The example shows how to setup BLE "properties" in the ESP32 Arduino 
 *  environment for use with the SparkFun BLE Web Property Sheet.
 * 
 *  HISTORY
 *    Feb, 2021     - Initial developement - KDB
 * 
 *==================================================================================
 * Copyright (c) 2021 SparkFun Electronics
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *==================================================================================
 * 
 */
#include <string>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// Include BLE2902 - used to enable BLE notifications on a Characterisitcs.
#include <BLE2902.h>

// The examples callback object simplification header file
#include "ESP32_BLE_Config.h"

// Include SparkFun functions to define "properties" out of characteristics
// This enables use of the SparkFun BLE Settings Web-App
#include "sf_ble_prop.h"

#define LED_BUILTIN 13

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define kTargetServiceUUID  "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
// Name of this app/service
#define kTargetServiceName  "ESP32 Thing"

//--------------------------------------------------------------------------------------
// Our Characteristic UUIDs - and yes, just made these up
//--------------------------------------------------------------------------------------
#define kCharacteristicBaudUUID         "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define kCharacteristicEnabledUUID      "beb5483e-36e1-4688-b7f5-ea07361b26a9"
#define kCharacteristicSSIDUUID         "beb5483e-36e1-4688-b7f5-ea07361b26aa"
#define kCharacteristicPasswordUUID     "beb5483e-36e1-4688-b7f5-ea07361b26ab"
#define kCharacteristicSampleUUID       "beb5483e-36e1-4688-b7f5-ea07311b260b"
#define kCharacteristicDateUUID         "beb5483e-36e1-4688-b7f5-ea07311b260c"
#define kCharacteristicTimeUUID         "beb5483e-36e1-4688-b7f5-ea07311b260d"
#define kCharacteristicOffsetUUID       "beb5483e-36e1-4688-b7f5-ea07311b260e"

//-------------------------------------------------------------------------
// BLE Characteristics require persistant values for data storage. Nothing stack-based.
//-------------------------------------------------------------------------
//
// The following define the values (property values) used in this example

// PROPERTY Data/Local Variables
uint32_t baudRate = 115200;

bool deviceEnabled = true;

// SSID and password prop
std::string strSSID("myWiFiNetwork");
std::string strPassword("HelloWiFi");

// sample rate prop (a range)
uint32_t sampleRate = 123;
uint32_t sampleRateMin = 10;
uint32_t sampleRateMax = 240;

// Date prop - date is a string "YYYY-MM-DD"  - formatting is important
std::string strDate("2021-03-01");

// Time prop - date is a string "HH:MM" - 
std::string strTime("2:5"); // test value - will be parsed as 02:05

// A float property - "offset value"
float offsetValue = 4.124;

BLECharacteristic *pCharOffset; // will use later for notifications.
unsigned long ticks; // used for notification

// Value for our mode "selector"
std::string strMode("Stepper");

//-------------------------------------------------------------------------
// A BLE client (device) is connected logic.
//-------------------------------------------------------------------------
//
// The system is setup to call callback methods to the below object. 
// A bool is used to keep track of connected state..

bool deviceConnected = false;

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

// String utility for BLE string data marshaling
//
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

//-------------------------------------------------------------------------
// Characteristic Setup Section
//-------------------------------------------------------------------------
// The following functions are used to setup specific characteristics
//
// Consisist of a setup function and a callback function that the BLE system
// will call on a value update.
//
//-------------------------------------------------------------------------
// Enabled Characterisitic 
//
// A Bool Characterisitic (property) example

void onEnabledUpdate(bool newValue){

    Serial.print("Update Enabled Value: "); 
    Serial.println(deviceEnabled);
    deviceEnabled = newValue;
    digitalWrite(LED_BUILTIN, (deviceEnabled ? HIGH : LOW));    
}

BLECharacteristic * setupEnabledCharacteristic(BLEService *pService){

    BLECharacteristic *pCharBaud;

    pCharBaud = pService->createCharacteristic(
                            kCharacteristicEnabledUUID,
                            BLECharacteristic::PROPERTY_READ  |
                            BLECharacteristic::PROPERTY_WRITE |
                            BLECharacteristic::PROPERTY_NOTIFY );

    // Set the value in the callbacks - this is using our defined object to
    // just relay the update to the onEnabledUpdate() c function.
    pCharBaud->setCallbacks(new BoolPropertyValueCB(&deviceEnabled, onEnabledUpdate));

    return pCharBaud;
}


//---------------------------------------------------------------------------------
// Baud Rate Characterisitic 
//
// A Integer Characterisitic (property) example

void onBaudUpdate(int32_t newValue){

    Serial.print("Update Baud Value: "); 
    Serial.println(baudRate);
}

BLECharacteristic * setupBaudCharacteristic(BLEService *pService){

    BLECharacteristic *pCharBaud;

    pCharBaud = pService->createCharacteristic(
                            kCharacteristicBaudUUID,
                            BLECharacteristic::PROPERTY_READ  |
                            BLECharacteristic::PROPERTY_WRITE |
                            BLECharacteristic::PROPERTY_NOTIFY );

    // Set the value in the callbacks 
    pCharBaud->setCallbacks(new IntPropertyValueCB((int32_t*)&baudRate, onBaudUpdate));
    
    return pCharBaud;
}
//---------------------------------------------------------------------------------
// SSID Characterisitic 
//
// A String Characterisitic (property) example

void onSSIDUpdate(std::string &  newValue){

    Serial.print("Update SSID Value: "); 
    Serial.println(newValue.c_str());
}

BLECharacteristic * setupSSIDCharacteristic(BLEService *pService){

    BLECharacteristic *pChar;

    pChar = pService->createCharacteristic(
                            kCharacteristicSSIDUUID,
                            BLECharacteristic::PROPERTY_READ  |
                            BLECharacteristic::PROPERTY_WRITE );

    // Set the value in the callbacks 
    pChar->setCallbacks(new TextPropertyValueCB(strSSID, onSSIDUpdate));

    return pChar;
}

//---------------------------------------------------------------------------------
// WiFi Password Characterisitic 
//
// A String Characterisitic (property) example

void onPasswordUpdate(std::string &  newValue){

    Serial.print("Update WiFi Password Value: "); 
    Serial.println(newValue.c_str());
}

BLECharacteristic * setupPasswordCharacteristic(BLEService *pService){

    BLECharacteristic *pChar;

    pChar = pService->createCharacteristic(
                            kCharacteristicPasswordUUID,
                            BLECharacteristic::PROPERTY_READ  |
                            BLECharacteristic::PROPERTY_WRITE );

    // Set the value in the callbacks 
    pChar->setCallbacks(new TextPropertyValueCB(strPassword, onPasswordUpdate));

    return pChar;
}
//---------------------------------------------------------------------------------
// Sample Rate Characterisitic 
//
// A Integer Range Characterisitic (property) example

void onSampleRateUpdate(int32_t newValue){

    Serial.print("Update Sample Value: "); 
    Serial.println(sampleRate);
}

BLECharacteristic * setupSampleRateCharacteristic(BLEService *pService){


    BLECharacteristic *pCharRate;

    pCharRate = pService->createCharacteristic(
                            kCharacteristicSampleUUID,
                            BLECharacteristic::PROPERTY_READ  |
                            BLECharacteristic::PROPERTY_WRITE |
                            BLECharacteristic::PROPERTY_NOTIFY );

    // Set the value in the callbacks - can just use the int callbacks
    pCharRate->setCallbacks(new IntPropertyValueCB((int32_t*)&sampleRate, onSampleRateUpdate));

    return pCharRate;
}

//---------------------------------------------------------------------------------
// Date Characterisitic 
//
// A Date Characterisitic (property) example - format is a string "YYYY-MM-DD"

void onDateUpdate(std::string &  newValue){

    Serial.print("Update Date Value: "); 
    Serial.println(newValue.c_str());
}

BLECharacteristic * setupDateCharacteristic(BLEService *pService){

    BLECharacteristic *pCharDate;

    pCharDate = pService->createCharacteristic(
                            kCharacteristicDateUUID,
                            BLECharacteristic::PROPERTY_READ  |
                            BLECharacteristic::PROPERTY_WRITE |
                            BLECharacteristic::PROPERTY_NOTIFY );

    // Set the value in the callbacks 
    pCharDate->setCallbacks(new TextPropertyValueCB(strDate, onDateUpdate));

    return pCharDate;   
}
//---------------------------------------------------------------------------------
// Time Characterisitic 
//
// A Time Characterisitic (property) example - format is a string "HH:MM"

void onTimeUpdate(std::string &  newValue){

    Serial.print("Update Time Value: "); 
    Serial.println(newValue.c_str());
}

BLECharacteristic * setupTimeCharacteristic(BLEService *pService){

    BLECharacteristic *pCharTime;

    pCharTime = pService->createCharacteristic(
                            kCharacteristicTimeUUID,
                            BLECharacteristic::PROPERTY_READ  |
                            BLECharacteristic::PROPERTY_WRITE |
                            BLECharacteristic::PROPERTY_NOTIFY );

    // Set the value in the callbacks 
    pCharTime->setCallbacks(new TextPropertyValueCB(strTime, onTimeUpdate));

    return pCharTime;
}

//---------------------------------------------------------------------------------
// FLoat Characterisitic 
//
// A float Characterisitic (property) example 

void onOffsetUpdate(float newValue){

    Serial.print("Update Offset(float) Value: "); 
    Serial.println(newValue);
}

BLECharacteristic * setupOffsetCharacteristic(BLEService *pService){

    BLECharacteristic *pCharOff;

    pCharOff = pService->createCharacteristic(
                            kCharacteristicOffsetUUID,
                            BLECharacteristic::PROPERTY_READ  |
                            BLECharacteristic::PROPERTY_WRITE |
                            BLECharacteristic::PROPERTY_NOTIFY );

    // Set the value in the callbacks 
    pCharOff->setCallbacks(new FloatPropertyValueCB(&offsetValue, onOffsetUpdate));

    return pCharOff;

}

//---------------------------------------------------------------------------------
// Mode Characterisitic 
//
// A Mode Characterisitic (property) example - A string value

void onModeUpdate(std::string &  newValue){

    Serial.print("Update Mode Value: "); 
    Serial.println(newValue.c_str());
}

BLECharacteristic * setupModeCharacteristic(BLEService *pService){

    BLECharacteristic *pChar;

    pChar = pService->createCharacteristic(
                            kCharacteristicTimeUUID,
                            BLECharacteristic::PROPERTY_READ  |
                            BLECharacteristic::PROPERTY_WRITE );

    // Set the value in the callbacks 
    pChar->setCallbacks(new TextPropertyValueCB(strMode, onModeUpdate));

    return pChar;
}
char szName[24]; // name storage
//---------------------------------------------------------------------------------
// Setup our system
//---------------------------------------------------------------------------------

void setup() {

    Serial.begin(115200);
    Serial.println("Starting BLE work!");

    // Build a unique name for this device - based on the BLE mac address
    // esp32 requires the name during the init process and apparent way to change 
    // the value (not via arduino API). But the mac address is based of the unique chip ID.
    //
    // Use the first three bytes of the chipd id to create a unique address
    //
    uint64_t chipid = ESP.getEfuseMac(); // The chip ID is essentially its MAC address(length: 6 bytes).
    snprintf(szName, sizeof(szName), "%s - %06X", kTargetServiceName, chipid & 0xFFFFFF);
    Serial.print("Device Name: "); Serial.println(szName);

    // Init BLE  - give it our device name.
    BLEDevice::init(szName);

    Serial.print("Device Address: "); 
    Serial.println(BLEDevice::getAddress().toString().c_str());

    // Create our BLE Server - And add callback object (defined above)
    // The callback is used to keep track if a device is connected or not.
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // >>> NOTE <<<
    // When creating a service. The default - when you just pass in a UUID,
    // only creates 15 handles. This isn't enough to support the > 3 characteristics 
    // in this demo. Setting this to 5 per Characteristic seems to work.
    // see: https://github.com/nkolban/ESP32_BLE_Arduino/blob/master/src/BLEServer.cpp
    // 
    // Defining these here to highlight this concept. A
#define NUMBER_OF_CHARACTERISTICS 8
#define ESP32_BLE_HANDLES_PER_CHAR 5
    BLEService *pService = pServer->createService(BLEUUID(kTargetServiceUUID), 
            NUMBER_OF_CHARACTERISTICS * ESP32_BLE_HANDLES_PER_CHAR, 1);

    // >>> Characteristics Setup <<<
    //
    // The order of the setup calls, sets the order the characteristics (properties)
    // in the property/settings application. 
    //
    // A grouping title/area in the settings application is defined by adding a title
    // to the first Characteristic in that group. **ONLY** do this on the first 
    // characterisitic of the group - it's just a title, nothing more.

    BLECharacteristic *pChar;

    pChar = setupEnabledCharacteristic(pService);
    BLEProperties.addBool(pChar, "Device Enabled");

    // Event Details Group
    pChar = setupDateCharacteristic(pService);
    BLEProperties.addTitle("Event Details");
    BLEProperties.addDate(pChar, "Start Date");

    pChar = setupTimeCharacteristic(pService); 
    BLEProperties.addTime(pChar, "Start Time");

    // Device Settings
    pChar = setupBaudCharacteristic(pService);
    BLEProperties.addTitle("Device Settings");   
    // NOTE: Can call add_int() with or without an increment value. 
    
    // No increment - defaults to 1
    //BLEProperties.addInt(pChar, "Baud Rate"); // setup property descriptor        

    // Set to an increment value of 100
    BLEProperties.addInt(pChar, "Baud Rate", 100);  

    // WiFi Settings
    pChar = setupSSIDCharacteristic(pService);
    BLEProperties.addTitle("WiFi Settings");        
    BLEProperties.addString(pChar, "SSID");

    pChar =  setupPasswordCharacteristic(pService);    
    BLEProperties.addString(pChar, "Password");

    // Sensor Settings 
    pChar = setupSampleRateCharacteristic(pService);
    BLEProperties.addTitle("Sensor Settings");            
    BLEProperties.addRange(pChar, "Sample Rate (sec)", sampleRateMin, sampleRateMax);    

    pCharOffset = setupOffsetCharacteristic(pService);

    // add_float() - default, with no increment defaults to .01 increment
    // BLEProperties.addFloat(pCharOffset, "Offset Bias");

    // Set the increment value of .001
    BLEProperties.addFloat(pCharOffset, "Offset Bias", 0.001);    


    // Setup "mode" - note this is a "select" property, with three possible values:
    //   "Constant", "Stepper", "Chirp".  -- the values are seperated by a '|' in a single string
    pChar = setupModeCharacteristic(pService);
    // The Mode (select type) characteristic
    BLEProperties.addSelect(pChar, "Active Mode", "Constant|Stepper|Chirp"); 

    // >> Notifications <<
    //
    // Will send update settings of this characteristic - save the Characterisitc pointer
    // for use in loop, and add the BLE2902 descriptor which is used for BLE Notifications.

    // On ESP32 - to enable notification, you need to add a special descriptor
    pCharOffset->addDescriptor(new BLE2902());


    // Startup up the system and start BLE advertising
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

    // We're up, LED ON
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, (deviceEnabled ? HIGH : LOW));

    // Keep track of millis to support our Notification example in loop
    ticks = millis();
    Serial.println("BLE Started");
}

void loop() {

    delay(200);

    // >> Update and Notification Example <<
    //
    // This section updates the offset characterisitc value every 5 secs if a
    // device is connected. Demostrates how to send a notification using the 
    // ESP32 BLE API.
    if(millis() - ticks > 5000){

        // Update the value of offset and set new value in the target Char.
        // Then send notification

        offsetValue += .5;
        if(deviceConnected){
            pCharOffset->setValue(offsetValue);
            pCharOffset->notify();
            Serial.print("Incrementing Offset to: "); Serial.println(offsetValue);
        }
        ticks = millis();
    }

}
