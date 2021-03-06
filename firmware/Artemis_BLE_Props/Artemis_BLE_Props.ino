/*
 *
 * SparkFun ArduinoBLE (SparkFun Artemis) BLE Settings App Example
 * =======================================
 *
 *  ArduinoBLE/Artemis example of the  SparkFun BLE Property system.
 *
 *  The example shows how to setup BLE "properties" in the Artemis Arduino 
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
#include "ArduinoBLE.h"

// Include SparkFun functions to define "properties" out of characteristics
// This enables use of the SparkFun BLE Settings Web-App
#include "sf_ble_prop.h"

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
#define kTargetServiceUUID  "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define kTargetServiceName  "Artemis"
#define kIDNameLength 6
//--------------------------------------------------------------------------------------
// Our Characteristic UUIDs - and yes, just made these up
//--------------------------------------------------------------------------------------

#define kCharacteristicBaudUUID     "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define kCharacteristicEnabledUUID  "beb5483e-36e1-4688-b7f5-ea07361b26a9"
#define kCharacteristicMessageUUID  "beb5483e-36e1-4688-b7f5-ea07361b26aa"
#define kCharacteristicSampleUUID   "beb5483e-36e1-4688-b7f5-ea07311b260b"
#define kCharacteristicDateUUID     "beb5483e-36e1-4688-b7f5-ea07311b260c"
#define kCharacteristicTimeUUID     "beb5483e-36e1-4688-b7f5-ea07311b260d"
#define kCharacteristicOffsetUUID   "beb5483e-36e1-4688-b7f5-ea07311b260e"
#define kCharacteristicModeUUID   "beb5483e-36e1-4688-b7f5-ea07311b260f"

// helper for message limits
#define kMessageMax 64

//--------------------------------------------------------------------------------------
// ArduinoBLE Object declaration 
//
// Declare the main objects that define this BLE service here. Will connect everything 
// together later in the app logic. This enables these objects to be stack based

// Our BLE Service
BLEService bleService(kTargetServiceUUID);

// Define the BLE Service Characteristics - or "Properties"
//
// Note - for each characterisitic, a storage value is also defined for it
//
// >> Notifications <<
//    Only enable for those characteristics that will use notifications. Adding this type
//    to a Characteristic adds a BLE Descriptor. If not using notify, save the resources.
//
//---------------------------------------------------------------------------
// First Characteristic - "Enabled" - bool - is the system enabled 
boolean bIsEnabled = true;
BLEBooleanCharacteristic bleCharEnabled(kCharacteristicEnabledUUID, BLERead | BLEWrite);


//---------------------------------------------------------------------------
// Second Characteristic - Baud Rate - int - the baud rate of the syste
int32_t baudRate = 115200;
BLEIntCharacteristic bleCharBaudRate(kCharacteristicBaudUUID, BLERead | BLEWrite);


//---------------------------------------------------------------------------
// Third Characteristic - Device Name - string - a string Name for the device
String strName = "Artemis Device";
// Note: Setting max value size of kMessage Max  
BLEStringCharacteristic bleCharName(kCharacteristicMessageUUID, BLERead | BLEWrite, kMessageMax);


//---------------------------------------------------------------------------
// Fourth Characteristic - Sample Rate - Range (int) - how often to sample this thing
uint32_t sampleRate = 123;

const uint32_t sampleRateMin = 10;
const uint32_t sampleRateMax = 240;

BLEIntCharacteristic bleCharSampleRate(kCharacteristicSampleUUID, BLERead | BLEWrite);


//---------------------------------------------------------------------------
// Fifth Characteristic - Date - date type - a date string of format "YYYY-MM-DD"
String strDate = "2021-03-01";
BLEStringCharacteristic bleCharDate(kCharacteristicDateUUID, BLERead | BLEWrite, kMessageMax);


//---------------------------------------------------------------------------
// Sixth Characteristic - Time - time type - a time string of format "HH:MM"
String strTime = "12:13";
BLEStringCharacteristic bleCharTime(kCharacteristicTimeUUID, BLERead | BLEWrite, kMessageMax);


//---------------------------------------------------------------------------
// Seventh Characteristic -Offset - Float type - the offset value.
//
// This value has Notifications enabled.
// A float property - "offset value"
float offsetValue = 4.124;
BLEFloatCharacteristic bleCharOffset(kCharacteristicOffsetUUID, BLERead | BLENotify | BLEWrite);

//---------------------------------------------------------------------------
//  Characteristic - "Mode" - Select type 
//
// This value has Notifications enabled.
// A float property - "offset value"
String strMode = "Stepper";
BLEStringCharacteristic bleCharMode(kCharacteristicModeUUID, BLERead | BLEWrite, kMessageMax);

//--------------- end object setup ------------------------
// We're using the Enable property to control the on-board LED...
const byte LED_TO_TOGGLE = 19;

//--------------------------------------------------------------------------------------
// Operational State
//--------------------------------------------------------------------------------------
// Value used to manage timing for a Notification example in loop.
unsigned long ticks;

// >> Work Timeout <<
// On BLE connect from the settings app client, the BLE system needs resources to process
// the various descriptor requests from the client app. Any additional work being 
// performed in loop() can impact the BLE systems performance.
//
// To provide a "work" pause on connect, this example implements a "on connection" event  
// is determined,and a work "timeout" is implemented for N seconds. 
//  
// Define work timeout in MS. 
const unsigned int bleOnConnectDelay = 3400;  // ms  on BLE connection "work" timeout


//--------------------------------------------------------------------------------------
// Property changed callback functions.
//--------------------------------------------------------------------------------------
// These functions are connected to their respective characteristics and are called
// when the underlying value is changed.
//
// These functions provide examples of accessing different data types from a
// Characteristic
//
// The function signature is defined by Arduino BLE
//---------------------------------------------------------------------------
void enbabledUpdateCB(BLEDevice central, BLECharacteristic theChar){

    Serial.print("Enabled Update: ");
    uint8_t newValue;
    theChar.readValue(newValue); // no readValue for bools
    bIsEnabled = (boolean)newValue;
    Serial.println(bIsEnabled);

    digitalWrite(LED_BUILTIN, (bIsEnabled ? HIGH : LOW));
}

//---------------------------------------------------------------------------
void buadRateUpdateCB(BLEDevice central, BLECharacteristic theChar){

    Serial.print("Baud Rate Update: ");
    theChar.readValue(baudRate);
    Serial.println(baudRate);
}

//---------------------------------------------------------------------------
void nameUpdateCB(BLEDevice central, BLECharacteristic theChar){

    char buffer[kMessageMax+1]={0}; // buffer size + null - zero out 

    int len = theChar.valueLength();
    theChar.readValue((void*)buffer, len > kMessageMax ? kMessageMax : len);

    strName = buffer;
    Serial.print("Device Name Update: ");
    Serial.println(strName);;
}

//---------------------------------------------------------------------------
void sampleRateUpdateCB(BLEDevice central, BLECharacteristic theChar){

    Serial.print("Sample Rate Update: ");
    theChar.readValue(sampleRate);
    Serial.println(sampleRate);
}

//---------------------------------------------------------------------------
void dateUpdateCB(BLEDevice central, BLECharacteristic theChar){

    char buffer[kMessageMax+1]={0}; // buffer size + null - zero out 

    int len = theChar.valueLength();
    theChar.readValue((void*)buffer, len > kMessageMax ? kMessageMax : len);

    strDate = buffer;
    Serial.print("Date Update: ");
    Serial.println(strDate);
}

//---------------------------------------------------------------------------
void timeUpdateCB(BLEDevice central, BLECharacteristic theChar){

    char buffer[kMessageMax+1]={0}; // buffer size + null - zero out 

    int len = theChar.valueLength();
    theChar.readValue((void*)buffer, len > kMessageMax ? kMessageMax : len);

    strTime = buffer;
    Serial.print("Time Update: ");
    Serial.println(strTime);
}

//---------------------------------------------------------------------------
void offsetUpdateCB(BLEDevice central, BLECharacteristic theChar){

    Serial.print("Offset (float) Update: ");
    theChar.readValue((void*)&offsetValue, 4);
    Serial.println(offsetValue);
}

//---------------------------------------------------------------------------
void modeUpdateCB(BLEDevice central, BLECharacteristic theChar){

    char buffer[kMessageMax+1]={0}; // buffer size + null - zero out 

    int len = theChar.valueLength();
    theChar.readValue((void*)buffer, len > kMessageMax ? kMessageMax : len);

    strMode = buffer;
    Serial.print("Mode Update: ");
    Serial.println(strMode);
}
//--------------------------------------------------------------------------------------
// Characteristic Setup
//--------------------------------------------------------------------------------------
// Connect and setup each characteristic broadcast from this service.
//
// >> SparkFun Web-App Configuration <<
//    
//   The characteristics are configured to work with the SparkFun BLE Settings Web-App.
// 
//   For each characteristic:
//      - The related SparkFun BLE Property function is called. This adds the BLE Descriptors 
//        used to define the attributes of this characteristic that the SparkFun BLE app uses
//      - Add the characteristic to the service
//      - Set the value of the characteristic to it's associated value variable (declared above)
//      - Set the value event handler (declared above)
// 
//   The setup sequence defines the user experience in the SparkFun BLE Application. 
//   
//   Specifically:
//          - The order that characteritics are setup (calls to sfe_bleprop_ functions),
//            defines the display order in the app.
//          - Adding a "Group Title" to a characteristic, causes the app to display this 
//            title before rendering the property GUI. A group title is just a title, nothing more.
//   
//   

void setupBLECharacteristics(BLEService& theService){

    // The enabled characteristic --- add it as a boolean property
    BLEProperties.addBool(bleCharEnabled, "Device Enabled"); // setup property descriptor    

    theService.addCharacteristic(bleCharEnabled);  
    bleCharEnabled.setValue(bIsEnabled);
    bleCharEnabled.setEventHandler(BLEWritten, enbabledUpdateCB);

    // The date characteristic with title
    BLEProperties.addTitle("Event Details");    
    BLEProperties.addDate(bleCharDate, "Start Date" ); // setup property descriptor     

    theService.addCharacteristic(bleCharDate);  
    bleCharDate.setValue(strDate);
    bleCharDate.setEventHandler(BLEWritten, dateUpdateCB); 

    // The time characteristic
    BLEProperties.addTime(bleCharTime, "Start Time"); // setup property descriptor                

    theService.addCharacteristic(bleCharTime);  
    bleCharTime.setValue(strTime);
    bleCharTime.setEventHandler(BLEWritten, timeUpdateCB);  


    // The BaudRate characteristic with title
    BLEProperties.addTitle("Device Settings");

    // NOTE: Can call add_int() with or without an increment value. 
    // No increment - defaults to 1
    //BLEProperties.addInt(bleCharBaudRate, "Baud Rate"); // setup property descriptor        
    
    // Set to an increment value of 100
    BLEProperties.addInt(bleCharBaudRate, "Baud Rate", 100); // setup property descriptor    

    theService.addCharacteristic(bleCharBaudRate);  
    bleCharBaudRate.setValue(baudRate);
    bleCharBaudRate.setEventHandler(BLEWritten, buadRateUpdateCB);

    // The Name characteristic
    BLEProperties.addString(bleCharName, "Device Name"); // setup property descriptor        

    theService.addCharacteristic(bleCharName);  
    bleCharName.setValue(strName);
    bleCharName.setEventHandler(BLEWritten, nameUpdateCB);    
        
    // The Sample Rate characteristic with title
    BLEProperties.addTitle("Sensor Settings");    
    BLEProperties.addRange(bleCharSampleRate, "Sample Rate (sec)", sampleRateMin, sampleRateMax); // setup property descriptor            

    theService.addCharacteristic(bleCharSampleRate);
    bleCharSampleRate.setValue(sampleRate);     
    bleCharSampleRate.setEventHandler(BLEWritten, sampleRateUpdateCB);             

    // The Offset (float) characteristic

    // add_float() - default, with no increment defaults to .01 increment
    // BLEProperties.addFloat(bleCharOffset, "Offset Bias"); // setup property descriptor  

    // Set a increment value of .001
    BLEProperties.addFloat(bleCharOffset, "Offset Bias", 0.001); // setup property descriptor  

    // Set the increment value of .001
    theService.addCharacteristic(bleCharOffset);  
    bleCharOffset.setValue(offsetValue);
    bleCharOffset.setEventHandler(BLEWritten, offsetUpdateCB);  

    // The Mode (select type) characteristic. Note the available values seperated by a "|"
    BLEProperties.addSelect(bleCharMode, "Active Mode", "Constant|Stepper|Chirp"); // setup property descriptor  

    theService.addCharacteristic(bleCharMode);  
    bleCharMode.setValue(strMode);
    bleCharMode.setEventHandler(BLEWritten, modeUpdateCB);  
}
//-------------------------------------------------------------------------
// A BLE client  is connected logic.
//-------------------------------------------------------------------------
//
// The system is setup to call callback methods to the below object. 
// A bool is used to keep track of connected state..
bool deviceConnected = false;

// General Connect callbacks
void blePeripheralConnectHandler(BLEDevice central) {
  // central connected event handler
  Serial.print("Connected event, central: ");
  Serial.println(central.address());
  deviceConnected = true;
}

void blePeripheralDisconnectHandler(BLEDevice central) {
  // central disconnected event handler
  Serial.print("Disconnected event, central: ");
  Serial.println(central.address());
  deviceConnected = false;  
}
//---------------------------------------------------------------------------------
// Build a device name that uses the BLE mac address
//---------------------------------------------------------------------------------
char szName[24];
void setUniqueBLEName(){

    // Build a unique name using the BLE address 
    String strAddress = BLE.address();
    Serial.print("BLE Address: ");Serial.println(strAddress);

    // Make a unique name using the address - pull out ":"s, shorten length to kIDNameLength and upcase it
    for(int i;   (i = strAddress.indexOf(":")) > -1; strAddress.remove(i,1));

    // shorten name and upcase it
    strAddress = strAddress.substring(strAddress.length()-kIDNameLength);
    strAddress.toUpperCase();
    snprintf(szName, sizeof(szName), "%s - %s", kTargetServiceName, strAddress.c_str());

    Serial.print("Device Name: "); Serial.println(szName);

    // name the device
    BLE.setLocalName(szName);
}
//---------------------------------------------------------------------------------
// Setup our system
//---------------------------------------------------------------------------------

void setup() {

    // start up serial port
    Serial.begin(115200);
    while (!Serial);

    Serial.println();
    Serial.println("Starting BLE Setup...");
    
    // led to display when connected
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, (bIsEnabled ? HIGH : LOW));

    // start BLE
    if ( ! BLE.begin()){
        Serial.println("starting BLE failed!");
        while (1);
    }

    // assign event handlers for connected, disconnected to peripheral
    BLE.setEventHandler(BLEConnected, blePeripheralConnectHandler);
    BLE.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

    // Build a unique name using the BLE address 
    setUniqueBLEName();
    
    // Setup service Characteristics
    setupBLECharacteristics(bleService);

    // add the service to the BLE device and start to broadcast.
    BLE.addService(bleService);
    BLE.setAdvertisedService(bleService);

    // broadcast BLE connection
    BLE.advertise();

    ticks = millis(); // for our notify example below
    Serial.println(F("OLA BLE ready for connections!"));

}

//-----------------------------------------------------------------------------------
// function to determine if work should pause while a new BLE Connection initializes
//
// Returns true if work should be performed. 
bool doWork(){
    
    // vars to keep track of state. Make static to live across function calls
    static unsigned int bleTicks =0;
    static bool wasConnected = false;

    // did we just connect? If so, give the BLE system most of our loop resouces
    // to manage the connection startup
    if(!wasConnected && deviceConnected){ // connection state change
        bleTicks = millis();
        Serial.println("start work delay");
    }
    // Are we at the end the work timeout?
    if(bleTicks && millis()-bleTicks > bleOnConnectDelay){
        bleTicks = 0;
        Serial.println("end work delay");
    }
    wasConnected = deviceConnected;

    // do work if ticks equals 0.
    return bleTicks == 0;

}
//-----------------------------------------------------------------------------------
void loop()
{
    // >> Update and Notification Example <<
    //
    // This section updates the offset characterisitc value every 5 secs if a
    // device is connected. Demostrates how to send a notification using the 
    // BLE API.
    if(millis() - ticks > 5000){
        // Update the value of offset and set in BLE char
        // Should trigger a notification on client
        offsetValue += .5;
        if(deviceConnected && doWork()){
            bleCharOffset.setValue(offsetValue); // triggers notify message b/c char created w/ BLENotify
            Serial.print("Incrementing Offset to: "); Serial.println(offsetValue);
        }
        ticks = millis();
    }
    
    // Do work, or pause work to give most resources to the BLE system on client connection initialization
    if(doWork()){
        ////////////////////////////
        // >> DO LOOP WORK HERE <<
        ///////////////////////////
        delay(200);    // Example *WORK*
    }

    // Pump the BLE service - everything is handled in callbacks.
    BLE.poll();

}
