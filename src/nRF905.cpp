/*
 * Project: nRF905 Radio Library for Arduino
 * Author: Zak Kemble, contact@zakkemble.net
 * Copyright: (C) 2020 by Zak Kemble
 * License: GNU GPL v3 (see License.txt)
 * Web: https://blog.zakkemble.net/nrf905-avrarduino-librarydriver/
 */

#include <Arduino.h>
#include <SPI.h>
#include <stdint.h>
#include "nRF905.h"
#include "nRF905_config.h"
#include "nRF905_defs.h"

#if defined(ESP32)
	#warning "ESP32 platforms don't seem to have a way of disabling and enabling interrupts. Try to avoid accessing the SPI bus from within the nRF905 event functions. That is, don't read the payload from inside the rxComplete event and make sure to connect the AM pin. See https://github.com/zkemble/nRF905-arduino/issues/1"
#endif

static const uint8_t config[] PROGMEM = {
	NRF905_CMD_W_CONFIG,
	NRF905_CHANNEL,
	NRF905_AUTO_RETRAN | NRF905_LOW_RX | NRF905_PWR | NRF905_BAND | ((NRF905_CHANNEL>>8) & 0x01),
	(NRF905_ADDR_SIZE_TX<<4) | NRF905_ADDR_SIZE_RX,
	NRF905_PAYLOAD_SIZE_RX,
	NRF905_PAYLOAD_SIZE_TX,
	(uint8_t)NRF905_ADDRESS, (uint8_t)(NRF905_ADDRESS>>8), (uint8_t)(NRF905_ADDRESS>>16), (uint8_t)(NRF905_ADDRESS>>24),
	NRF905_CRC | NRF905_CLK_FREQ | NRF905_OUTCLK
};

inline uint8_t nRF905::cselect()
{
#if defined(ESP32) || defined(ESP8266)
	if(!isrBusy)
		noInterrupts();
#endif
	spi.beginTransaction(spiSettings);
	digitalWrite(csn, LOW);
	return 1;
}

inline uint8_t nRF905::cdeselect()
{
	digitalWrite(csn, HIGH);
	spi.endTransaction();
#if defined(ESP32) || defined(ESP8266)
	if(!isrBusy)
		interrupts();
#endif
	return 0;
}

// Can be in any mode to write registers, but standby or power-down is recommended
#define CHIPSELECT()	for(uint8_t _cs = cselect(); _cs; _cs = cdeselect())

uint8_t nRF905::readConfigRegister(uint8_t reg)
{
	uint8_t val = 0;
	CHIPSELECT()
	{
		spi.transfer(NRF905_CMD_R_CONFIG | reg);
		val = spi.transfer(NRF905_CMD_NOP);
	}
	return val;
}

void nRF905::writeConfigRegister(uint8_t reg, uint8_t val)
{
	CHIPSELECT()
	{
		spi.transfer(NRF905_CMD_W_CONFIG | reg);
		spi.transfer(val);
	}
}

void nRF905::setConfigReg1(uint8_t val, uint8_t mask, uint8_t reg)
{
	// TODO atomic read/write?
	writeConfigRegister(reg, (readConfigRegister(NRF905_REG_CONFIG1) & mask) | val);
}

void nRF905::setConfigReg2(uint8_t val, uint8_t mask, uint8_t reg)
{
	// TODO atomic read/write?
	writeConfigRegister(reg, (readConfigRegister(NRF905_REG_CONFIG2) & mask) | val);
}

void nRF905::defaultConfig()
{
	// Should be in standby mode

	// Set control registers
	CHIPSELECT()
	{
		for(uint8_t i=0;i<sizeof(config);i++)
			spi.transfer(pgm_read_byte(&((uint8_t*)config)[i]));
	}

	// Default transmit address
	// TODO is this really needed?
	CHIPSELECT()
	{
		spi.transfer(NRF905_CMD_W_TX_ADDRESS);
		for(uint8_t i=0;i<4;i++)
			spi.transfer(0xE7);
	}

	// Clear transmit payload
	// TODO is this really needed?
	CHIPSELECT()
	{
		spi.transfer(NRF905_CMD_W_TX_PAYLOAD);
		for(uint8_t i=0;i<NRF905_MAX_PAYLOAD;i++)
			spi.transfer(0x00);
	}

	if(pwr == NRF905_PIN_UNUSED)
	{
		// Clear DR by reading receive payload
		CHIPSELECT()
		{
			spi.transfer(NRF905_CMD_R_RX_PAYLOAD);
			for(uint8_t i=0;i<NRF905_MAX_PAYLOAD;i++)
				spi.transfer(NRF905_CMD_NOP);
		}
	}
}

