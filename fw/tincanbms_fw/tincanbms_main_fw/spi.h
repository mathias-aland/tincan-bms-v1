/*
 * spi.h
 *
 * Created: 25-04-2024 00:21:10
 *  Author: Mathias
 */ 


#ifndef SPI_H_
#define SPI_H_

#include <avr/io.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

void mcp2515_reset();
uint8_t mcp2515_readbyte(uint8_t addr);
void mcp2515_writebyte(uint8_t addr, uint8_t data);
void mcp2515_bitmodify(uint8_t addr, uint8_t mask, uint8_t data);
void mcp2515_enable_boot(bool enableBoot);



#endif /* SPI_H_ */