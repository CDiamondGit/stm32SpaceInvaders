#ifndef SOUND_SETUP_H
#define SOUND_SETUP_H

#include <stdint.h>

void initSound(void);   // Set up timer/PWM output for sound channel 1
void playNote(uint32_t freq);   // Play one note on channel 1 by setting PWM frequency
void stopSound(void);   // Stop channel 1 output

void initSound2(void);  // Set up timer/PWM output for sound channel 2
void playNote2(uint32_t freq);  // Play one note on channel 2
void stopSound2(void);  // Stop channel 2 output
#endif