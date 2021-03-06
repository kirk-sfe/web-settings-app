
//--------------------------------------------------------------------------------------
// Written by SparkFun Electronics, Spring 2021
//
// An Client Property Sheet App to connect to an embedded device (like SparkFun OpenLog),
// determine available propertys using BLE and dynamically render the appropriate user 
// interface for each property.
//
// The app leverages the WEB BLE API, available on Chrome-based browsers.
//
// For this app to operate correctly, the underlying device must encode and advertise properties
// as shown by the examples included in the apps respository.
//
//==================================================================================
// Copyright (c) 2021 SparkFun Electronics
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//==================================================================================

// SETUP - Define the Target BLE Service
//--------------------------------------------------------------------------------------
const kTargetServiceUUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
const kTargetServiceName = "SparkFun Board";
//--------------------------------------------------------------------------------------
//
console.clear();

//--------------------------------------------------------------------------------------
// Property Creation and Management
//
// Dynamically create and manage properties
//--------------------------------------------------------------------------------------

// BLE Codes for our service
const kBLEDescSFEPropCoreUUID = 0xA101;

var bIsConnected = false;

// Text conversion helpers

const textDecoder = new TextDecoder();
function dataToText(data){ return textDecoder.decode(data);}

const textEncoder = new TextEncoder();
function textToData(text){ return textEncoder.encode(text);}

const targetID = 'settings-container';

// Property list - keeps track of the devices properties.
const currentProperties=[];

let deviceName=kTargetServiceName;
function setDeviceName(name){
    deviceName = name;
    document.getElementById("settings-title").innerHTML= name+ " Settings";
}
// DEBUG?
let debugLoadTime=0;

// Setup our progress bar object -- used when loading
const progressBar = {
    increment: 1,
    set_value: function(value){
        document.getElementById("myBar").style.width=value + "%";
    },
    add_value: function(value){
        let element =  document.getElementById("myBar");
        element.style.width = (parseInt(element.style.width)+value) + "%";
    },
    start: function(){
        this.set_value(0);
        document.getElementById("myProgress").style.display="flex";
    },
    end: function(){
        document.getElementById("myProgress").style.display="none";
    }
};

// Simple error/warning/info message box object
const messageBox = {

    msg_error : document.querySelector("#message-error"),
    msg_warning : document.querySelector("#message-warning"),

    _show_msg : function(msg, message, temporary){
        msg.innerHTML = message;
        msg.classList.replace("hide", "show");
        if(temporary)
            window.setTimeout(() =>this._hide_msg(msg), 4000);
        
    },
    _hide_msg : function(msg){
        msg.classList.replace("show", "hide");
    },
    showError: function(message, temporary=true){
        this._show_msg(this.msg_error, message, temporary);
    },
    showWarning: function(message, temporary=true){
        this._show_msg(this.msg_warning, message, temporary);
    },
    hideMessages: function(){
        this._hide_msg(this.msg_error);
        this._hide_msg(this.msg_warning);
    } 
}

//---------------------------------------
// property things
//---------------------------------------
// Used to main unique ids for property objects
let namecnt = 0;

const kBlkRange     = 0x02;
const kBlkTitle     = 0x01;
const kBlkSelectOps = 0x03;
const kBlkIncrement = 0x04;

// Each property "type" is subclassed from a base class the defines the
// property interface. 
//
// Each specialization class implements the UX of the property element and how
// to manage the property value. Also it manages information decoding from
// the BLE data received. 
//
// Base property class 
class Property{

    constructor(bleChar, name, order){
      	this.characteristic = bleChar;
        this.name = name;
        this.order = order;
    	this.ID = "property-" + namecnt++;  // unique id for the DOM
        this.div = null;
        this.timer=-1;
        this.group=null;
        this.title=null;

    }
    
    // Called to process data from the properties descriptor
    processBlk(blkType, value, iPos){


        if(blkType != kBlkTitle){  // does this property have a group title?
            return iPos;
        }
        let len = value.getUint8(iPos++);
        this.title = dataToText(new DataView(value.buffer, iPos, len));
        return iPos + len;

    }
    init(){

        // For now - enable notifications on all values ...
        this.enableNotifications();
            
        // check for group title - process using a Promise...
        
        return new Promise( (resolve) => {

            // title?
            if(this.title != null && this.title.length > 2){
                this.group= document.createElement("div");

                this.group.innerHTML = ` <h2 class="title">`+ this.title + `</h2>`;  
                document.getElementById(targetID).appendChild(this.group);
            }
            this.generateElement();
            resolve(0);
            
        }).catch(error => {
            console.log("Error setting up property: ", this.name);
        });
        

    }

