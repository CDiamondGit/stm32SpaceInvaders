#ifndef SOUND_ENGINE_H
#define SOUND_ENGINE_H

#include <stdint.h>

void start_sound_effect_ch1(const uint32_t notes[], const uint32_t times[], uint32_t count, uint32_t repeat);   // Start a sound effect on channel 1 using note and timing arrays
void start_sound_effect_ch2(const uint32_t notes[], const uint32_t times[], uint32_t count, uint32_t repeat);   // Start a sound effect on channel 2 using note and timing arrays

void stop_sound_effect_ch1(void);   // Stop whatever sound is currently playing on channel 1
void stop_sound_effect_ch2(void);   // Stop whatever sound is currently playing on channel 2

void sound_engine_tick_isr(void);   // Advance the sound engine each SysTick and switch notes when needed
int sound_effect_ch1_busy(void);   // Check if channel 1 is already busy playing a sound

#endif