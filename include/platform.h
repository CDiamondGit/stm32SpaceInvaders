#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include <stm32f031x6.h>

extern volatile uint32_t milliseconds;

void pinMode(GPIO_TypeDef* port, uint32_t pin, uint32_t mode);
void initClock(void);
void initSysTick(void);
void setupIO(void);
void delay(volatile uint32_t ms);
void lightLivesIndicator(int lives);

#endif