// NOTE: SPI registers can still be accessed when in power-down mode
inline void nRF905::powerOn(bool val)
{
	if(pwr != NRF905_PIN_UNUSED)
		digitalWrite(pwr, val ? HIGH : LOW);
}

inline void nRF905::standbyMode(bool val)
{
	if(trx != NRF905_PIN_UNUSED)
		digitalWrite(trx, val ? LOW : HIGH);
}

inline void nRF905::txMode(bool val)
{
	if(tx != NRF905_PIN_UNUSED)
		digitalWrite(tx, val ? HIGH : LOW);
}

void nRF905::setAddress(uint32_t address, uint8_t cmd)
{
	CHIPSELECT()
	{
		spi.transfer(cmd);
		//spi.transfer16(address);
		//spi.transfer16(address>>16);
		for(uint8_t i=0;i<4;i++)
			spi.transfer(address>>(8 * i));
		//spi.transfer(&address, 4);
	}
}

uint8_t nRF905::readStatus()
{
	uint8_t status = 0;
	CHIPSELECT()
		status = spi.transfer(NRF905_CMD_NOP);
	return status;
}

// TODO not used
/*
bool nRF905::dataReady()
{
	if(dr_mode == 1)
		return (readStatus() & (1<<NRF905_STATUS_DR));
	return digitalRead(dr);
}
*/

bool nRF905::addressMatched()
{
	if(am == NRF905_PIN_UNUSED)
		return (readStatus() & (1<<NRF905_STATUS_AM));
	return digitalRead(am);
}

nRF905::nRF905()
{
}

void nRF905::begin(
	SPIClass spi,
	uint32_t spiClock,
	int csn,
	int trx,
	int tx,
	int pwr,
	int cd,
	int dr,
	int am,
	void (*callback_interrupt_dr)(),
	void (*callback_interrupt_am)()
)
{
	this->spi = spi;
	this->spiSettings = SPISettings(spiClock, MSBFIRST, SPI_MODE0);

	this->csn = csn;
	this->trx = trx;
	this->tx = tx;
	this->pwr = pwr;
	this->cd = cd;
	this->dr = dr;
	this->am = am;

	digitalWrite(csn, HIGH);
	pinMode(csn, OUTPUT);
	if(trx != NRF905_PIN_UNUSED)
		pinMode(trx, OUTPUT);
	if(tx != NRF905_PIN_UNUSED)
		pinMode(tx, OUTPUT);
	if(pwr != NRF905_PIN_UNUSED)
		pinMode(pwr, OUTPUT);

#if !defined(ESP32) && !defined(ESP8266)
	// SPI.usingInterrupts() is not supported on ESP platforms. More info: https://github.com/zkemble/nRF905-arduino/issues/1
	if(dr != NRF905_PIN_UNUSED)
		spi.usingInterrupt(digitalPinToInterrupt(dr));
	if(am != NRF905_PIN_UNUSED)
		spi.usingInterrupt(digitalPinToInterrupt(am));
#endif

	powerOn(false);
	standbyMode(true);
	txMode(false);
	delay(3);
	defaultConfig();

	if(dr == NRF905_PIN_UNUSED || callback_interrupt_dr == NULL)
		polledMode = true;
	else
	{
		attachInterrupt(digitalPinToInterrupt(dr), callback_interrupt_dr, RISING);
		if(am != NRF905_PIN_UNUSED && callback_interrupt_am != NULL)
			attachInterrupt(digitalPinToInterrupt(am), callback_interrupt_am, CHANGE);
	}
}

void nRF905::otherSPIinterrupts()
{
#if !defined(ESP32) && !defined(ESP8266)
	spi.usingInterrupt(255);
#endif
}

