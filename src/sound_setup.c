#include "sound_setup.h"
#include "platform.h"
#include "musical_notes.h"

#include <stdint.h>
#include <stm32f031x6.h>

void initSound(void)
{
    // Power up the timer module
	RCC->APB1ENR |= (1 << 8);
	pinMode(GPIOB,1,2); // Assign a non-GPIO (alternate) function to GPIOB bit 1
	GPIOB->AFR[0] &= ~(0x0fu << 4); // Assign alternate function 0 to GPIOB 1 (Timer 14 channel 1)
	TIM14->CR1 = 0; // Set Timer 14 to default values
	TIM14->CCMR1 = (1 << 6) + (1 << 5);
	TIM14->CCER |= (1 << 0);
	TIM14->PSC = 48000000UL/65536UL; // Use the prescaled to set the counter running at 65536 Hz
									 // yields maximum frequency of 21kHz when ARR = 2;
	TIM14->ARR = (48000000UL/(uint32_t)(TIM14->PSC))/((uint32_t)C4);
	TIM14->CCR1 = TIM14->ARR/2;	
	TIM14->CNT = 0;
}
void playNote(uint32_t freq)
{
    // Counter is running at 65536 Hz 
	TIM14->ARR = (uint32_t)65536/((uint32_t)freq); 
	TIM14->CCR1 = TIM14->ARR/2;	
	TIM14->CNT = 0; // set the count to zero initially
	TIM14->CR1 |= (1 << 0); // and enable the counter
}
void stopSound(void)
{
    TIM14->CR1 &= ~TIM_CR1_CEN;  // disable timer
}
void initSound2(void)
{
    RCC->APB1ENR |= (1 << 1); // TIM3 clock
    pinMode(GPIOB, 0, 2);     // PB0 alternate function
    GPIOB->AFR[0] &= ~(0x0fu << 0); // AF1 on PB0
    GPIOB->AFR[0] |= (1 << 0);
    TIM3->CR1 = 0;
    TIM3->CCMR2 = (1 << 6) + (1 << 5); // CH3 PWM mode
    TIM3->CCER |= (1 << 8);             // CH3 enable
    TIM3->PSC = 48000000UL/65536UL;
    TIM3->ARR = (48000000UL/(uint32_t)(TIM3->PSC))/((uint32_t)C4);
    TIM3->CCR3 = TIM3->ARR/2;
    TIM3->CNT = 0;
}
void playNote2(uint32_t freq)
{
    TIM3->ARR = (uint32_t)65536/((uint32_t)freq);
    TIM3->CCR3 = TIM3->ARR/2;
    TIM3->CNT = 0;
    TIM3->CR1 |= (1 << 0);
}
void stopSound2(void)
{
    TIM3->CR1 &= ~TIM_CR1_CEN;
}