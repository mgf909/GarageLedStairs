/**
* The MySensors Arduino library handles the wireless radio link and protocol
* between your home built sensors/actuators and HA controller of choice.
* The sensors forms a self healing radio network with optional repeaters. Each
* repeater and gateway builds a routing tables in EEPROM which keeps track of the
* network topology allowing messages to be routed to nodes.
*
* Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
* Copyright (C) 2013-2015 Sensnology AB
* Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
*
* Documentation: http://www.mysensors.org
* Support Forum: http://forum.mysensors.org
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* version 2 as published by the Free Software Foundation.
*
*******************************
*
* DESCRIPTION
* This sketch provides is for driving an LED light strip for lighting up stairs. Its triggered by motion sensor, or can be overridden by the controller.
Dimming function has been removed.


iDEAS:
aDD "pARTY mODE" TO RAMP UP AND DOWN RANDOMLY....

*/



#define MY_NODE_ID 22
#define MY_DEFAULT_TX_LED_PIN A2
#define MY_DEFAULT_RX_LED_PIN A3
#define MY_DEFAULT_ERR_LED_PIN A4

// Enable debug prints to serial monitor
#define MY_DEBUG 

// Enable and select radio type attached
#define MY_RADIO_NRF24

#include <SPI.h>
#include <MySensor.h> 
#include <AnalogSmooth.h>



#define SN "Stairs Light"
#define SV "1.1"
#define CHILD_ID_LIGHT 1
#define CHILD_ID_PIR1 3
#define CHILD_ID_PIR2 4



#define LED_PIN 5                         // Arduino pin attached to MOSFET Gate pin
#define PIR1_PIN 3            // The digital input you attached your motion sensor.
#define PIR2_PIN 6						 //PIR number 2

//#define INTERRUPT PIR1_PIN-2  // Usually the interrupt = pin -2 (on uno/nano anyway)
#define LIGHT_SENSOR_ANALOG_PIN A0			// Analog pin with LDR connected.


int FADE_DELAY = 20;                        // Delay in ms for each percentage fade up/down (10ms = 1s full-range dim)

unsigned long lightOffDelay = 20000;		//millisecs of no motion until the light will be turn off.
unsigned long lastOnMillis = 0L;			//no of loops until light will be turned off.
int mode = 2;								//set light control to AUTO on startup
int lastMode = 0;
int lastLightLevel;
boolean lastTripped = 0;
boolean lastTrippedB = 0;

int nightThreshold = 200;					//The default LDR value below which the led will turn on - this can be overidden.

int sendCounter = 0;
int sendCount = 10000;						//How often to send the data




MyMessage lightMsg(CHILD_ID_LIGHT, V_LIGHT);
//MyMessage dimmerMsg(CHILD_ID_LIGHT, V_DIMMER);
MyMessage motionMsg1(CHILD_ID_PIR1, V_TRIPPED);
MyMessage motionMsg2(CHILD_ID_PIR2, V_TRIPPED);
MyMessage lightLevelMsg(CHILD_ID_LIGHT, V_LIGHT_LEVEL);



// Window size can range from 1 - 100
AnalogSmooth as100 = AnalogSmooth(100);




void setup()
{
	pinMode(PIR1_PIN, INPUT);      // sets the motion sensor digital pin as input
	pinMode(PIR2_PIN, INPUT);      // sets the motion sensor digital pin as input

	Serial.println("Node ready to receive messages...");
}


void presentation() {
	// Register the LED Dimmable Light with the gateway
	present(CHILD_ID_LIGHT, S_DIMMER);
	present(CHILD_ID_PIR1, S_MOTION);
	present(CHILD_ID_PIR2, S_MOTION);
	present(CHILD_ID_LIGHT, S_LIGHT_LEVEL);
	sendSketchInfo(SN, SV);
}



void loop()
{

	//create counter for periodic sending data
	sendCounter++;
	
	

//Read the light level and smooth the LDR data
	float analogSmooth100 = as100.analogReadSmooth(LIGHT_SENSOR_ANALOG_PIN);
	int luxValue = (int)analogSmooth100;
	//Serial.println(luxValue); 
	if ((sendCount == 500) || (sendCount == 1500)) Serial.println(luxValue);

//Determine if its dark enough for the led to come on!
	boolean isItNight = (luxValue < nightThreshold);
		//if (isItNight) Serial.println("its night");



		

// Read digital motion value
	boolean tripped = digitalRead(PIR1_PIN) == HIGH;
	boolean trippedB = digitalRead(PIR2_PIN) == HIGH;

//Print out the current mode
	if (lastMode != mode) {
		Serial.println(mode);
		lastMode = mode;
	}


//Send Motion Detected to controller
	if (lastTripped != tripped) {
		Serial.println("motion detected");
		send(motionMsg1.set(tripped ? "1" : "0"));
		lastTripped = tripped;
	}

	//Send Motion Detected to controller
	if (lastTrippedB != trippedB) {
		Serial.println("motion detected on B");
		send(motionMsg2.set(trippedB ? "1" : "0"));
		lastTrippedB = trippedB;
	}


//Send data such as mode and light level value periodically
	if (sendCounter == sendCount) {
		Serial.println("SendingLuxValue");
		send(lightLevelMsg.set(luxValue));
		
		Serial.print("SendingMode : ");
		Serial.println(mode);
		send(lightMsg.set(mode));
		
		sendCounter = 0; //reset counter
	}






//Logic section
	if (mode == 2) {
		//Serial.println("Mode: NightTime AUTO ");
		if ((tripped == 1 || trippedB == 1) && (isItNight)) {
			fadeToLevel(100); //turn on the light
			lastOnMillis = millis();
		}

		if (millis() - lastOnMillis > lightOffDelay) {
			fadeToLevel(0); //turn the light off
		}
	}


	if (mode == 0) {
		//Serial.println("Mode: OFF ");
		fadeToLevel(0);
	}

	if (mode == 1) {
		//Serial.println("Mode: ON ");
		fadeToLevel(100);
	}



}





void receive(const MyMessage &message)
{
	if (message.type == V_LIGHT) {
		Serial.println("V_LIGHT command received...");

		int lstate = atoi(message.data);
		mode = lstate; ///set mode for use in logic
	}

	// get the lux value for determining night value. MQTT : mygateway1-in/11/1/1/0/24
	if ((message.sensor == CHILD_ID_LIGHT) && (message.type == V_VAR1) && (message.getInt() >25)) {  // this desired value for the 
			nightThreshold = (message.getInt()); // over-ride the default value
			Serial.print("Night Threshold value received: ");
			Serial.println(nightThreshold);

		}



}



static int currentLevel = 0;  // Current dim level...
void fadeToLevel(int toLevel) {
	int delta = (toLevel - currentLevel) < 0 ? -1 : 1;
	while (currentLevel != toLevel) {
		currentLevel += delta;
		analogWrite(LED_PIN, (int)(currentLevel / 100. * 255));
		delay(FADE_DELAY);
	}
}