void nRF905::events(
	void (*onRxComplete)(nRF905* device),
	void (*onRxInvalid)(nRF905* device),
	void (*onTxComplete)(nRF905* device),
	void (*onAddrMatch)(nRF905* device)
)
{
	this->onRxComplete = onRxComplete;
	this->onRxInvalid = onRxInvalid;
	this->onTxComplete = onTxComplete;
	this->onAddrMatch = onAddrMatch;
}

void nRF905::setChannel(uint16_t channel)
{
	if(channel > 511)
		channel = 511;

	// TODO atomic read/write?
	uint8_t reg = (readConfigRegister(NRF905_REG_CONFIG1) & NRF905_MASK_CHANNEL) | (channel>>8);

	CHIPSELECT()
	{
		spi.transfer(NRF905_CMD_W_CONFIG | NRF905_REG_CHANNEL);
		spi.transfer(channel);
		spi.transfer(reg);
	}
}

void nRF905::setBand(nRF905_band_t band)
{
	// TODO atomic read/write?
	uint8_t reg = (readConfigRegister(NRF905_REG_CONFIG1) & NRF905_MASK_BAND) | band;

	CHIPSELECT()
	{
		spi.transfer(NRF905_CMD_W_CONFIG | NRF905_REG_CONFIG1);
		spi.transfer(reg);
	}
}

void nRF905::setAutoRetransmit(bool val)
{
	setConfigReg1(
		val ? NRF905_AUTO_RETRAN_ENABLE : NRF905_AUTO_RETRAN_DISABLE,
		NRF905_MASK_AUTO_RETRAN,
		NRF905_REG_AUTO_RETRAN
	);
}

void nRF905::setLowRxPower(bool val)
{
	setConfigReg1(
		val ? NRF905_LOW_RX_ENABLE : NRF905_LOW_RX_DISABLE,
		NRF905_MASK_LOW_RX,
		NRF905_REG_LOW_RX
	);
}

void nRF905::setTransmitPower(nRF905_pwr_t val)
{
	setConfigReg1(val, NRF905_MASK_PWR, NRF905_REG_PWR);
}

void nRF905::setCRC(nRF905_crc_t val)
{
	setConfigReg2(val, NRF905_MASK_CRC, NRF905_REG_CRC);
}

void nRF905::setClockOut(nRF905_outclk_t val)
{
	setConfigReg2(val, NRF905_MASK_OUTCLK, NRF905_REG_OUTCLK);
}

void nRF905::setPayloadSize(uint8_t sizeTX, uint8_t sizeRX)
{
	if(sizeTX > NRF905_MAX_PAYLOAD)
		sizeTX = NRF905_MAX_PAYLOAD;

	if(sizeRX > NRF905_MAX_PAYLOAD)
		sizeRX = NRF905_MAX_PAYLOAD;

	CHIPSELECT()
	{
		spi.transfer(NRF905_CMD_W_CONFIG | NRF905_REG_RX_PAYLOAD_SIZE);
		spi.transfer(sizeRX);
		spi.transfer(sizeTX);
	}
}

void nRF905::setAddressSize(uint8_t sizeTX, uint8_t sizeRX)
{
	if(sizeTX != 1 && sizeTX != 4)
		sizeTX = 4;

	if(sizeRX != 1 && sizeRX != 4)
		sizeRX = 4;

	CHIPSELECT()
	{
		spi.transfer(NRF905_CMD_W_CONFIG | NRF905_REG_ADDR_WIDTH);
		spi.transfer((sizeTX<<4) | sizeRX);
	}
}

bool nRF905::receiveBusy()
{
	return addressMatched();
}

bool nRF905::airwayBusy()
{
	if(cd != NRF905_PIN_UNUSED)
		return digitalRead(cd);
	return false;
}

void nRF905::setListenAddress(uint32_t address)
{
	setAddress(address, NRF905_CMD_W_CONFIG | NRF905_REG_RX_ADDRESS);
}

