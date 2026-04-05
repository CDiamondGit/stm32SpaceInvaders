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
    return; // ignore empty sound effects
  }

  __disable_irq(); // stop ISR from reading half-updated playback state
  channel1_notes = notes;
  channel1_times = times;
  channel1_note_count = count;
  channel1_repeat = repeat;
  channel1_note_index = 0;
  channel1_note_timer = channel1_times[0]; // load duration of the first note

  if (channel1_notes[0] == REST) {
    stopSound(); // silent gap/rest instead of a tone
  } else {
    playNote(channel1_notes[0]); // start first note immediately
  }
  __enable_irq();
}
void start_sound_effect_ch2(const uint32_t notes[], const uint32_t times[], uint32_t count, uint32_t repeat) {
  if (count == 0) {
    return; // ignore empty sound effects
  }

  __disable_irq(); // protect channel 2 state while updating it
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
  __disable_irq(); // clear playback state safely
  channel1_notes = 0;
  channel1_times = 0;
  channel1_note_count = 0;
  channel1_repeat = 0;
  channel1_note_index = 0;
  channel1_note_timer = 0;
  stopSound(); // physically stop the PWM output too
  __enable_irq();
}
void stop_sound_effect_ch2(void) {
  __disable_irq(); // clear playback state safely
  channel2_notes = 0;
  channel2_times = 0;
  channel2_note_count = 0;
  channel2_repeat = 0;
  channel2_note_index = 0;
  channel2_note_timer = 0;
  stopSound2(); // stop output
  __enable_irq();
}

void sound_engine_tick_isr(void) {

  /* channel 1 */
  if (channel1_notes != 0) {
    if (channel1_note_timer > 0)
      channel1_note_timer--; // One tick has passed, so count down the current note time

    if (channel1_note_timer == 0) {
      channel1_note_index++; // Move on to the next note when the current one has finished

      if (channel1_note_index >= channel1_note_count) {
        if (channel1_repeat == 0) {
          channel1_notes = 0; // No more notes left, so mark the channel as idle
          channel1_times = 0;
          channel1_note_count = 0;
          channel1_note_index = 0;
          channel1_note_timer = 0;
          stopSound(); // Stop the hardware output because the sound effect is finished
        } else {
          channel1_note_index = 0; // Loop back to the start if this sound should repeat
        }
      }

      if (channel1_notes != 0) {
        channel1_note_timer = channel1_times[channel1_note_index]; // Load the duration of the new current note
        if (channel1_notes[channel1_note_index] == REST) {
          stopSound(); // A REST means keep silent for this part of the sequence
        } else {
          playNote(channel1_notes[channel1_note_index]); // Play the next note in the sequence
        }
      }
    }
  }

  /* channel 2 */
  if (channel2_notes != 0) {
    if (channel2_note_timer > 0)
      channel2_note_timer--; // Count down how much longer the current channel 2 note should last

    if (channel2_note_timer == 0) {
      channel2_note_index++; // Step to the next note in the channel 2 sequence

      if (channel2_note_index >= channel2_note_count) {
        if (channel2_repeat == 0) {
          channel2_notes = 0; // Sequence is finished, so clear channel 2 playback state
          channel2_times = 0;
          channel2_note_count = 0;
          channel2_repeat = 0;
          channel2_note_index = 0;
          channel2_note_timer = 0;
          stopSound2(); // Stop channel 2 hardware output
        } else {
          channel2_note_index = 0; // Restart from the beginning if looping is enabled
        }
      }

      if (channel2_notes != 0) {
        channel2_note_timer = channel2_times[channel2_note_index]; // Load the duration of the next note
        if (channel2_notes[channel2_note_index] == REST) {
          stopSound2(); // REST means silence on this step
        } else {
          playNote2(channel2_notes[channel2_note_index]); // Play the next channel 2 note
        }
      }
    }
  }
}
int sound_effect_ch1_busy(void) {
  return channel1_notes != 0; // true while a channel 1 effect is active
}