    updateValue(value){}
    update(){
        this.characteristic.readValue().then( value =>{
            this.updateValue(value);
        });
    }
    enableNotifications(){
        if(this.characteristic){
            this.characteristic.startNotifications().then( _ =>{
                let target = this;
                this.characteristic.addEventListener('characteristicvaluechanged', function(event){
                    target.updateValue(event.target.value);
                });
            }).catch(error => {
                //console.log("Notifications not available.");
                // just eat this 
            });
        }
    }

    deleteElement(){

        if(this.div){
            this.div.remove();
            this.div=null;
        }
        if(this.group){
            this.group.remove();
            this.group=null;
        }

    }
}
// -------------------------------------------
// Boolean property 
class boolProperty extends Property{

    // create the ux/html / element
   	generateElement(){

   	    this.div = document.createElement("div");

   		this.div.innerHTML = `
	   		<div class="setting">
				<input type="checkbox" id="`+ this.ID + `" />
				<label for="` + this.ID + `">` + this.name + `</label>
			</div>
   		`;

   		document.getElementById(targetID).appendChild(this.div);
   		this.inputField = this.div.querySelector('input');

        // event handler wireup - on change event, save the new value to BLE device
   		this.inputField.addEventListener("change", () => {
   			this.saveValue();
   		});
   		this.update();   			
   	}

    updateValue(value){
        this.inputField.checked = value.getUint8(0, true);
    }

    saveValue(){

        // Get the value from the input field and save it to the BLEcharacteristic
        let buff = new ArrayBuffer(1);
        let newValue = new Uint8Array(buff);
        newValue[0] = this.inputField.checked;
        this.characteristic.writeValue(buff);
    }
}

//-------------------------------------------------------------
// Range Property
//
// Define some colors/UX
const range_fill = "#0B1EDF";
const range_background = "rgba(255, 255, 255, 0.214)";   

class rangeProperty extends Property{

    // Grab min and max values from the data block for this property

    processBlk(blkType, value, iPos){

        if(blkType != kBlkRange){
            return super.processBlk(blkType, value, iPos);
        }
        this.min = value.getInt32(iPos,true);
        this.max = value.getInt32(iPos+4, true);
        return iPos + 8;

    }
    
    // Used to update the UX when the value changes...
    _updateUX(value){
        //set text
        this.txtValue.setAttribute("data-length", ' ' + value);
        // set gutter color - 
        const percentage = (100 * (value - this.input.min)) / (this.input.max - this.input.min);
        const bg = `linear-gradient(90deg, ${range_fill} ${percentage}%, ${range_background} ${percentage + 0.1}%)`;
        this.input.style.background = bg;
    }
    //------------------------
   	generateElement(){

   		this.div = document.createElement("div");
   		this.div.innerHTML = `
   			<div class="length range__slider" data-min="`+ this.min +`" data-max="` + this.max +`">
				<div class="length__title field-title" data-length='0'>`+ this.name +`:</div>
				<input id="slider" type="range" min="`+ this.min +`" max="`+ this.max +`" value="16" />
			</div>
   		`;
		this.input = this.div.querySelector('input');
		this.txtValue = this.div.querySelector('.length__title');

   		document.getElementById(targetID).appendChild(this.div);
   			
   		// Update slider values - event handler
   		this.input.addEventListener("input", event => {
            this._updateUX(event.target.value);
		});

   		// On change, set the underlying value
   		this.input.addEventListener("change", () => {
   			this.saveValue();
   		});
   		// and now update to the current value
   		this.update();   			
   	}

    updateValue(value){
        this.input.value = value.getInt32(0, true);
        this._updateUX(this.input.value);

    }

    saveValue(){
        // Get the value from the input field and save it to the BLE characteristic
        let buff = new ArrayBuffer(4);
        let newValue = new Int32Array(buff);
        newValue[0] = this.input.value;
        this.characteristic.writeValue(buff);
      }
}

//-------------------------------------------------------------
// textProperty Object

class textProperty extends Property{