void nRF905::write(uint32_t sendTo, void* data, uint8_t len)
{
	setAddress(sendTo, NRF905_CMD_W_TX_ADDRESS);

	if(len > 0 && data != NULL)
	{
		if(len > NRF905_MAX_PAYLOAD)
			len = NRF905_MAX_PAYLOAD;

		CHIPSELECT()
		{
			spi.transfer(NRF905_CMD_W_TX_PAYLOAD);
			//spi.transfer(data, len);
			for(uint8_t i=0;i<len;i++)
				spi.transfer(((uint8_t*)data)[i]);
		}
	}
}

void nRF905::read(void* data, uint8_t len)
{
	if(len > NRF905_MAX_PAYLOAD)
		len = NRF905_MAX_PAYLOAD;

	CHIPSELECT()
	{
		spi.transfer(NRF905_CMD_R_RX_PAYLOAD);

		// Get received payload
		//spi.transfer(data, len);
		for(uint8_t i=0;i<len;i++)
			((uint8_t*)data)[i] = spi.transfer(NRF905_CMD_NOP);

		// Must make sure all of the payload has been read, otherwise DR never goes low
		//uint8_t remaining = NRF905_MAX_PAYLOAD - len;
		//while(remaining--)
		//	spi.transfer(NRF905_CMD_NOP);
	}
}
/*
uint32_t nRF905::readUInt32() // TODO
{
	uint32_t data;
	CHIPSELECT()
	{
		spi.transfer(NRF905_CMD_R_RX_PAYLOAD);

		// Get received payload
		for(uint8_t i=0;i<4;i++)
			((uint8_t*)&data)[i] = spi.transfer(NRF905_CMD_NOP);
	}
	return data;
}

uint8_t nRF905::readUInt8() // TODO
{
	uint8_t data;
	CHIPSELECT()
	{
		spi.transfer(NRF905_CMD_R_RX_PAYLOAD);

		// Get received payload
		data = spi.transfer(NRF905_CMD_NOP);
	}
	return data;
}
*/
/*
size_t nRF905::write(uint8_t data)
{
	return 1;
}

size_t nRF905::write(const uint8_t *data, size_t quantity)
{
	write(0xE7E7E7E7, data, quantity);
	return quantity;
}

int nRF905::available()
{
	return 1;
}

int nRF905::read()
{
	return 1;
}

int nRF905::peek()
{
	return 1;
}

void nRF905::flush()
{
	// XXX: to be implemented.
}
*/
bool nRF905::TX(nRF905_nextmode_t nextMode, bool collisionAvoid)
{
	// TODO check DR is low?
	// check AM for incoming packet?
	// what if already in TX mode? (auto-retransmit or carrier wave)

	nRF905_mode_t currentMode = mode();
	if(currentMode == NRF905_MODE_POWERDOWN)
	{
		currentMode = NRF905_MODE_STANDBY;
		standbyMode(true);
		powerOn(true);
		if(nextMode != NRF905_NEXTMODE_TX)
			delay(3); // Delay is needed to the radio has time to power-up and see the standby/TX pins pulse
	}
	else if(collisionAvoid && airwayBusy())
		return false;

	// Put into transmit mode
	txMode(true); //PORTB |= _BV(PORTB1);

	// Pulse standby pin to start transmission
	if(currentMode == NRF905_MODE_STANDBY)
		standbyMode(false); //PORTD |= _BV(PORTD7);
	
	// NOTE: If nextMode is RX or STANDBY and a long running interrupt happens during the delays below then
	// we may end up transmitting a blank carrier wave until the interrupt ends.
	// If nextMode is RX then an unexpected onTxComplete event will also fire and RX mode won't be entered until the interrupt finishes.

	if(nextMode == NRF905_NEXTMODE_RX)
	{
		// 1.	The datasheets says that the radio can switch straight to RX mode after
		//	a transmission is complete by clearing TX_EN while transmitting, but if the radio was
		//	in standby mode and TX_EN is cleared within ~700us then the transmission seems to get corrupt.
		// 2.	Going straight to RX also stops DR from pulsing after transmission complete which means the onTxComplete event doesn't work
		if(currentMode == NRF905_MODE_STANDBY)
		{
			// Use micros() timing here instead of delayMicroseconds() to get better accuracy incase other interrupts happen (which would cause delayMicroseconds() to pause)
			unsigned int start = micros();
			while((unsigned int)(micros() - start) < 700);
		}
		else
			delayMicroseconds(14);
		txMode(false); //PORTB &= ~_BV(PORTB1);
	}
	else if(nextMode == NRF905_NEXTMODE_STANDBY)
	{
		delayMicroseconds(14);
		standbyMode(true);
		//txMode(false);
	}
	// else NRF905_NEXTMODE_TX

	return true;
}

