#include "sound_data.h"
#include "musical_notes.h"

#include <stdint.h>

const uint32_t shoot_notes[3] = {D5, D4, D5};
const uint32_t shoot_times[3] = {80, 100, 80};
const uint32_t shoot_note_count = 3;

const uint32_t explode_notes[11] = {F4, D4, C4, A3, F3, D3, C3, A2, F2, D2, C2};
const uint32_t explode_times[11] = {20, 20, 25, 25, 30, 35, 35, 40, 40, 45, 55};
const uint32_t explode_note_count = 11;

const uint32_t enter_game_notes_ch1[3] = {C3, D3, G3};
const uint32_t enter_game_times_ch1[3] = {150, 150, 150};
const uint32_t enter_game_note_count_ch1 = 3;

const uint32_t enter_game_notes_ch2[3] = {C6, D6, G6};
const uint32_t enter_game_times_ch2[3] = {150, 150, 150};
const uint32_t enter_game_note_count_ch2 = 3;

const uint32_t game_loop_notes[768] = {
    REST,    G5,      A5,      AS5_Bb5, A5,      F5,      A5,      G5,
    REST,    G5,      A5,      AS5_Bb5, C6,      AS5_Bb5, A5,      G5,
    REST,    G5,      A5,      AS5_Bb5, A5,      F5,      A5,      G5,
    D6,      REST,    C6,      REST,    AS5_Bb5, A5,      AS5_Bb5, C6,
    F6,      REST,    REST,    G5,      D5,      D6,      D5,      C6,
    D5,      AS5_Bb5, D5,      A5,      D5,      AS5_Bb5, D5,      A5,
    D5,      G5,      D5,      A5,      D5,      AS5_Bb5, D5,      C6,
    D5,      AS5_Bb5, D5,      A5,      D5,      F5,      D5,      A5,
    D5,      G5,      D5,      G5,      D5,      D6,      D5,      C6,
    D5,      AS5_Bb5, D5,      A5,      D5,      AS5_Bb5, D5,      A5,
    D5,      AS5_Bb5, D5,      A5,      D5,      AS5_Bb5, D5,      C6,
    D5,      AS5_Bb5, D5,      A5,      D5,      F5,      D5,      A5,
    D5,      G5,      D5,      G5,      D5,      D6,      D5,      C6,
    D5,      AS5_Bb5, D5,      A5,      D5,      AS5_Bb5, D5,      A5,
    D5,      G5,      D5,      A5,      D5,      AS5_Bb5, D5,      C6,
    D5,      AS5_Bb5, D5,      A5,      D5,      F5,      D5,      A5,
    D5,      G5,      D5,      AS5_Bb5, D5,      D6,      D5,      C6,
    D5,      AS5_Bb5, D5,      A5,      D5,      AS5_Bb5, D5,      A5,
    D5,      G5,      D5,      A5,      D5,      AS5_Bb5, D5,      C6,
    D5,      AS5_Bb5, D5,      A5,      D5,      F5,      D5,      A5,
    D5,      G5,      D5,      C6,      C6,      F6,      D6,      REST,
    REST,    REST,    C6,      AS5_Bb5, C6,      F6,      D6,      C6,
    AS5_Bb5, C6,      F6,      D6,      REST,    REST,    REST,    C6,
    D6,      DS6_Eb6, F6,      D6,      REST,    DS6_Eb6, REST,    C6,
    F6,      D6,      REST,    REST,    REST,    C6,      AS5_Bb5, C6,
    F6,      D6,      C6,      AS5_Bb5, C6,      F6,      D6,      REST,
    REST,    REST,    C6,      D6,      DS6_Eb6, F6,      D5,      FS5_Gb5,
    F5,      A5,      A5,      G5,      A5,      G5,      A5,      G5,
    AS5_Bb5, A5,      G5,      F5,      A5,      G5,      D5,      A5,
    G5,      D5,      A5,      G5,      D5,      AS5_Bb5, C6,      A5,
    AS5_Bb5, G5,      D5,      D6,      D5,      C6,      D5,      AS5_Bb5,
    D5,      A5,      D5,      AS5_Bb5, D5,      A5,      D5,      G5,
    D5,      A5,      D5,      AS5_Bb5, D5,      C6,      D5,      AS5_Bb5,
    D5,      A5,      D5,      F5,      D5,      A5,      D5,      G5,
    D5,      G5,      D5,      D6,      D5,      C6,      D5,      AS5_Bb5,
    D5,      A5,      D5,      AS5_Bb5, D5,      A5,      D5,      AS5_Bb5,
    D5,      A5,      D5,      AS5_Bb5, D5,      C6,      D5,      AS5_Bb5,
    D5,      A5,      D5,      F5,      D5,      A5,      D5,      G5,
    D5,      G5,      D5,      D6,      D5,      C6,      D5,      AS5_Bb5,
    D5,      A5,      D5,      AS5_Bb5, D5,      A5,      D5,      G5,
    D5,      A5,      D5,      AS5_Bb5, D5,      C6,      D5,      AS5_Bb5,
    D5,      A5,      D5,      F5,      D5,      A5,      D5,      G5,
    D5,      AS5_Bb5, D5,      D6,      D5,      C6,      D5,      AS5_Bb5,
    D5,      A5,      D5,      AS5_Bb5, D5,      A5,      D5,      G5,
    D5,      A5,      D5,      AS5_Bb5, D5,      C6,      D5,      AS5_Bb5,
    D5,      A5,      D5,      F5,      D5,      A5,      D5,      G5,
    D5,      C6,      C6,      F6,      D6,      REST,    REST,    REST,
    C5,      REST,    A4,      AS4_Bb4, C5,      D6,      G4,      AS4_Bb4,
    G4,      C5,      G4,      D6,      G4,      C6,      F4,      A4,
    F4,      F5,      F4,      D6,      DS4_Eb4, D6,      REST,    E4,
    F4,      GS4_Ab4, REST,    AS4_Bb4, REST,    DS5_Eb5, GS4_Ab4, B4,
    GS4_Ab4, CS5_Db5, GS4_Ab4, DS5_Eb5, GS4_Ab4, CS5_Db5, FS4_Gb4, AS4_Bb4,
    FS4_Gb4, FS5_Gb5, FS4_Gb4, DS5_Eb5, E5,      D5,      REST,    CS5_Db5,
    REST,    AS4_Bb4, B4,      CS5_Db5, DS5_Eb5, GS4_Ab4, B4,      GS4_Ab4,
    CS5_Db5, GS4_Ab4, DS5_Eb5, GS4_Ab4, CS5_Db5, FS4_Gb4, AS4_Bb4, FS4_Gb4,
    FS5_Gb5, FS4_Gb4, DS5_Eb5, E5,      DS5_Eb5, REST,    DS5_Eb5, E5,
    FS5_Gb5, CS5_Db5, E5,      CS4_Db4, DS5_Eb5, E5,      G5,      AS5_Bb5,
    GS5_Ab5, DS5_Eb5, DS6_Eb6, DS5_Eb5, CS6_Db6, DS5_Eb5, B5,      DS5_Eb5,
    AS5_Bb5, DS5_Eb5, B5,      DS5_Eb5, AS5_Bb5, DS5_Eb5, GS5_Ab5, DS5_Eb5,
    AS5_Bb5, DS5_Eb5, B5,      DS5_Eb5, CS6_Db6, DS5_Eb5, B5,      DS5_Eb5,
    AS5_Bb5, DS5_Eb5, FS5_Gb5, DS5_Eb5, AS5_Bb5, DS5_Eb5, GS5_Ab5, DS5_Eb5,
    GS5_Ab5, DS5_Eb5, DS6_Eb6, DS5_Eb5, CS6_Db6, DS5_Eb5, B5,      DS5_Eb5,
    AS5_Bb5, DS5_Eb5, B5,      DS5_Eb5, AS5_Bb5, DS5_Eb5, GS5_Ab5, DS5_Eb5,
    AS5_Bb5, DS5_Eb5, B5,      DS5_Eb5, CS6_Db6, DS5_Eb5, B5,      DS5_Eb5,
    AS5_Bb5, DS5_Eb5, FS5_Gb5, DS5_Eb5, AS5_Bb5, DS5_Eb5, GS5_Ab5, DS5_Eb5,
    GS5_Ab5, DS5_Eb5, DS6_Eb6, DS5_Eb5, CS6_Db6, DS5_Eb5, B5,      DS5_Eb5,
    AS5_Bb5, DS5_Eb5, B5,      DS5_Eb5, AS5_Bb5, DS5_Eb5, GS5_Ab5, DS5_Eb5,
    AS5_Bb5, DS5_Eb5, B5,      DS5_Eb5, CS6_Db6, DS5_Eb5, B5,      DS5_Eb5,
    AS5_Bb5, DS5_Eb5, FS5_Gb5, DS5_Eb5, AS5_Bb5, DS5_Eb5, GS5_Ab5, DS5_Eb5,
    GS5_Ab5, DS5_Eb5, DS6_Eb6, DS5_Eb5, CS6_Db6, DS5_Eb5, B5,      DS5_Eb5,
    AS5_Bb5, DS5_Eb5, B5,      DS5_Eb5, AS5_Bb5, DS5_Eb5, GS5_Ab5, DS5_Eb5,
    AS5_Bb5, DS5_Eb5, B5,      DS5_Eb5, CS6_Db6, DS5_Eb5, B5,      DS5_Eb5,
    AS5_Bb5, DS5_Eb5, FS5_Gb5, DS5_Eb5, AS5_Bb5, DS5_Eb5, GS5_Ab5, DS5_Eb5,
   CS6_Db6, FS6_Gb6, DS6_Eb6, REST,    REST,    REST,    CS6_Db6, B5,
    CS6_Db6, FS6_Gb6, DS6_Eb6, CS6_Db6, B5,      CS6_Db6, FS6_Gb6, DS6_Eb6,
    REST,    REST,    REST,    CS6_Db6, B5,      E6,      F6,      DS6_Eb6,
    REST,    E6,      REST,    REST,    CS6_Db6, FS6_Gb6, DS6_Eb6, REST,
    REST,    REST,    CS6_Db6, B5,      CS6_Db6, FS6_Gb6, DS6_Eb6, CS6_Db6,
    B5,      CS6_Db6, FS6_Gb6, DS6_Eb6, REST,    REST,    REST,    CS5_Db5,
    DS5_Eb5, E5,      F5,      DS5_Eb5, G5,      GS5_Ab5, AS5_Bb5, AS5_Bb5,
    GS5_Ab5, AS5_Bb5, GS5_Ab5, AS5_Bb5, GS5_Ab5, B6,      AS5_Bb5, GS5_Ab5,
    FS5_Gb5, AS5_Bb5, GS6_Ab6, DS5_Eb5, AS5_Bb5, GS6_Ab6, DS5_Eb5, AS5_Bb5,
    GS6_Ab6, DS5_Eb5, B5,      CS6_Db6, AS5_Bb5, B5,      GS5_Ab5, REST,
    REST};
