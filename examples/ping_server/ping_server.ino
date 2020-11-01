/*
 * Project: nRF905 Radio Library for Arduino (Ping server example)
 * Author: Zak Kemble, contact@zakkemble.net
 * Copyright: (C) 2020 by Zak Kemble
 * License: GNU GPL v3 (see License.txt)
 * Web: https://blog.zakkemble.net/nrf905-avrarduino-librarydriver/
 */

/*
 * Listen for packets and send them back
 */

#include <nRF905.h>
#include <SPI.h>

#define RXADDR 0xE7E7E7E7 // Address of this device
#define TXADDR 0xE7E7E7E7 // Address of device to send to

#define PACKET_NONE		0
#define PACKET_OK		1
#define PACKET_INVALID	2

#define LED				A5
#define PAYLOAD_SIZE	NRF905_MAX_PAYLOAD

nRF905 transceiver = nRF905();

static volatile uint8_t packetStatus;

// Don't modify these 2 functions. They just pass the DR/AM interrupt to the correct nRF905 instance.
void nRF905_int_dr(){transceiver.interrupt_dr();}
void nRF905_int_am(){transceiver.interrupt_am();}

// Event function for RX complete
void nRF905_onRxComplete(nRF905* device)
{
	packetStatus = PACKET_OK;
	transceiver.standby();
}

// Event function for RX invalid
void nRF905_onRxInvalid(nRF905* device)
{
	packetStatus = PACKET_INVALID;
	transceiver.standby();
}

void setup()
{
	Serial.begin(115200);

	Serial.println(F("Server starting..."));

	pinMode(LED, OUTPUT);

	// This must be called first
	SPI.begin();

	transceiver.begin(
		SPI, // SPI bus to use (SPI, SPI1, SPI2 etc)
		10000000, // SPI Clock speed (10MHz)
		6, // SPI SS
		7, // CE (standby)
		9, // TRX (RX/TX mode)
		8, // PWR (power down)
		4, // CD (collision avoid)
		3, // DR (data ready)
		2, // AM (address match)
		nRF905_int_dr, // Interrupt function for DR
		nRF905_int_am // Interrupt function for AM
	);
	
	// Register event functions
	transceiver.events(
		nRF905_onRxComplete,
		nRF905_onRxInvalid,
		NULL,
		NULL
	);
	
	// Set address of this device
	transceiver.setListenAddress(RXADDR);

	// Put into receive mode
	transceiver.RX();

	Serial.println(F("Server started"));
}

void loop()
{
	static uint32_t pings;
	static uint32_t invalids;
	static uint32_t badData;

	Serial.println(F("Waiting for ping..."));

	// Wait for data
	while(packetStatus == PACKET_NONE);

	if(packetStatus != PACKET_OK)
	{
		invalids++;
		packetStatus = PACKET_NONE;
		Serial.println(F("Invalid packet!"));
		transceiver.RX();
	}
	else
	{
		pings++;
		packetStatus = PACKET_NONE;
		
		Serial.println(F("Got ping"));

		// Toggle LED
		digitalWrite(LED, digitalRead(LED) ? LOW : HIGH);

		// Make buffer for data
		uint8_t buffer[PAYLOAD_SIZE];

		// Read payload
		transceiver.read(buffer, sizeof(buffer));

		// Copy data into new buffer for modifying
		uint8_t replyBuffer[PAYLOAD_SIZE];
		memcpy(replyBuffer, buffer, PAYLOAD_SIZE);

		// Validate data and modify
		// Each byte ofthe payload should be the same value, increment this value and send back to the client
		bool dataIsBad = false;
		uint8_t value = replyBuffer[0];
		for(uint8_t i=0;i<PAYLOAD_SIZE;i++)
		{
			if(replyBuffer[i] == value)
				replyBuffer[i]++;
			else
			{
				badData++;
				dataIsBad = true;
				break;
			}
		}

		// Write reply data and destination address to radio
		transceiver.write(TXADDR, replyBuffer, sizeof(replyBuffer));

		// Send the reply data, once the transmission has completed go into receive mode
		while(!transceiver.TX(NRF905_NEXTMODE_RX, true));

		Serial.println(F("Reply sent"));
		
		if(dataIsBad)
			Serial.println(F("Received data was bad!"));

		// Show received data
		Serial.print(F("Data from client:"));
		for(uint8_t i=0;i<PAYLOAD_SIZE;i++)
		{
			Serial.print(F(" "));
			Serial.print(buffer[i], DEC);
		}
		Serial.println();

		// Show sent data
		Serial.print(F("Reply data:"));
		for(uint8_t i=0;i<PAYLOAD_SIZE;i++)
		{
			Serial.print(F(" "));
			Serial.print(replyBuffer[i], DEC);
		}
		Serial.println();
	}

	Serial.println(F("Totals:"));
	Serial.print(F(" Pings   "));
	Serial.println(pings);
	Serial.print(F(" Invalid "));
	Serial.println(invalids);
	Serial.print(F(" Bad     "));
	Serial.println(badData);
	Serial.println(F("------"));
}
