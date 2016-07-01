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



#define MY_NODE_ID 11
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
#define CHILD_ID_PIR 3



#define LED_PIN 5                         // Arduino pin attached to MOSFET Gate pin
#define DIGITAL_INPUT_SENSOR 3            // The digital input you attached your motion sensor.
#define INTERRUPT DIGITAL_INPUT_SENSOR-2  // Usually the interrupt = pin -2 (on uno/nano anyway)
#define LIGHT_SENSOR_ANALOG_PIN A0			// Analog pin with LDR connected.


int FADE_DELAY = 20;                        // Delay in ms for each percentage fade up/down (10ms = 1s full-range dim)

unsigned long lightOffDelay = 20000;		//millisecs of no motion until the light will be turn off.
unsigned long lastOnMillis = 0L;			//no of loops until light will be turned off.
int mode = 2;								//set light control to AUTO on startup
int lastMode = 0;
int lastLightLevel;
boolean lastTripped = 0;

int nightThreshold = 200;

int sendCounter = 0;
int sendCount = 10000;						//How often to send the data




MyMessage lightMsg(CHILD_ID_LIGHT, V_LIGHT);
//MyMessage dimmerMsg(CHILD_ID_LIGHT, V_DIMMER);
MyMessage motionMsg(CHILD_ID_PIR, V_TRIPPED);
MyMessage lightLevelMsg(CHILD_ID_LIGHT, V_LIGHT_LEVEL);



// Window size can range from 1 - 100
AnalogSmooth as100 = AnalogSmooth(100);




void setup()
{
	pinMode(DIGITAL_INPUT_SENSOR, INPUT);      // sets the motion sensor digital pin as input
	
	Serial.println("Node ready to receive messages...");
}


void presentation() {
	// Register the LED Dimmable Light with the gateway
	present(CHILD_ID_LIGHT, S_DIMMER);
	present(CHILD_ID_PIR, S_MOTION);
	present(CHILD_ID_LIGHT, S_LIGHT_LEVEL);
	sendSketchInfo(SN, SV);
}



void loop()
{

	//create counter for periodic sending data
	sendCounter++;
	

	//get and smooth the LDR data
	float analogSmooth100 = as100.analogReadSmooth(LIGHT_SENSOR_ANALOG_PIN);
	int luxValue = (int)analogSmooth100;
	//Serial.println(luxValue); 


	boolean isItNight = (luxValue < nightThreshold);
		if (isItNight) Serial.println("Night");






/*
	int lightLevel = (1023 - analogRead(LIGHT_SENSOR_ANALOG_PIN)) / 10.23;
	Serial.println(lightLevel);
	if (lightLevel != lastLightLevel) {
		//send(lightLevelMsg.set(lightLevel));
		lastLightLevel = lightLevel;
	}

	*/


	// Read digital motion value
	boolean tripped = digitalRead(DIGITAL_INPUT_SENSOR) == HIGH;

	//Serial.println(millis() - lastOnMillis);
	if (lastMode != mode) {
		Serial.println(mode);
		lastMode = mode;
	}


	//Send Motion Detected to controller
	if (lastTripped != tripped) {
		send(motionMsg.set(tripped ? "1" : "0"));
		lastTripped = tripped;
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
		if ((tripped == 1) && (isItNight)) {
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