   	generateElement(){

   		this.div = document.createElement("div");
   		this.div.innerHTML = `
	   		<div class="text-prop">
				<input type="text" id="`+ this.ID + `" />
				<label for="` + this.ID + `">` + this.name + `</label>
			</div>
   		`;
   		document.getElementById(targetID).appendChild(this.div);

   		this.inputField = this.div.querySelector('input');
   		this.inputField.addEventListener("change", () => {
   			this.saveValue();
   		});
   		this.update();   			
   	}

	updateValue(value){
        this.inputField.value = dataToText(value);
    }

    saveValue(){
        // Get the value from the input field and save it to the characteristic
        this.characteristic.writeValue(textToData(this.inputField.value));
    }
}

//-------------------------------------------------------------
// intProperty Object

class intProperty extends Property{

    constructor(bleChar, name, order){

        super(bleChar, name, order);
        this.increment = 1;         
    }

    processBlk(blkType, value, iPos){

        if(blkType != kBlkIncrement){
            return super.processBlk(blkType, value, iPos);
        }
        this.increment = value.getUint32(iPos,true);
        return iPos + 4;
    }
   	generateElement(){

   		this.div = document.createElement("div");

   		this.div.innerHTML = `
	   		<div class="number-prop">
        <input type="number" id="`+ this.ID + `" step="` + this.increment + `" />
				<label for="` + this.ID + `">` + this.name + `</label>
			</div>
   		`;
   		document.getElementById(targetID).appendChild(this.div);

   		this.inputField = this.div.querySelector('input');
   		this.inputField.addEventListener("change", () => {
   			this.saveValue();
   		});
   		this.update();   			
   	}

    updateValue(value){
        // get the value from the BLE char and place it in the field
        this.inputField.value = value.getInt32(0, true);
    }

    saveValue(){
        // The arrows in the number field can fire off rapid events, which
        // can hammer the update, so buffer using a timer

        window.clearTimeout(this.timer);
        this.timer = window.setTimeout(()=>{
            // Get the value from the input field and save it to the characteristic
            let buff = new ArrayBuffer(4);
            let newValue = new Int32Array(buff);
            newValue[0] = this.inputField.value;
            this.characteristic.writeValue(buff);
        }, 1500);
        
    }
}
//-------------------------------------------------------------
// dateProperty Object

class dateProperty extends Property{

    
    generateElement(){

        this.div = document.createElement("div");
        this.div.innerHTML = `
            <div class="date-prop">
                <input type="date" id="`+ this.ID + `" />
                <label for="` + this.ID + `">` + this.name + `</label>
            </div>
        `;
        document.getElementById(targetID).appendChild(this.div);

        this.inputField = this.div.querySelector('input');
        this.inputField.addEventListener("change", () => {
            this.saveValue();
        });
        this.update();             
    }

    updateValue(value){
        // get the value from the BLE char and place it in the field
        // Check for a valid date format...
        let digits = dataToText(value).split("-", 3).concat(['','']);

        // if any of the digits are not a digit, default to current date
        if(isNaN(digits[0]) || isNaN(digits[1]) || isNaN(digits[2])){
            let currTime = new Date();                
            digits = [currTime.getFullYear(), currTime.getMonth(), currTime.getDay()];
        } 
        if(digits[0].length == 2){ // short year value?
            digits[0] = '20'+digits[0];
        }

        // set value - pad month and day numbers with zeros
        this.inputField.value = [digits[0], ('0'+digits[1]).slice(-2), ('0'+digits[2]).slice(-2)].join('-');
    }

    saveValue(){
        // Typing in the timefield is slow, so you can hammer the BLE
        // char with multiple sets as the date changes. Buffer this using a timer
        window.clearTimeout(this.timer);
        this.timer = window.setTimeout(()=>{
            // Get the value from the input field and save it to the characteristic
            this.characteristic.writeValue(textToData(this.inputField.value));
        }, 1500);

    }
}

//-------------------------------------------------------------
// timeProperty Object

class timeProperty extends Property{

    
    generateElement(){

        this.div = document.createElement("div");
        this.div.innerHTML = `
            <div class="time-prop">
                <input type="time" id="`+ this.ID + `" />
                <label for="` + this.ID + `">` + this.name + `</label>
            </div>
        `;
        document.getElementById(targetID).appendChild(this.div);

        this.inputField = this.div.querySelector('input');
        this.inputField.addEventListener("change", () => {
            this.saveValue();
        });
        this.update();             
    }

