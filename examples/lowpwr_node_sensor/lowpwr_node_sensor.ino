/*
 * Project: nRF905 Radio Library for Arduino (Low power sensor node example)
 * Author: Zak Kemble, contact@zakkemble.net
 * Copyright: (C) 2020 by Zak Kemble
 * License: GNU GPL v3 (see License.txt)
 * Web: https://blog.zakkemble.net/nrf905-avrarduino-librarydriver/
 */

// This examples requires the low power library from https://github.com/rocketscream/Low-Power

// This examples configures the nRF905 library to only use 5 connections:
// MOSI
// MISO
// SCK
// SS -> 6
// PWR -> 8

// The following pins on the nRF905 must be connected to VCC (3.3V) or GND:
// CE (TRX_EN) -> VCC
// TXE (TX_EN) -> VCC

#include <nRF905.h>
#include <SPI.h>
#include <LowPower.h>

#define BASE_STATION_ADDR	0xE7E7E7E7
#define NODE_ID				78
#define LED					A5

nRF905 transceiver = nRF905();

static bool txDone; // NOTE: In polling mode this does not need to be volatile

// Event function for TX completion
void nRF905_onTxComplete(nRF905* device)
{
	txDone = true;
}

void setup()
{
	Serial.begin(115200);
	Serial.println(F("Sensor node starting..."));
	
	pinMode(LED, OUTPUT);


	// standby off TODO
	//pinMode(7, OUTPUT);
	//digitalWrite(7, HIGH);
	// pwr
	//pinMode(8, OUTPUT);
	//digitalWrite(8, HIGH);
	// trx
	//pinMode(9, OUTPUT);
	//digitalWrite(9, HIGH);
	
	
	// This must be called first
	SPI.begin();

	// Minimal wires (polling mode)
	// Up to 5 wires can be disconnected, however this will reduce functionalliy and will put the library into polling mode instead of interrupt mode.
	// In polling mode the .poll() method must be called as often as possible. If .poll() is not called often enough then events may be missed.
	transceiver.begin(
		SPI,
		10000000,
		6,
		NRF905_PIN_UNUSED, // CE (standby) pin must be connected to VCC (3.3V)
		NRF905_PIN_UNUSED, // TRX (RX/TX mode) pin must be connected to VCC (3.3V) (force TX mode)
		8, // PWR
		NRF905_PIN_UNUSED, // Without the CD pin collision avoidance will be disabled
		NRF905_PIN_UNUSED, // Without the DR pin the library will run in polling mode and poll the status register over SPI. This also means the nRF905 can not wake the MCU up from sleep mode
		NRF905_PIN_UNUSED, // Without the AM pin the library must poll the status register over SPI.
		NULL, // Running in polling mode so no interrupt function
		NULL // Running in polling mode so no interrupt function
	);

	// Register event functions
	transceiver.events(
		NULL,
		NULL,
		nRF905_onTxComplete,
		NULL
	);

	// Low-mid transmit level -2dBm (631uW)
	transceiver.setTransmitPower(NRF905_PWR_n2);

	Serial.println(F("Sensor node started"));
}

void loop()
{
	digitalWrite(LED, HIGH);

	uint8_t buffer[NRF905_MAX_PAYLOAD];
	
	// Read some digital pins and analog values
	int val1 = analogRead(A0);
	int val2 = analogRead(A1);
	int val3 = analogRead(A2);
	
	buffer[0] = NODE_ID;
	buffer[1] = digitalRead(5);
	buffer[2] = digitalRead(7);
	buffer[3] = digitalRead(9);
	buffer[4] = val1>>8;
	buffer[5] = val1;
	buffer[6] = val2>>8;
	buffer[7] = val2;
	buffer[8] = val3>>8;
	buffer[9] = val3;
	
	Serial.print(F("Analog values: "));
	Serial.print(val1);
	Serial.print(F(" "));
	Serial.print(val2);
	Serial.print(F(" "));
	Serial.println(val3);
	
	Serial.print(F("Digital values: "));
	Serial.print(buffer[1]);
	Serial.print(F(" "));
	Serial.print(buffer[2]);
	Serial.print(F(" "));
	Serial.println(buffer[3]);

	Serial.println("---");

	// Write data to radio
	transceiver.write(BASE_STATION_ADDR, buffer, sizeof(buffer));

	txDone = false;

	// This will power-up the radio and send the data
	transceiver.TX(NRF905_NEXTMODE_TX, false);

	// Transmission will take approx 6-7ms to complete (smaller paylaod sizes will be faster)
	while(!txDone)
		transceiver.poll();

	// Transmission done, power-down the radio
	transceiver.powerDown();
	
	// NOTE:
	// After the payload has been sent the radio will continue to transmit an empty carrier wave until .powerDown() is called. Since this is a sensor node that doesn't need to receive data, only transmit and go into low power mode, this example hard wires the radio into TX mode (TXE connected to VCC) to reduce number of connections to the Arduino.
	
	digitalWrite(LED, LOW);

	// TODO
	// Sleep for 64 seconds
	//uint8_t sleepCounter = 8;
	//while(sleepCounter--)
	// Sleep for 1 second
		LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);
}
