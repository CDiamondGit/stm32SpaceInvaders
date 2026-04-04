#ifndef SOUND_ENGINE_H
#define SOUND_ENGINE_H

#include <stdint.h>

void start_sound_effect_ch1(const uint32_t notes[], const uint32_t times[], uint32_t count, uint32_t repeat);
void start_sound_effect_ch2(const uint32_t notes[], const uint32_t times[], uint32_t count, uint32_t repeat);

void stop_sound_effect_ch1(void);
void stop_sound_effect_ch2(void);

void sound_engine_tick_isr(void);
int sound_effect_ch1_busy(void);

#endif