    updateValue(value){
        // Check for a valid time - split value, concat empty value to ensure 2 elements
        let digits = dataToText(value).split(":", 2).concat('');

        // if any of the time value are not a digit, default to current time
        if(isNaN(digits[0]) || isNaN(digits[1])){
            let currTime = new Date();                
            digits = [currTime.getHours(), currTime.getMinutes()];
        } 
        // set value - pad numbers with zeros
        this.inputField.value = [('0'+digits[0]).slice(-2), ('0'+digits[1]).slice(-2)].join(':');
    }

    saveValue(){
        // Typing in the timefield is slow, so you can hammer the BLE
        // char with multiple sets as the date changes. Buffer this using a timer

        window.clearTimeout(this.timer);
        this.timer = window.setTimeout(()=>{
            // Get the value from the input field and save it to the characteristic
            this.characteristic.writeValue(textToData(this.inputField.value));
        }, 1500);


    }
}
//-------------------------------------------------------------
// floatProperty Object

class floatProperty extends Property{
    
     processBlk(blkType, value, iPos){

        if(blkType != kBlkIncrement){
            return super.processBlk(blkType, value, iPos);
        }
        this.increment = value.getFloat32(iPos,true).toFixed(5); //round it

        return iPos + 4;
    }
    constructor(bleChar, name, order){

        super(bleChar, name, order);
        this.increment = .01;         
    }
    generateElement(){

      this.div = document.createElement("div");
      this.div.innerHTML = `
        <div class="number-prop">
        <input type="number" id="`+ this.ID + `" step="`+this.increment+`" />
        <label for="` + this.ID + `">` + this.name + `</label>
      </div>
      `;
      document.getElementById(targetID).appendChild(this.div);

      this.inputField = this.div.querySelector('input');
      this.inputField.addEventListener("change", () => {
        this.saveValue();
      });
      this.update();         
    }

    updateValue(value){
        // We are limiting floats to three decimal places.
        // Round out the floating number noise
        this.inputField.value = Math.round((value.getFloat32(0,true) + Number.EPSILON)*1000)/1000;
    }

    saveValue(){
        // The arrows in the number field can fire off rapid events, which
        // can hammer the update, so buffer using a timer

        window.clearTimeout(this.timer);
        this.timer = window.setTimeout(()=>{
            // Get the value from the input field and save it to the characteristic
            let buff = new ArrayBuffer(4);
            let newValue = new Float32Array(buff);
            newValue[0] = this.inputField.value;
            this.characteristic.writeValue(buff);
        }, 1500);
        
    }
}
//-------------------------------------------------------------
// selectProperty Object
//
// Select a value from a list of possible values

class selectProperty extends Property{

    processBlk(blkType, value, iPos){

        if(blkType != kBlkSelectOps){
            return super.processBlk(blkType, value, iPos);
        }
        let len = value.getUint8(iPos++);
        let buffer = dataToText(new DataView(value.buffer, iPos, len));
        this.options = buffer.split('|');
        return iPos + len;
    }

    //------------------------
    generateElement(){

        this.div = document.createElement("div");
        let buffer =`<div class="select-prop">
                        <select id="`+ this.ID + `">`;
        for(const option of this.options ){
            buffer = buffer + ` <option value="` + option + `">` + option + `</option>`;
        }
        this.div.innerHTML = buffer + ` </select>
                            <label for="` + this.ID + `">` + this.name + `</label>
                        </div>`;

        this.select = this.div.querySelector('select');

        document.getElementById(targetID).appendChild(this.div);

        // On change, set the underlying value
        this.select.addEventListener("change", () => {
            this.saveValue();
        });
        // and now update to the current value
        this.update();
    }

    updateValue(value){
        this.select.value = dataToText(value);
    }

    saveValue(){
        // Get the value from the select and save it.
        this.characteristic.writeValue(textToData(this.select.value));
    }
}
// --------------------
// Delete all properties
function deleteProperties(){

    while(currentProperties.length > 0){
        currentProperties[0].deleteElement(); // delete UI
        currentProperties.splice(0,1); // pop out of array
    }
    //document.getElementById(targetID).style.display="none"; // hide settings area
}

function showProperties(){ 
/*
    if(currentProperties.length > 0){
       document.getElementById(targetID).style.display="flex"; 
    }
*/
}