const uint32_t game_loop_times[768] = {
    417, 417, 417, 417, 417, 417, 417, 417, 417, 417, 417, 417, 417, 417, 417,
    417, 417, 417, 417, 417, 417, 417, 417, 417, 417, 208, 208, 417, 417, 417,
    208, 208, 208, 208, 417, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
    104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
    104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
    104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
    104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
    104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
    104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
    104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
    104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
    208, 104, 208, 417, 104, 104, 312, 312, 625, 208, 208, 208, 104, 208, 104,
    208, 417, 208, 208, 312, 312, 312, 104, 208, 208, 208, 104, 208, 104, 208,
    417, 208, 208, 312, 312, 625, 208, 208, 208, 104, 208, 104, 208, 417, 208,
    208, 312, 312, 208, 208, 208, 208, 312, 625, 312, 625, 312, 625, 208, 208,
    208, 208, 312, 312, 208, 312, 312, 208, 312, 312, 208, 417, 417, 417, 417,
    104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
    104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
    104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
    104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
    104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
    104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
    104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
    104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
    104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
    104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
    104, 104, 208, 52,  52,  52,  52,  104, 104, 104, 104, 104, 104, 104, 104,
    104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
    417, 417, 208, 208, 156, 156, 104, 104, 104, 104, 104, 104, 104, 104, 104,
    104, 104, 104, 104, 104, 104, 104, 104, 104, 208, 104, 104, 208, 208, 208,
    208, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
    104, 104, 104, 417, 417, 208, 208, 156, 156, 104, 104, 104, 104, 104, 104,
    104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 417, 208, 104,
    104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
    104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
    104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
    104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
    104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
    104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
    104, 104, 104, 208, 104, 208, 104, 208, 417, 208, 208, 312, 312, 625, 208,
    208, 208, 104, 208, 104, 208, 417, 208, 208, 312, 312, 312, 104, 104, 208,
    104, 208, 104, 208, 417, 208, 208, 312, 312, 625, 208, 208, 208, 104, 208,
    104, 208, 417, 208, 208, 312, 312, 208, 208, 208, 208, 312, 312, 312, 312,
    208, 208, 104};
