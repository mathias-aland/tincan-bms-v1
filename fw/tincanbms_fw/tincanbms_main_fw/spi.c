/*
 * spi.c
 *
 * Created: 25-04-2024 00:21:02
 *  Author: Mathias
 */ 

#include "spi.h"

#define SPI_CLIENT_SELECT()    PORTA.OUTCLR = PIN7_bm  /* Set SS pin value to LOW */
#define SPI_CLIENT_DESELECT()  PORTA.OUTSET = PIN7_bm  /* Set SS pin value to HIGH */


uint8_t SPI0_exchangeData(uint8_t sent_data)
{
	SPI0.DATA = sent_data;
	while (!(SPI0.INTFLAGS & SPI_IF_bm));  /* waits until data is exchanged */

	return SPI0.DATA;
}


void mcp2515_reset()
{
	SPI_CLIENT_SELECT();
	SPI0_exchangeData(0xC0);		// RESET
	SPI_CLIENT_DESELECT();
}

uint8_t mcp2515_readbyte(uint8_t addr)
{
	uint8_t reg;
    SPI_CLIENT_SELECT();
    SPI0_exchangeData(0x03);		// READ
    SPI0_exchangeData(addr);		// address byte
    reg = SPI0_exchangeData(0x00);	// read register
    SPI_CLIENT_DESELECT();
	return reg;
}

void mcp2515_writebyte(uint8_t addr, uint8_t data)
{
	SPI_CLIENT_SELECT();
	SPI0_exchangeData(0x02);    // WRITE
	SPI0_exchangeData(addr);    // address byte
	SPI0_exchangeData(data);    // write register
	SPI_CLIENT_DESELECT();
}

void mcp2515_bitmodify(uint8_t addr, uint8_t mask, uint8_t data)
{
	SPI_CLIENT_SELECT();
	SPI0_exchangeData(0x05);    // BIT MODIFY
	SPI0_exchangeData(addr);    // address byte
	SPI0_exchangeData(mask);    // mask byte
	SPI0_exchangeData(data);    // data byte
	SPI_CLIENT_DESELECT();
}

void mcp2515_enable_boot(bool enableBoot)
{
	if (enableBoot)
		mcp2515_bitmodify(0x0C, 0x08, 0x08);	//(BFPCTRL.B1BFE = 1)
	else
		mcp2515_bitmodify(0x0C, 0x08, 0x00);	//(BFPCTRL.B1BFE = 0)
}