// Used for property sorting --
function compairPropOrder(a, b){

    if ( a.order < b.order ){
        return -1;
    }
    if ( a.order > b.order ){
        return 1;
    }
    return 0;
}

async function renderProperties(){

    // first sort our props so they display as desired
    currentProperties.sort(compairPropOrder);

    // build the UX for each property - want this is order -- so wait 

    for(const aProp of currentProperties){
        let result = await aProp.init();
    }
    progressBar.set_value(100);
    showProperties();
    endConnecting(true);

}
//--------------------------------------------------------------------------------------
// Add a property to the system based on a BLE Characteristic
//
// Our property object defs = KEY: The order is same as type code index above. 
const propFactory = [boolProperty, intProperty, rangeProperty, textProperty, dateProperty, 
                     timeProperty, floatProperty, selectProperty];

// use a promise to encapsulate the async BLE calls
function addPropertyToSystem(bleCharacteristic){

    // use a promise to encapsualte this add - might be overkill ... but 
    // with a lot of props ...
    return new Promise( (resolve) => {
        // Get the core descriptor
        bleCharacteristic.getDescriptor(kBLEDescSFEPropCoreUUID).then(descType =>{

            if(descType == null){
                console.log("No type descriptor found.")
                resolve(-1);
            }

            // Get the value of the type descriptor
            descType.readValue().then(value =>{
                let iPos = 0;
                let attributes = value.getUint32(0,true);
                let order = attributes >> 8 & 0xFF;
                let type = attributes & 0xFF;

                if(type < 0x1 || type > propFactory.length ){
                    console.log("Invalid Type value: " + type);
                    resolve(1);
                }
                iPos += 4;
                // get name
                let lenName = value.getUint8(iPos++);
                let name = dataToText(new DataView(value.buffer, iPos, lenName));
                iPos += lenName;
                // build prop object - notice index into array of prop class defs 
                let property = new propFactory[type-1](bleCharacteristic, name, order);   

                // Does this property have other data blocks in the descriptor value 
                while(iPos < value.byteLength){
                    let blkType = value.getUint8(iPos++);
                    iPos = property.processBlk(blkType, value , iPos);
                }
                currentProperties.push(property);
                progressBar.add_value(progressInc);
                resolve(0);
            }).catch(error => {
                console.log("readValue error: ", error);
                resolve(1);
            });
        }).catch(error => {
            console.log("getDescriptor - property type failed", error);
            resolve(1);
        });
    });
}

//--------------------------------------------------------------------------------------
// BLE Logic
//--------------------------------------------------------------------------------------

let theGattServer = null;
let isConnecting = false;

function disconnectBLEService(){


    if( theGattServer != null && theGattServer.connected){
        theGattServer.disconnect();

        theGattServer = null;
    }    

}
function bleConnected(gattServer){

    if(!gattServer || !gattServer.connected){
        return;
    }
    theGattServer = gattServer;
    
}
// disconnect event handler.
function onDisconnected(){

    if(isConnecting){
        return;
    }
    document.getElementById("connect").innerHTML ="Connect To " + deviceName;

    // Delete our properties....
    deleteProperties();

    theGattServer=null;

}

function startConnecting(){ 
    document.body.style.cursor = "wait";
    // update button label
    let button= document.getElementById("connect");
    button.innerHTML ="Connecting to " + deviceName + '...';
    button.style.fontStyle='italic';
    button.disabled=true;
    button.style.cursor = "not-allowed";  

    progressBar.start();
    isConnecting=true;

}

function endConnecting(success){ 

    isConnecting=false;
    document.body.style.cursor = "default";
    let button= document.getElementById("connect");
    button.style.fontStyle ='';
    button.disabled=false;
    button.style.cursor = "auto";   
    progressBar.end();
   
    if(success){
        // update button label
        document.getElementById("connect").innerHTML ="Disconnect From " + deviceName;
    }else{
        onDisconnected();
    }
    console.log("Properties Load Time:", Date.now()-debugLoadTime);
}

