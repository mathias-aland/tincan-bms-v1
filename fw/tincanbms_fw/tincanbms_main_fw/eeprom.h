/*
 * eeprom.h
 *
 * Created: 06-04-2024 19:58:57
 *  Author: Mathias
 */ 


#ifndef EEPROM_H_
#define EEPROM_H_


void EE_readParams();
void EE_writeParamsLoop();
void EE_requestWrite();
bool EE_writePending();

#endif /* EEPROM_H_ */