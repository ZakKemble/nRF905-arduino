/*
 * Project: nRF905 Radio Library for Arduino
 * Author: Zak Kemble, contact@zakkemble.net
 * Copyright: (C) 2020 by Zak Kemble
 * License: GNU GPL v3 (see License.txt)
 * Web: https://blog.zakkemble.net/nrf905-avrarduino-librarydriver/
 */

#ifndef NRF905_H_
#define NRF905_H_

#include <Arduino.h>
#include <SPI.h>
#include <stdint.h>
#include "nRF905_config.h"

/**
* @brief Available modes after transmission complete.
*/
typedef enum
{
// TODO use nRF905_mode_t instead of nRF905_nextmode_t?

	NRF905_NEXTMODE_STANDBY, ///< Standby mode
	NRF905_NEXTMODE_RX, ///< Receive mode
	NRF905_NEXTMODE_TX ///< Transmit mode (will auto-retransmit if enabled, otherwise will transmit a carrier wave with no data)
} nRF905_nextmode_t;

/**
* @brief Current radio mode
*/
typedef enum
{
	NRF905_MODE_POWERDOWN, ///< Power-down mode
	NRF905_MODE_STANDBY, ///< Standby mode
	NRF905_MODE_RX, ///< Receive mode
	NRF905_MODE_TX, ///< Transmit mode
	NRF905_MODE_ACTIVE ///< Receive or transmit mode (when unable to tell due to the tx pin being unused, hardwired to VCC (TX) or GND (RX))
} nRF905_mode_t;

/**
* @brief Frequency bands.
*/
typedef enum
{
// NOTE:
// When using NRF905_BAND_868 and NRF905_BAND_915 for calculating channel (NRF905_CALC_CHANNEL(f, b)) they should be value 0x01,
// but when using them for setting registers their value should be 0x02.
// They're defined as 0x02 here so when used for calculating channel they're right shifted by 1

	NRF905_BAND_433 = 0x00,	///< 433MHz band
	NRF905_BAND_868 = 0x02,	///< 868/915MHz band
	NRF905_BAND_915 = 0x02	///< 868/915MHz band
} nRF905_band_t;

/**
* @brief Output power (n means negative, n10 = -10).
*/
typedef enum
{
	NRF905_PWR_n10 = 0x00,	///< -10dBm = 100uW
	NRF905_PWR_n2 = 0x04,	///< -2dBm = 631uW
	NRF905_PWR_6 = 0x08,	///< 6dBm = 4mW
	NRF905_PWR_10 = 0x0C	///< 10dBm = 10mW
} nRF905_pwr_t;

/**
* @brief Output a clock signal on pin 3 of IC.
*/
typedef enum
{
	NRF905_OUTCLK_DISABLE = 0x00,	///< Disable output clock
	NRF905_OUTCLK_4MHZ = 0x04,		///< 4MHz clock
	NRF905_OUTCLK_2MHZ = 0x05,		///< 2MHz clock
	NRF905_OUTCLK_1MHZ = 0x06,		///< 1MHz clock
	NRF905_OUTCLK_500KHZ = 0x07,	///< 500KHz clock (default)
} nRF905_outclk_t;

/**
* @brief CRC Checksum.
*
* The CRC is calculated across the address (SYNC word) and payload
*/
typedef enum
{
	NRF905_CRC_DISABLE = 0x00,	///< Disable CRC
	NRF905_CRC_8 = 0x40,		///< 8bit CRC (Don't know what algorithm is used for this one)
	NRF905_CRC_16 = 0xC0,		///< 16bit CRC (CRC16-CCITT-FALSE (0xFFFF))
} nRF905_crc_t;

#define NRF905_MAX_PAYLOAD		32 ///< Maximum payload size
#define NRF905_REGISTER_COUNT	10 ///< Configuration register count
#define NRF905_DEFAULT_RXADDR	0xE7E7E7E7 ///< Default receive address
#define NRF905_DEFAULT_TXADDR	0xE7E7E7E7 ///< Default transmit/destination address
#define NRF905_PIN_UNUSED		255 ///< Mark a pin as not used or not connected