//------------------------------------------------------
// For connectToGatt() - cascading timer obj
const gattWatchdog = {
    timer_id: 0,
    start: (timeout) => {
        // Let's cascade warning and reset timeouts
        timer_id = setTimeout(()=>{
            // first warning
            messageBox.showWarning("Still trying to connect...");
            
            // set the reset timer
            timer_id = setTimeout(() =>{
                disconnectBLEService();
                messageBox.showError("The bluetooth service is not responding. Resetting the application connection...", false);
                // system is hung, reload page
                window.setTimeout(()=>window.location.reload(false), 4000);
            },timeout/2);
            
        }, timeout/2);
    },
    cancel: () => clearTimeout(this.timer_id)
}; 
//------------------------------------------------------
// connectToGATT()
//
// Will retry if the connection seqeunce fails - retries nTries.
//
// On failure, this routine will recurse on itself. 
//
function connectToGATT(device, nTries){

    if(nTries<1){
        console.log("Error connecting GATT server on device");
        messageBox.showError("Unable to connect with the BLE GATT server. Disconnecting.");
        endConnecting(false);
        return;
    }
    nTries--;

    progressBar.set_value(5);

    return device.gatt.connect().then(gattServer => {

        bleConnected(gattServer);
        progressBar.add_value(15);

        // On Windows - the get service call hangs sometimes. To catch
        // this create a watchdog obj that cascades timer - issues warnings and if
        // timeout exceeded, reloads page
        
        gattWatchdog.start(15000);

        // Connect to our target Service 
        return gattServer.getPrimaryService(kTargetServiceUUID).then(primaryService => {

            // kill watchdog
            gattWatchdog.cancel();

            progressBar.add_value(20);

            // Now get all the characteristics for this service
            primaryService.getCharacteristics().then(theCharacteristics => {   
                // Add the characteristics to the property sheet
                // The adds are async - so use promises, chaining everything
                // together

                progressBar.add_value(5);
                progressInc = 70./theCharacteristics.length; // for updates in promises

                // Chain together
                let chain = Promise.resolve();
                for(const aChar of theCharacteristics){                    
                    chain = chain.then(()=>addPropertyToSystem(aChar));
                }
                chain.then(() =>renderProperties());       

            }).catch(error => {
                messageBox.showWarning("Having trouble accessing the device properties. Retrying ...");
                disconnectBLEService();
                setTimeout(()=>connectToGATT(device, nTries), 200);
            });

        }).catch(error => {
            gattWatchdog.cancel();            
            disconnectBLEService();
            messageBox.showWarning("Having trouble accessing bluetooth service. Retrying ...");
            setTimeout(()=>connectToGATT(device, nTries), 200);
        });
                
    }).catch(error => {
        messageBox.showWarning("Having trouble accessing bluetooth device. Retrying ...");
        setTimeout(()=>connectToGATT(device, nTries), 200);
    });
}

function connectToBLEService() {

    // is ble supported?
    if(typeof navigator.bluetooth == "undefined" ){
        messageBox.showWarning("Bluetooth is not available in this browser. Only Chrome-based browers support Bluetooth connections.");
        return;
    }

    let filters = [];
    // KDB - for esp32 ...
    // Filtering on the name on it's own will find the device, but not the device with the
    // desired service. So adding the service to the search will find a device with the service
    // BUT not the desired name (at least in the returned objects ...)
    filters.push({name: kTargetServiceName}); 
    filters.push({services:[kTargetServiceUUID]});
    let options = {};
    options.filters = filters;
    startConnecting();
    return navigator.bluetooth.requestDevice(options).then(device => {

        debugLoadTime=Date.now(); // time the loading of the settings parameters ...

        if(device.name)
            setDeviceName(device.name);

        progressBar.set_value(5);
        device.addEventListener('gattserverdisconnected', onDisconnected);

        ///////////////////////////////////
        // Connect to the GATT - 
        // 
        // GATT service connection is touchy on windoze. This routine inplements a retry
        // strategy, trying 3 times to connect.
        ////////////////////////////////////
        connectToGATT(device, 3);
                
    }).catch(error => {
        console.log(error);
        messageBox.showWarning("Connection to bluetooth device cancelled.");
        endConnecting(false);
    });

}
//--------------------------------------------------------------------------------------
// Misc Items
//--------------------------------------------------------------------------------------

// The connect button 
const connectBtn = document.getElementById("connect");
connectBtn.addEventListener("click", () => {
    if(theGattServer && theGattServer.connected){
        disconnectBLEService();
    }else{
        connectToBLEService();
    }

});
window.onload = function(){
    setDeviceName(kTargetServiceName);

}
