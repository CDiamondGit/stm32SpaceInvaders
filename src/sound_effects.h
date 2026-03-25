#ifndef SOUND_EFFECTS_H
#define SOUND_EFFECTS_H

#include <stdint.h>
void playNote(uint32_t Freq);
void initSound(void);
void stopSound(void);
void initSound2(void);
void playNote2(uint32_t freq);
void stopSound2(void);
#endif
