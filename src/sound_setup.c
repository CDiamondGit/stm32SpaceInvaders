#include "sound_setup.h"
#include "platform.h"
#include "musical_notes.h"

#include <stdint.h>
#include <stm32f031x6.h>

void initSound(void)
{
    RCC->APB1ENR |= (1 << 8); // Enable TIM14
	pinMode(GPIOB,1,2);
	GPIOB->AFR[0] &= ~(0x0fu << 4); // Route PB1 to TIM14 channel 1

	TIM14->CR1 = 0;
	TIM14->CCMR1 = (1 << 6) + (1 << 5); // PWM mode on channel 1
	TIM14->CCER |= (1 << 0);

	TIM14->PSC = 48000000UL/65536UL; // Prescale 48 MHz down to about 65.5 kHz
	TIM14->ARR = (48000000UL/(uint32_t)(TIM14->PSC))/((uint32_t)C4);
	TIM14->CCR1 = TIM14->ARR/2;	// 50% duty cycle for a square wave
	TIM14->CNT = 0;
}
void playNote(uint32_t freq)
{
    // Timer is running at about 65536 Hz after prescaling
	TIM14->ARR = (uint32_t)65536/((uint32_t)freq);
	TIM14->CCR1 = TIM14->ARR/2;	// Keep duty cycle at 50%
	TIM14->CNT = 0;
	TIM14->CR1 |= (1 << 0); // Start the timer
}
void stopSound(void)
{
    TIM14->CR1 &= ~TIM_CR1_CEN;  // Stop TIM14
}

void initSound2(void)
{
    RCC->APB1ENR |= (1 << 1); // Enable TIM3
    pinMode(GPIOB, 0, 2);
    GPIOB->AFR[0] &= ~(0x0fu << 0);
    GPIOB->AFR[0] |= (1 << 0); // Route PB0 to TIM3 channel 3

    TIM3->CR1 = 0;
    TIM3->CCMR2 = (1 << 6) + (1 << 5); // PWM mode on channel 3
    TIM3->CCER |= (1 << 8);

    TIM3->PSC = 48000000UL/65536UL; // Same prescaler as TIM14
    TIM3->ARR = (48000000UL/(uint32_t)(TIM3->PSC))/((uint32_t)C4);
    TIM3->CCR3 = TIM3->ARR/2; // 50% duty cycle
    TIM3->CNT = 0;
}
void playNote2(uint32_t freq)
{
    TIM3->ARR = (uint32_t)65536/((uint32_t)freq); // Set pitch from frequency
    TIM3->CCR3 = TIM3->ARR/2;
    TIM3->CNT = 0;
    TIM3->CR1 |= (1 << 0); // Start TIM3
}
void stopSound2(void)
{
    TIM3->CR1 &= ~TIM_CR1_CEN; // Stop TIM3
}