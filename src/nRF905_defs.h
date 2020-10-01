/*
 * Project: nRF905 Radio Library for Arduino
 * Author: Zak Kemble, contact@zakkemble.net
 * Copyright: (C) 2020 by Zak Kemble
 * License: GNU GPL v3 (see License.txt)
 * Web: https://blog.zakkemble.net/nrf905-avrarduino-librarydriver/
 */

#ifndef NRF905_DEFS_H_
#define NRF905_DEFS_H_

// Instructions
#define NRF905_CMD_NOP			0xFF
#define NRF905_CMD_W_CONFIG		0x00
#define NRF905_CMD_R_CONFIG		0x10
#define NRF905_CMD_W_TX_PAYLOAD	0x20
#define NRF905_CMD_R_TX_PAYLOAD	0x21
#define NRF905_CMD_W_TX_ADDRESS	0x22
#define NRF905_CMD_R_TX_ADDRESS	0x23
#define NRF905_CMD_R_RX_PAYLOAD	0x24
#define NRF905_CMD_CHAN_CONFIG	0x80

// Registers
#define NRF905_REG_CHANNEL			0x00
#define NRF905_REG_CONFIG1			0x01
#define NRF905_REG_ADDR_WIDTH		0x02
#define NRF905_REG_RX_PAYLOAD_SIZE	0x03
#define NRF905_REG_TX_PAYLOAD_SIZE	0x04
#define NRF905_REG_RX_ADDRESS		0x05
#define NRF905_REG_CONFIG2			0x09


// TODO remove
#define NRF905_REG_AUTO_RETRAN	NRF905_REG_CONFIG1
#define NRF905_REG_LOW_RX		NRF905_REG_CONFIG1
#define NRF905_REG_PWR			NRF905_REG_CONFIG1
#define NRF905_REG_BAND			NRF905_REG_CONFIG1
#define NRF905_REG_CRC			NRF905_REG_CONFIG2
#define NRF905_REG_CLK			NRF905_REG_CONFIG2
#define NRF905_REG_OUTCLK		NRF905_REG_CONFIG2
#define NRF905_REG_OUTCLK_FREQ	NRF905_REG_CONFIG2


// Clock options
#define NRF905_CLK_4MHZ			0x00
#define NRF905_CLK_8MHZ			0x08
#define NRF905_CLK_12MHZ		0x10
#define NRF905_CLK_16MHZ		0x18
#define NRF905_CLK_20MHZ		0x20

// Register masks
#define NRF905_MASK_CHANNEL		0xFE
#define NRF905_MASK_AUTO_RETRAN	~(NRF905_AUTO_RETRAN_ENABLE | NRF905_AUTO_RETRAN_DISABLE) //0xDF
#define NRF905_MASK_LOW_RX		~(NRF905_LOW_RX_ENABLE | NRF905_LOW_RX_DISABLE) //0xEF
#define NRF905_MASK_PWR			~(NRF905_PWR_n10 | NRF905_PWR_n2 | NRF905_PWR_6 | NRF905_PWR_10) //0xF3
#define NRF905_MASK_BAND		~(NRF905_BAND_433 | NRF905_BAND_868 | NRF905_BAND_915) //0xFD
#define NRF905_MASK_CRC			(uint8_t)(~(NRF905_CRC_DISABLE | NRF905_CRC_8 | NRF905_CRC_16)) //0x3F // typecast to stop compiler moaning about large integer truncation
#define NRF905_MASK_CLK			~(NRF905_CLK_4MHZ | NRF905_CLK_8MHZ | NRF905_CLK_12MHZ | NRF905_CLK_16MHZ | NRF905_CLK_20MHZ) //0xC7
#define NRF905_MASK_OUTCLK		~(NRF905_OUTCLK_DISABLE | NRF905_OUTCLK_4MHZ | NRF905_OUTCLK_2MHZ | NRF905_OUTCLK_1MHZ | NRF905_OUTCLK_500KHZ) // 0xF8

// Bit positions
#define NRF905_STATUS_DR		5
#define NRF905_STATUS_AM		7

/**
* @brief Save a few mA by reducing receive sensitivity.
*/
typedef enum
{
	NRF905_LOW_RX_DISABLE = 0x00,	///< Disable low power receive
	NRF905_LOW_RX_ENABLE = 0x10		///< Enable low power receive
} nRF905_low_rx_t;

/**
* @brief Auto re-transmit options.
*/
typedef enum
{
	NRF905_AUTO_RETRAN_DISABLE = 0x00,	///< Disable auto re-transmit
	NRF905_AUTO_RETRAN_ENABLE = 0x20	///< Enable auto re-transmit
} nRF905_auto_retran_t;

/**
* @brief Address size.
*
* This is actually used as the SYNC word
*/
typedef enum
{
	NRF905_ADDR_SIZE_1 = 0x01,	///< 1 byte (not recommended, a lot of false invalid packets will be received)
	NRF905_ADDR_SIZE_4 = 0x04,	///< 4 bytes
} nRF905_addr_size_t;

#endif /* NRF905_DEFS_H_ */