#define NRF905_CALC_CHANNEL(f, b)	((((f) / (1 + (b>>1))) - 422400000UL) / 100000UL) ///< Workout channel from frequency & band

class nRF905 //: public Stream // TODO see Wire library
{
private:
	SPIClass spi;
	SPISettings spiSettings;

#if defined(ESP32) || defined(ESP8266)
	volatile uint8_t isrBusy;
#endif
	
	// Pins
	uint8_t csn; // SPI SS
	uint8_t trx; // Standby (CE/TRX_EN)
	uint8_t tx; // TX or RX mode (TXE/TX_EN)
	uint8_t pwr; // Power-down control (PWR)
	uint8_t cd; // Carrier detect (CD)
	uint8_t dr; // Data ready (DR)
	uint8_t am; // Address match (AM)
	
	// Events
	void (*onRxComplete)(nRF905* device);
	void (*onRxInvalid)(nRF905* device);
	void (*onTxComplete)(nRF905* device);
	void (*onAddrMatch)(nRF905* device);

	volatile uint8_t validPacket;
	bool polledMode;

	inline uint8_t cselect();
	inline uint8_t cdeselect();
	uint8_t readConfigRegister(uint8_t reg);
	void writeConfigRegister(uint8_t reg, uint8_t val);
	void setConfigReg1(uint8_t val, uint8_t mask, uint8_t reg);
	void setConfigReg2(uint8_t val, uint8_t mask, uint8_t reg);
	void defaultConfig();
	inline void powerOn(bool val);
	inline void standbyMode(bool val);
	inline void txMode(bool val);
	void setAddress(uint32_t address, uint8_t cmd);
	uint8_t readStatus();
	//bool dataReady();
	bool addressMatched();

public:
	/*virtual size_t write(uint8_t);
	virtual size_t write(const uint8_t *, size_t);
	virtual int available();
	virtual int read();
	virtual int peek();
	virtual void flush();
	using Print::write;*/

