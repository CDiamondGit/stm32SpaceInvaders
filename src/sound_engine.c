#include "sound_engine.h"
#include "sound_setup.h"
#include "musical_notes.h"

#include <stdint.h>
#include <stm32f031x6.h>

static volatile const uint32_t* channel1_notes = 0;
static volatile const uint32_t* channel1_times = 0;
static volatile uint32_t channel1_note_count = 0;
static volatile uint32_t channel1_repeat = 0;
static volatile uint32_t channel1_note_index = 0;
static volatile int32_t channel1_note_timer = 0;

static volatile const uint32_t* channel2_notes = 0;
static volatile const uint32_t* channel2_times = 0;
static volatile uint32_t channel2_note_count = 0;
static volatile uint32_t channel2_repeat = 0;
static volatile uint32_t channel2_note_index = 0;
static volatile int32_t channel2_note_timer = 0;

void start_sound_effect_ch1(const uint32_t notes[], const uint32_t times[], uint32_t count, uint32_t repeat) {
  if (count == 0) {
    return;
  }

  __disable_irq();
  channel1_notes = notes;
  channel1_times = times;
  channel1_note_count = count;
  channel1_repeat = repeat;
  channel1_note_index = 0;
  channel1_note_timer = channel1_times[0];

  if (channel1_notes[0] == REST) {
    stopSound();
  } else {
    playNote(channel1_notes[0]);
  }
  __enable_irq();
}
void start_sound_effect_ch2(const uint32_t notes[], const uint32_t times[], uint32_t count, uint32_t repeat) {
  if (count == 0) {
    return;
  }

  __disable_irq();
  channel2_notes = notes;
  channel2_times = times;
  channel2_note_count = count;
  channel2_repeat = repeat;
  channel2_note_index = 0;
  channel2_note_timer = channel2_times[0];

  if (channel2_notes[0] == REST) {
    stopSound2();
  } else {
    playNote2(channel2_notes[0]);
  }
  __enable_irq();
}

void stop_sound_effect_ch1(void) {
  __disable_irq();
  channel1_notes = 0;
  channel1_times = 0;
  channel1_note_count = 0;
  channel1_repeat = 0;
  channel1_note_index = 0;
  channel1_note_timer = 0;
  stopSound();
  __enable_irq();
}
void stop_sound_effect_ch2(void) {
  __disable_irq();
  channel2_notes = 0;
  channel2_times = 0;
  channel2_note_count = 0;
  channel2_repeat = 0;
  channel2_note_index = 0;
  channel2_note_timer = 0;
  stopSound2();
  __enable_irq();
}

void sound_engine_tick_isr(void) {

  /* channel 1 */
  if (channel1_notes != 0) {
    if (channel1_note_timer > 0)
      channel1_note_timer--;

    if (channel1_note_timer == 0) {
      channel1_note_index++;

      if (channel1_note_index >= channel1_note_count) {
        if (channel1_repeat == 0) {
          channel1_notes = 0;
          channel1_times = 0;
          channel1_note_count = 0;
          channel1_note_index = 0;
          channel1_note_timer = 0;
          stopSound();
        } else {
          channel1_note_index = 0;
        }
      }

      if (channel1_notes != 0) {
        channel1_note_timer = channel1_times[channel1_note_index];
        if (channel1_notes[channel1_note_index] == REST) {
          stopSound();
        } else {
          playNote(channel1_notes[channel1_note_index]);
        }
      }
    }
  }

  /* channel 2 */
  if (channel2_notes != 0) {
    if (channel2_note_timer > 0)
      channel2_note_timer--;

    if (channel2_note_timer == 0) {
      channel2_note_index++;

      if (channel2_note_index >= channel2_note_count) {
        if (channel2_repeat == 0) {
          channel2_notes = 0;
          channel2_times = 0;
          channel2_note_count = 0;
          channel2_repeat = 0;
          channel2_note_index = 0;
          channel2_note_timer = 0;
          stopSound2();
        } else {
          channel2_note_index = 0;
        }
      }

      if (channel2_notes != 0) {
        channel2_note_timer = channel2_times[channel2_note_index];
        if (channel2_notes[channel2_note_index] == REST) {
          stopSound2();
        } else {
          playNote2(channel2_notes[channel2_note_index]);
        }
      }
    }
  }
}
int sound_effect_ch1_busy(void) {
  return channel1_notes != 0;
}