const uint32_t game_loop_note_count = 768;

const uint32_t game_win_notes_ch1[4] = {C3, E3, G3, C4};
const uint32_t game_win_times_ch1[4] = {200, 200, 200, 200};
const uint32_t game_win_note_count_ch1 = 4;

const uint32_t game_win_notes_ch2[4] = {C5, E5, G5, C6};
const uint32_t game_win_times_ch2[4] = {200, 200, 200, 200};
const uint32_t game_win_note_count_ch2 = 4;

const uint32_t game_lose_notes_ch1[4] = {C4, B3, AS3_Bb3, A3};
const uint32_t game_lose_times_ch1[4] = {200, 200, 200, 200};
const uint32_t game_lose_note_count_ch1 = 4;

const uint32_t game_lose_notes_ch2[4] = {C6, B5, AS5_Bb5, A5};
const uint32_t game_lose_times_ch2[4] = {200, 200, 200, 200};
const uint32_t game_lose_note_count_ch2 = 4;

const uint32_t lose_life_notes[2] = {FS2_Gb2, FS2_Gb2};
const uint32_t lose_life_times[2] = {160, 160};
const uint32_t lose_life_note_count = 2;

const uint32_t aliens_spawning_notes[3] = {G6, B6, G6};
const uint32_t aliens_spawning_times[3] = {220, 220, 220};
const uint32_t aliens_spawning_note_count = 3;