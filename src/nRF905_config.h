/*
 * Project: nRF905 Radio Library for Arduino
 * Author: Zak Kemble, contact@zakkemble.net
 * Copyright: (C) 2020 by Zak Kemble
 * License: GNU GPL v3 (see License.txt)
 * Web: https://blog.zakkemble.net/nrf905-avrarduino-librarydriver/
 */

#ifndef NRF905_CONFIG_H_
#define NRF905_CONFIG_H_

// Crystal frequency (the one the radio module is using)
// NRF905_CLK_4MHZ
// NRF905_CLK_8MHZ
// NRF905_CLK_12MHZ
// NRF905_CLK_16MHZ
// NRF905_CLK_20MHZ
#define NRF905_CLK_FREQ		NRF905_CLK_16MHZ


///////////////////
// Default radio settings
///////////////////

// Frequency
// Channel 0 is 422.4MHz for the 433MHz band, each channel increments the frequency by 100KHz, so channel 10 would be 423.4MHz
// Channel 0 is 844.8MHz for the 868/915MHz band, each channel increments the frequency by 200KHz, so channel 10 would be 846.8MHz
// Max channel is 511 (473.5MHz / 947.0MHz)
#define NRF905_CHANNEL			10

// Frequency band
// 868 and 915 are actually the same thing
// NRF905_BAND_433
// NRF905_BAND_868
// NRF905_BAND_915
#define NRF905_BAND			NRF905_BAND_433

// Output power
// n means negative, n10 = -10
// NRF905_PWR_n10 (-10dBm = 100uW)
// NRF905_PWR_n2 (-2dBm = 631uW)
// NRF905_PWR_6 (6dBm = 4mW)
// NRF905_PWR_10 (10dBm = 10mW)
#define NRF905_PWR			NRF905_PWR_10

// Save a few mA by reducing receive sensitivity
// NRF905_LOW_RX_DISABLE (Normal sensitivity)
// NRF905_LOW_RX_ENABLE (Lower sensitivity)
#define NRF905_LOW_RX		NRF905_LOW_RX_DISABLE

// Constantly retransmit payload while in transmit mode
// Can be useful in areas with lots of interference, but you'll need to make sure you can differentiate between re-transmitted packets and new packets (like an ID number).
// It will also block other transmissions if collision avoidance is enabled.
// NRF905_AUTO_RETRAN_DISABLE
// NRF905_AUTO_RETRAN_ENABLE
#define NRF905_AUTO_RETRAN	NRF905_AUTO_RETRAN_DISABLE

// Output a clock signal on pin 3 of IC
// NRF905_OUTCLK_DISABLE
// NRF905_OUTCLK_500KHZ
// NRF905_OUTCLK_1MHZ
// NRF905_OUTCLK_2MHZ
// NRF905_OUTCLK_4MHZ
#define NRF905_OUTCLK		NRF905_OUTCLK_DISABLE

// CRC checksum
// NRF905_CRC_DISABLE
// NRF905_CRC_8
// NRF905_CRC_16
#define NRF905_CRC			NRF905_CRC_16

// Address size
// The address is actually the SYNC part of the packet, just after the preamble and before the data.
// Size should be either 1 or 4 bytes.
// 1 byte is not recommended, a lot of false invalid packets will be received.
#define NRF905_ADDR_SIZE_RX	4
#define NRF905_ADDR_SIZE_TX	4

// Payload size (1 - 32) 
#define NRF905_PAYLOAD_SIZE_RX	32 //NRF905_MAX_PAYLOAD
#define NRF905_PAYLOAD_SIZE_TX	32 //NRF905_MAX_PAYLOAD

#define NRF905_ADDRESS		0xE7E7E7E7

#endif /* NRF905_CONFIG_H_ */
