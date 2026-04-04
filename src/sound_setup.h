#ifndef SOUND_SETUP_H
#define SOUND_SETUP_H

#include <stdint.h>

void initSound(void);
void playNote(uint32_t freq);
void stopSound(void);

void initSound2(void);
void playNote2(uint32_t freq);
void stopSound2(void);

#endif