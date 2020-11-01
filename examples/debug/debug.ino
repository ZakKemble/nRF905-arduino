/*
 * Project: nRF905 Radio Library for Arduino (Debug example)
 * Author: Zak Kemble, contact@zakkemble.net
 * Copyright: (C) 2020 by Zak Kemble
 * License: GNU GPL v3 (see License.txt)
 * Web: https://blog.zakkemble.net/nrf905-avrarduino-librarydriver/
 */

/*
 * Read configuration registers
 */

#include <nRF905.h>
#include <nRF905_defs.h>
#include <SPI.h>

nRF905 transceiver = nRF905();

// Don't modify these 2 functions. They just pass the DR/AM interrupt to the correct nRF905 instance.
void nRF905_int_dr(){transceiver.interrupt_dr();}
void nRF905_int_am(){transceiver.interrupt_am();}

void setup()
{
	Serial.begin(115200);

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

	Serial.println(F("Started"));
}

void loop()
{
	byte regs[NRF905_REGISTER_COUNT];
	transceiver.getConfigRegisters(regs);

	Serial.print(F("Raw: "));

	byte dataInvalid = 0;

	for(byte i=0;i<NRF905_REGISTER_COUNT;i++)
	{
		Serial.print(regs[i]);
		Serial.print(F(" "));
		if(regs[i] == 0xFF || regs[i] == 0x00)
			dataInvalid++;
	}

	Serial.println();

	// Registers were all 0xFF or 0x00,  this is probably bad
	if(dataInvalid >= NRF905_REGISTER_COUNT)
	{
		Serial.println(F("All registers read as 0xFF or 0x00! Is the nRF905 connected correctly?"));
		delay(1000);
		return;
	}

	char* str;
	byte data;

	uint16_t channel = ((uint16_t)(regs[1] & 0x01)<<8) | regs[0];
	uint32_t freq = (422400UL + (channel * 100UL)) * (1 + ((regs[1] & ~NRF905_MASK_BAND) >> 1));

	Serial.print(F("Channel:          "));
	Serial.println(channel);
	Serial.print(F("Freq:             "));
	Serial.print(freq);
	Serial.println(F(" KHz"));
	Serial.print(F("Auto retransmit:  "));
	Serial.println((regs[1] & ~NRF905_MASK_AUTO_RETRAN) ? F("Enabled") : F("Disabled"));
	Serial.print(F("Low power RX:     "));
	Serial.println((regs[1] & ~NRF905_MASK_LOW_RX) ? F("Enabled") : F("Disabled"));

	// TX power
	data = regs[1] & ~NRF905_MASK_PWR;
	switch(data)
	{
		case NRF905_PWR_n10:
			data = -10;
			break;
		case NRF905_PWR_n2:
			data = -2;
			break;
		case NRF905_PWR_6:
			data = 6;
			break;
		case NRF905_PWR_10:
			data = 10;
			break;
		default:
			data = -127;
			break;
	}
	Serial.print(F("TX Power:         "));
	Serial.print((signed char)data);
	Serial.println(F(" dBm"));

	// Freq band
	data = regs[1] & ~NRF905_MASK_BAND;
	switch(data)
	{
		case NRF905_BAND_433:
			str = (char*)"433";
			break;
		default:
			str = (char*)"868/915";
			break;
	}
	Serial.print(F("Band:             "));
	Serial.print(str);
	Serial.println(F(" MHz"));

	Serial.print(F("TX Address width: "));
	Serial.println(regs[2] >> 4);
	Serial.print(F("RX Address width: "));
	Serial.println(regs[2] & 0x07);

	Serial.print(F("RX Payload size:  "));
	Serial.println(regs[3]);
	Serial.print(F("TX Payload size:  "));
	Serial.println(regs[4]);

	Serial.print(F("RX Address [0]:   "));
	Serial.println(regs[5]);
	Serial.print(F("RX Address [1]:   "));
	Serial.println(regs[6]);
	Serial.print(F("RX Address [2]:   "));
	Serial.println(regs[7]);
	Serial.print(F("RX Address [3]:   "));
	Serial.println(regs[8]);
	Serial.print(F("RX Address:       "));
	Serial.println(((unsigned long)regs[8]<<24 | (unsigned long)regs[7]<<16 | (unsigned long)regs[6]<<8 | (unsigned long)regs[5]));

	// CRC mode
	data = regs[9] & ~NRF905_MASK_CRC;
	switch(data)
	{
		case NRF905_CRC_16:
			str = (char*)"16 bit";
			break;
		case NRF905_CRC_8:
			str = (char*)"8 bit";
			break;
		default:
			str = (char*)"Disabled";
			break;
	}
	Serial.print(F("CRC Mode:         "));
	Serial.println(str);

	// Xtal freq
	data = regs[9] & ~NRF905_MASK_CLK;
	switch(data)
	{
		case NRF905_CLK_4MHZ:
			data = 4;
			break;
		case NRF905_CLK_8MHZ:
			data = 8;
			break;
		case NRF905_CLK_12MHZ:
			data = 12;
			break;
		case NRF905_CLK_16MHZ:
			data = 16;
			break;
		case NRF905_CLK_20MHZ:
			data = 20;
			break;
		default:
			data = 0;
			break;
	}
	Serial.print(F("Xtal freq:        "));
	Serial.print(data);
	Serial.println(" MHz");

	// Clock out freq
	data = regs[9] & ~NRF905_MASK_OUTCLK;
	switch(data)
	{
		case NRF905_OUTCLK_4MHZ:
			str = (char*)"4 MHz";
			break;
		case NRF905_OUTCLK_2MHZ:
			str = (char*)"2 MHz";
			break;
		case NRF905_OUTCLK_1MHZ:
			str = (char*)"1 MHz";
			break;
		case NRF905_OUTCLK_500KHZ:
			str = (char*)"500 KHz";
			break;
		default:
			str = (char*)"Disabled";
			break;
	}
	Serial.print(F("Clock out freq:   "));
	Serial.println(str);

	Serial.println(F("---------------------"));

	delay(1000);
}
