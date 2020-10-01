/*
 * Project: nRF905 Radio Library for Arduino (Low power node base station example)
 * Author: Zak Kemble, contact@zakkemble.net
 * Copyright: (C) 2020 by Zak Kemble
 * License: GNU GPL v3 (see License.txt)
 * Web: https://blog.zakkemble.net/nrf905-avrarduino-librarydriver/
 */

// TODO requires 5 wires
// MOSI
// MISO
// SCK
// SS
// DR

// https://github.com/rocketscream/Low-Power

#include <nRF905.h>
#include <SPI.h>
#include <LowPower.h>

#define BASE_STATION_ADDR	0xE7E7E7E7
#define LED					A5
#define PAYLOAD_SIZE		NRF905_MAX_PAYLOAD

nRF905 transceiver = nRF905();

static volatile uint8_t packetStatus; // NOTE: In interrupt mode this must be volatile

void nRF905_int_dr(){transceiver.interrupt_dr();}

void nRF905_onRxComplete(nRF905* device)
{
	packetStatus = 1;
}

void nRF905_onRxInvalid(nRF905* device)
{
	packetStatus = 2;
}

void setup()
{
	Serial.begin(115200);
	Serial.println(F("Client started"));
	
	pinMode(LED, OUTPUT);


	// standby off TODO
	//pinMode(7, OUTPUT);
	//digitalWrite(7, HIGH);
	// pwr
	//pinMode(8, OUTPUT);
	//digitalWrite(8, HIGH);
	// trx
	//pinMode(9, OUTPUT);
	//digitalWrite(9, LOW);
	
	
	// This must be called first
	SPI.begin();

	// Minimal wires (interrupt mode)
	transceiver.begin(
		SPI,
		10000000,
		6,
		NRF905_PIN_UNUSED, // CE (standby) pin must be connected to VCC (3.3V)
		NRF905_PIN_UNUSED, // TRX (RX/TX mode) pin must be connected to GND (force RX mode)
		NRF905_PIN_UNUSED, // PWR (power-down) pin must be connected to VCC (3.3V)
		NRF905_PIN_UNUSED, // Without the CD pin collision avoidance will be disabled
		3, // DR
		NRF905_PIN_UNUSED, // Without the AM pin the library the library must poll the status register over SPI.
		nRF905_int_dr,
		NULL // No interrupt function
	);

	// Register event functions
	// NOTE: In interrupt mode these events will run from the interrupt, so any global variables accessed inside the event function should be declared 'volatile'.
	// Also avoid doing things that take a long time to complete (delay()) and avoid using things that rely on interrupts (millis(), Serial.print()).
	transceiver.events(
		nRF905_onRxComplete,
		nRF905_onRxInvalid,
		NULL,
		NULL
	);
	
	transceiver.setLowRxPower(true);
}

void loop()
{
	static uint32_t good;
	static uint32_t invalids;
	static uint32_t badData;

	digitalWrite(LED, HIGH);

	if(packetStatus == 0)
	{
		Serial.println("Woke for no reason?");
	}
	else if(packetStatus == 2)
	{
		Serial.println("Invalid packet");
		invalids++;
	}
	else
	{
		Serial.println("Got packet");
		
		// Make buffer for data
		uint8_t buffer[PAYLOAD_SIZE];

		// Read payload
		transceiver.read(buffer, sizeof(buffer));

		// Show received data
		Serial.print(F("Data from client:"));
		for(uint8_t i=0;i<PAYLOAD_SIZE;i++)
		{
			Serial.print(F(" "));
			Serial.print(buffer[i], DEC);
		}
		Serial.println();
		
		good++;
	}

	Serial.println(F("Totals:"));
	Serial.print(F(" Good    "));
	Serial.println(good);
	Serial.print(F(" Invalid "));
	Serial.println(invalids);
	Serial.print(F(" Bad     "));
	Serial.println(badData);
	Serial.println(F("------"));
	
	Serial.flush();
	
	digitalWrite(LED, LOW);

	// Sleep
	LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
}