void nRF905::RX()
{
	txMode(false);
	standbyMode(false);
	powerOn(true);
}

void nRF905::powerDown()
{
	powerOn(false);
}

void nRF905::standby()
{
	standbyMode(true);
	powerOn(true);
}

nRF905_mode_t nRF905::mode()
{
	if(pwr != NRF905_PIN_UNUSED)
	{
		if(!digitalRead(pwr))
			return NRF905_MODE_POWERDOWN;
	}
	
	if(trx != NRF905_PIN_UNUSED)
	{
		if(!digitalRead(trx))
			return NRF905_MODE_STANDBY;
	}

	if(tx != NRF905_PIN_UNUSED)
	{
		if(digitalRead(tx))
			return NRF905_MODE_TX;
		return NRF905_MODE_RX;
	}

	return NRF905_MODE_ACTIVE;
}
/*
bool nRF905::inStandbyMode()
{
	if(trx != NRF905_PIN_UNUSED)
		return !digitalRead(trx);
	return false;
}

bool nRF905::poweredUp()
{
	if(pwr != NRF905_PIN_UNUSED)
		return digitalRead(pwr);
	return true;
}
*/
void nRF905::getConfigRegisters(void* regs)
{
	CHIPSELECT()
	{
		spi.transfer(NRF905_CMD_R_CONFIG);
		for(uint8_t i=0;i<NRF905_REGISTER_COUNT;i++)
			((uint8_t*)regs)[i] = spi.transfer(NRF905_CMD_NOP);
	}
}

void nRF905::interrupt_dr()
{
	// If DR && AM = RX new packet
	// If DR && !AM = TX finished
	
#if defined(ESP32) || defined(ESP8266)
	isrBusy = 1;
#endif

	if(addressMatched())
	{
		validPacket = 1;
		if(onRxComplete != NULL)
			onRxComplete(this);
	}
	else
	{
		if(onTxComplete != NULL)
			onTxComplete(this);
	}

#if defined(ESP32) || defined(ESP8266)
	isrBusy = 0;
#endif
}

void nRF905::interrupt_am()
{
	// If AM goes HIGH then LOW without DR going HIGH then we got a bad packet

#if defined(ESP32) || defined(ESP8266)
	isrBusy = 1;
#endif

	if(addressMatched())
	{
		if(onAddrMatch != NULL)
			onAddrMatch(this);
	}
	else if(!validPacket)
	{
		if(onRxInvalid != NULL)
			onRxInvalid(this);
	}
	validPacket = 0;

#if defined(ESP32) || defined(ESP8266)
	isrBusy = 0;
#endif
}

void nRF905::poll()
{
	if(!polledMode)
		return;

	static uint8_t lastState;
	static uint8_t addrMatch;

// TODO read pins if am / dr defined

	uint8_t state = readStatus() & ((1<<NRF905_STATUS_DR)|(1<<NRF905_STATUS_AM));

	if(state != lastState)
	{
		if(state == ((1<<NRF905_STATUS_DR)|(1<<NRF905_STATUS_AM)))
		{
			addrMatch = 0;
			if(onRxComplete != NULL)
				onRxComplete(this);
		}
		else if(state == (1<<NRF905_STATUS_DR))
		{
			addrMatch = 0;
			if(onTxComplete != NULL)
				onTxComplete(this);
		}
		else if(state == (1<<NRF905_STATUS_AM))
		{
			addrMatch = 1;
			if(onAddrMatch != NULL)
				onAddrMatch(this);
		}
		else if(state == 0 && addrMatch)
		{
			addrMatch = 0;
			if(onRxInvalid != NULL)
				onRxInvalid(this);
		}
		
		lastState = state;
	}
}
