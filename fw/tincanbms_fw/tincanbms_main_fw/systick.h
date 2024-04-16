/*
 * systick.h
 *
 * Created: 06-11-2023 23:12:54
 *  Author: Mathias
 */ 


#ifndef SYSTICK_H_
#define SYSTICK_H_

#include <stdint.h>
#include <stdbool.h>

void SysTick_Delay(uint32_t delayTicks);
uint32_t SysTick_GetTicks(void);
bool SysTick_CheckElapsed(uint32_t initialTicks, uint32_t delayTicks);

#endif /* SYSTICK_H_ */