/*
 * Project: nRF905 Radio Library for Arduino (Low power node base station example)
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
// DR -> 3

// The following pins on the nRF905 must be connected to VCC (3.3V) or GND:
// CE (TRX_EN) -> VCC
// TXE (TX_EN) -> GND
// PWR -> VCC

// The nRF905 will always be in low-power receive mode, waiting for packets from sensor nodes.

#include <nRF905.h>
#include <SPI.h>
#include <LowPower.h>

#define BASE_STATION_ADDR	0xE7E7E7E7
#define PAYLOAD_SIZE		NRF905_MAX_PAYLOAD
#define LED					A5

#define PACKET_NONE		0
#define PACKET_OK		1
#define PACKET_INVALID	2

nRF905 transceiver = nRF905();

static volatile uint8_t packetStatus; // NOTE: In interrupt mode this must be volatile

void nRF905_int_dr(){transceiver.interrupt_dr();}

void nRF905_onRxComplete(nRF905* device)
{
	packetStatus = PACKET_OK;
}

void nRF905_onRxInvalid(nRF905* device)
{
	packetStatus = PACKET_INVALID;
}

void setup()
{
	Serial.begin(115200);
	Serial.println(F("Base station starting..."));
	
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
	// Also avoid doing things in the event functions that take a long time to complete or things that rely on interrupts (delay(), millis(), Serial.print())).
	transceiver.events(
		nRF905_onRxComplete,
		nRF905_onRxInvalid,
		NULL,
		NULL
	);

	transceiver.setLowRxPower(true);

	Serial.println(F("Base station started"));
}

void loop()
{
	static uint32_t good;
	static uint32_t invalids;
	static uint32_t badData;

	digitalWrite(LED, HIGH);
	
	// Atomically copy volatile global variable to local variable since the radio is still in RX mode and another packet could come in at any moment
	noInterrupts();
	uint8_t pktStatus = packetStatus;
	packetStatus = PACKET_NONE;
	interrupts();

	if(pktStatus == PACKET_NONE)
	{
		Serial.println("Woke for no reason?");
	}
	else if(pktStatus == PACKET_INVALID)
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

		Serial.print(F("Analog values: "));
		Serial.print(buffer[4]<<8 | buffer[5]);
		Serial.print(F(" "));
		Serial.print(buffer[6]<<8 | buffer[7]);
		Serial.print(F(" "));
		Serial.println(buffer[8]<<8 | buffer[9]);
		
		Serial.print(F("Digital values: "));
		Serial.print(buffer[1]);
		Serial.print(F(" "));
		Serial.print(buffer[2]);
		Serial.print(F(" "));
		Serial.println(buffer[3]);
		
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
	if(packetStatus == PACKET_NONE) // Make sure no new packets were received while since the last wake
		LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
}