	nRF905();

/**
* @brief Initialise, must be called after SPI.begin() and before any other nRF905 stuff
*
* \p trx, \p tx, \p pwr, \p cd, \p dr and \p am pins are optional. Use ::NRF905_PIN_UNUSED if the pin is not connected, but some functionally will be lost.
*
* Have a look at the examples to see how things work (or not work) when some pins are not used. If \p tx is not used and connected to VCC (TX) then either \p trx or \p pwr must be used otherwise the radio will always be transmitting either an empty carrier wave or if auto-retransmit is enabled then the payload is retransmitted over and over again, though updating the payload on-the-fly does work in auto-retransmit mode if that's what you're after.
*
* If \p dr pin is not used or \p callback_interrupt_dr is \p NULL then the library will run in polling mode. See .poll().
*
* If \p am pin is not used then the onAddrMatch event will not work.
*
* @param [spi] SPI bus (usually SPI, SPI1, SPI2 etc)
* @param [spiClock] SPI clock rate, max 10000000 (10MHz)
* @param [csn] SPI Slave select pin
* @param [trx] CE/TRX_EN pin (standby control) (optional - must be connected to VCC if not used)
* @param [tx] TXE/TX_EN pin (RX/TX mode control) (optional - must be connected to VCC (TX) or GND (RX) if not used depending on what the radio should do)
* @param [pwr] PWR pin (power-down control) (optional - must be connected to VCC if not used)
* @param [cd] CD pin (carrier detect) (optional)
* @param [dr] DR pin (data ready) (optional)
* @param [am] AM pin (address match) (optional)
* @param [callback_interrupt_dr] Callback for DR interrupt (optional)
* @param [callback_interrupt_am] Callback for AM interrupt (optional)
*
* @return (none)
*/
	void begin(
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
	);

/**
* @brief If your sketch uses libraries that access the SPI bus from inside an interrupt then this method should be called after the .begin() method
*
* This will disable all interrupts when accessing the nRF905 via SPI instead of only disabling our own DR/AM pin interrupts. This will prevent other libraries from accessing the SPI at the same time as this library.
*
* Example: `transceiver.otherSPIinterrupts();`
*
* @return (none)
*/
	void otherSPIinterrupts();

/**
* @brief Register event functions
*
* Example: `transceiver.events(onRxComplete, onRxInvalid, onTxComplete, NULL);`
*
* @param [onRxComplete] Function to run on new payload received
* @param [onRxInvalid] Function to run on corrupted payload received
* @param [onTxComplete] Function to run on transmission completion (only works when calling .TX() with ::NRF905_NEXTMODE_TX or ::NRF905_NEXTMODE_STANDBY)
* @param [onAddrMatch] Function to run on address match (beginning of payload reception)
*
* @return (none)
*/
	void events(
		void (*onRxComplete)(nRF905* device),
		void (*onRxInvalid)(nRF905* device),
		void (*onTxComplete)(nRF905* device),
		void (*onAddrMatch)(nRF905* device)
	);

/**
* @brief Channel to listen and transmit on
*
* 433MHz band: Channel 0 is 422.4MHz up to 511 which is 473.5MHz (Each channel is 100KHz apart)
*
* 868/915MHz band: Channel 0 is 844.8MHz up to 511 which is 947MHz (Each channel is 200KHz apart)
*
* Example: `transceiver.setChannel(50);`
*
* @param [channel] The channel (0 - 511)
* @return (none)
*
* @see .setBand()
*/
	void setChannel(uint16_t channel);

/**
* @brief Frequency band
*
* Radio modules are usually only tuned for a specific band, either 433MHz or 868/915MHz. Using the wrong frequency band will result in very short range of less than a meter, sometimes they have to be touching each other before anything happens.
*
* Example: `transceiver.setBand(NRF905_BAND_433);`
*
* @param [band] Frequency band, see ::nRF905_band_t
* @return (none)
*
* @see .setChannel() ::nRF905_band_t
*/
	void setBand(nRF905_band_t band);

/**
* @brief Set auto-retransmit
*
* If \p nextMode is set to ::NRF905_NEXTMODE_TX when calling .TX() and auto-retransmit is enabled then the radio will continuously retransmit the payload. If auto-retransmit is disabled then a carrier wave with no data will be transmitted instead (kinda useless).\n
* Transmission will continue until the radio is put into standby, power-down or RX mode.
*
* Can be useful in areas with lots of interference, but you'll need to make sure you can differentiate between retransmitted packets and new packets (like an ID number).
*
* Other transmissions will be blocked if collision avoidance is enabled.
*
* Example: `transceiver.setAutoRetransmit(true);`
*
* @param [val] \p true = enable, \p false = disable
* @return (none)
*/
	void setAutoRetransmit(bool val);

/**
* @brief Set low power receive mode
*
* Reduces receive current from 12.2mA to 10.5mA, but also reduces sensitivity.
*
* Example: `transceiver.setLowRxPower(true);`
*
* @param [val] \p true = enable, \p false = disable
* @return (none)
*/
	void setLowRxPower(bool val);

/**
* @brief Set transmit output power
*
* Example: `transceiver.setTransmitPower(NRF905_PWR_10);`
*
* @param [val] Output power level, see ::nRF905_pwr_t
* @return (none)
*
* @see ::nRF905_pwr_t
*/
	void setTransmitPower(nRF905_pwr_t val);

/**
* @brief Set CRC algorithm
*
* Example: `transceiver.setCRC(NRF905_CRC_16);`
*
* @param [val] CRC Type, see ::nRF905_crc_t
* @return (none)
*
* @see ::nRF905_crc_t
*/
	void setCRC(nRF905_crc_t val);

/**
* @brief Set clock output
*
* Example: `transceiver.setClockOut(NRF905_OUTCLK_4MHZ);`
*
* @param [val] Clock out frequency, see ::nRF905_outclk_t
* @return (none)
*
* @see ::nRF905_outclk_t
*/
	void setClockOut(nRF905_outclk_t val);

/**
* @brief Payload sizes
*
* The nRF905 only supports fixed sized payloads. The receiving end must be configured for the same payload size as the transmitting end.
*
* Example: This will send 5 byte payloads in one direction, and 32 byte payloads in the other direction:\n
* `transceiver1.setPayloadSize(5, 32); // Will transmit 5 byte payloads, receive 32 byte payloads`\n
* `transceiver2.setPayloadSize(32, 5); // Will transmit 32 byte payloads, receive 5 byte payloads`
*
* @param [sizeTX] Transmit payload size (1 - 32)
* @param [sizeRX] Recivie payload size (1 - 32)
* @return (none)
*
* @see ::NRF905_MAX_PAYLOAD
*/
	void setPayloadSize(uint8_t sizeTX, uint8_t sizeRX);

/**
* @brief Address sizes
*
* The address is actually the SYNC part of the packet, just after the preamble and before the data. The SYNC word is used to mark the beginning of the payload data.
*
*  A 1 byte address is not recommended, a lot of false invalid packets will be received.
*
* Example: `transceiver.setAddressSize(4, 4);`
*
* @param [sizeTX] Transmit address size, either 1 or 4 bytes only
* @param [sizeRX] Receive address size, either 1 or 4 bytes only
* @return (none)
*/
	void setAddressSize(uint8_t sizeTX, uint8_t sizeRX);

/**
* @brief See if the address match (AM) is asserted
*
* Example: `if(transceiver.receiveBusy())`
*
* @return \p true if currently receiving payload or payload is ready to be read, otherwise \p false
*/
	bool receiveBusy();

/**
* @brief See if airway is busy (carrier detect pin asserted).
*
* Example: `if(transceiver.airwayBusy())`
*
* @return \p true if other transmissions detected, otherwise \p false
*/
	bool airwayBusy();

/**
* @brief Set address to listen to
*
* Some good addresses would be \p 0xA94EC554 and \p 0xB54CAB34, bad address would be \p 0xC201043F and \p 0xFF00FF00
*
* Example: `transceiver.setListenAddress(0xA94EC554);`
*
* @note From the datasheet: Each byte within the address should be unique. Repeating bytes within the address reduces the effectiveness of the address and increases its susceptibility to noise which increases the packet error rate. The address should also have several level shifts (that is, 10101100) reducing the statistical effect of noise and the packet error rate.
* @param [address] The address, a 32 bit integer (default address is 0xE7E7E7E7)
* @return (none)
*/
	void setListenAddress(uint32_t address);

/**
* @brief Write payload data and set destination address
*
* If \p data is \p NULL and/or \p len is \p 0 then the payload will not be modified, only the destination address will be updated. Useful to sending the same data to multiple addresses.
*
* If the radio is still transmitting then the payload and address will be updated as it is being sent, this means the payload on the receiving end may contain old and new data.\n
* This also means that a node may receive part of a payload that was meant for a node with a different address.\n
* Use the \p onTxComplete event to set a flag or something to ensure the transmission is complete before writing another payload. However this only works if \p nextMode is set to ::NRF905_NEXTMODE_TX or ::NRF905_NEXTMODE_STANDBY.
*
* Example: `transceiver.write(0xB54CAB34, buffer, sizeof(buffer));`
*
* @param [sendTo] Address to send the payload to
* @param [data] The data
* @param [len] Data length (max ::NRF905_MAX_PAYLOAD)
* @return (none)
*/
	void write(uint32_t sendTo, void* data, uint8_t len);

/**
* @brief Read received payload.
*
* This method can be called multiple times to read a few bytes at a time.\n
* The payload is cleared when the radio enters power-down, RX or TX mode. Entering standby mode from RX mode does not clear the payload.\n
* The radio will not receive any more data until all of the payload has been read or cleared.
*
* Example: `transceiver.read(buffer, 4);`
*
* @param [data] Buffer for the data
* @param [len] How many bytes to read
* @return (none)
*/
	void read(void* data, uint8_t len);

	//uint32_t readUInt32(); // TODO
	//uint8_t readUInt8(); // TODO
	//char readChar(); // TODO
	//uint8_t readString(char str, uint8_t size); // TODO

/**
* @brief Begin a transmission
*
* If the radio is in power-down mode then this function will take an additional 3ms to complete.\n
* If 3ms is too long then call .standby() and do whatever you need to do for at least 3ms before calling .TX().
*
* If \p nextMode is set to ::NRF905_NEXTMODE_RX and the radio was in standby mode then this function will take an additional 700us to complete.\n
* If 700us is too long then set \p nextMode to ::NRF905_NEXTMODE_STANDBY and call .RX() in the \p onTxComplete event instead.
*
* The onTxComplete event does not work when \p nextMode is set to ::NRF905_NEXTMODE_RX.
*
* For the collision avoidance to work the radio should be in RX mode for around 5ms before attempting to transmit.
*
* Example: `transceiver.TX(NRF905_NEXTMODE_STANDBY, true);`
*
* @param [nextMode] What mode to enter once the transmission is complete, see ::nRF905_nextmode_t
* @param [collisionAvoid] \p true = check for other transmissions before transmitting (CD pin must be connected), \p false = skip the check and just transmit
* @return \p false if collision avoidance is enabled and other transmissions are going on, \p true if transmission has successfully begun
*
* @see ::nRF905_nextmode_t
*/
	bool TX(nRF905_nextmode_t nextMode, bool collisionAvoid);

/**
* @brief Enter receive mode.
*
* If the radio is currently transmitting then receive mode will be entered once it has finished.
* This function will also automatically power-up the radio and leave standby mode.
*
* Example: `transceiver.RX();`
*
* @return (none)
*/
	void RX();

/**
* @brief Sleep.
*
* Current consumption will be 2.5uA in this mode.
* Cancels any ongoing transmissions and clears the RX payload.
*
* Example: `transceiver.powerDown();`
*
* @return (none)
*/
	void powerDown();

/**
* @brief Enter standby mode.
*
* Radio will wait for any ongoing transmissions to complete before entering standby mode.
*
* If the radio was in power-down mode then there must be a 3ms delay between entering standby mode and beginning a transmission using .TX() with \p nextMode as ::NRF905_NEXTMODE_STANDBY or ::NRF905_NEXTMODE_RX otherwise the transmission will not start.
* Calling .TX() while the radio is in power-down mode will automatically power-up the radio and wait 3ms before starting the transmission.
*
* Example: `transceiver.standby();`
*
* @return (none)
*/
	void standby();

/**
* @brief Get current radio mode
*
* Example: `if(transceiver.mode() == NRF905_MODE_POWERDOWN)`
*
* @return Current mode, see ::nRF905_mode_t
*
* @see ::nRF905_mode_t
*/
	nRF905_mode_t mode();

/**
* @brief Read configuration registers into byte array of ::NRF905_REGISTER_COUNT elements, mainly for debugging.
*
* Example: `transceiver.getConfigRegisters(regs);`
*
* @param [regs] Buffer for register data, size must be at least ::NRF905_REGISTER_COUNT bytes.
* @return (none)
*/
	void getConfigRegisters(void* regs);

/**
* @brief When running in interrupt mode this method must be called from the DR interrupt callback function.
*
* Example: `transceiver.interrupt_dr();`
*
* @return (none)
*/
	void interrupt_dr();

/**
* @brief When running in interrupt mode this method must be called from the AM interrupt callback function.
*
* Example: `transceiver.interrupt_am();`
*
* @return (none)
*/
	void interrupt_am();

/**
* @brief When running in polled mode this method should be called as often as possible.
*
* Maximum interval depends on how long it takes to receive a payload, which is around 6ms for 32 bytes, down to around 1.1ms for a 1 byte payload. If .poll() is not called often enough then events will be missed, the worse one being a missed \p onRxComplete event caused by reading a received payload, then another payload is received again without a .poll() call in between.
*
* Example: `transceiver.poll();`
*
* @return (none)
*/
	void poll();
};

#endif /* NRF905_H_ */
