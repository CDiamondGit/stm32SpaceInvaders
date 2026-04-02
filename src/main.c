/*
 * 1. INCLUDES & DEFINES
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stm32f031x6.h>
#include <time.h>
#include "display.h"
#include "musical_notes.h"
#include "serial.h"
#include "sound_effects.h"

/*----GAME
 * FUNCTIONALITY------------------------------------------______---------------------*/
#define SCORE_INC_EASY 10
#define SCORE_INC_MED 15
#define SCORE_INC_HARD 20
#define MAX_RECORDS 5

/* --- Screen --------------------------------------------------------------- */
#define SCREEN_W 128
#define SCREEN_H 160

/* --- Ship ----------------------------------------------------------------- */
#define SHIP_W 12
#define SHIP_H 12
#define SHIP_MIN_X 2
#define SHIP_MAX_X (SCREEN_W - SHIP_W - 2)
#define SHIP_SPEED 2

/* --- Bullet --------------------------------------------------------------- */
#define BULLET_W 2
#define BULLET_H 2
#define BULLET_OFFSET_X 6
#define BULLET_OFFSET_Y 13
#define BULLET_SPEED 4
#define ALIEN_BULLET_SPEED 1
#define BULLET_COLOR 65287
#define ALIEN_BULLET_COLOR 57034

/* --- Alien grid ----------------------------------------------------------- */
#define ALIEN_W 11
#define ALIEN_H 8
#define MAIN_ALIEN_W 22
#define MAIN_ALIEN_H 16
#define ALIEN_ROWS 3
#define ALIEN_COLS 5
#define ALIEN_GAP_X 11    /* horizontal gap between aliens          */
#define ALIEN_GAP_Y 5     /* vertical gap between aliens            */
#define ALIEN_ORIGIN_X 12 /* grid left edge at game start           */
#define ALIEN_ORIGIN_Y 20 /* grid top  edge at game start           */

#define ALIEN_GRID_W \
  (ALIEN_COLS * (ALIEN_W + ALIEN_GAP_X) - ALIEN_GAP_X) /* 75 */
#define ALIEN_GRID_H \
  (ALIEN_ROWS * (ALIEN_H + ALIEN_GAP_Y) - ALIEN_GAP_Y) /* 34 */

/* Movement bounds for the grid origin (left edge of col-0) */
#define ALIEN_MIN_X 2
#define ALIEN_MAX_X (SCREEN_W - ALIEN_GRID_W - 2)

/* How many pixels the grid shifts per move tick, and ms between ticks */
#define ALIEN_STEP 5
#define ALIEN_MOVE_MS_EASY 1000
#define ALIEN_MOVE_MS_MED 500
#define ALIEN_MOVE_MS_HARD 200
/* decrease speeds up alien and increase to slow down          */

/* Drop distance when hitting a wall (one alien row + gap) */
#define ALIEN_DROP (ALIEN_H + ALIEN_GAP_Y)

/*---- Main alien--------------------------------------------------------*/

#define MAIN_ALIEN_Y 10
#define MAIN_ALIEN_MIN_DELAY 3000
#define MAIN_ALIEN_MAX_DELAY 6000
#define MAIN_ALIEN_MOVE_DELAY 30  // movement speed (lower = faster)

/* --- HUD ------------------------------------------------------------------ */
#define HUD_LINE_Y 144
#define HUD_LINE_COLOR 57351

// #define ALIEN_FIRE_MIN_MS 800
#define ALIEN_FIRE_MIN_MS 800
#define ALIEN_FIRE_MAX_MS 3000

/* --- Loading Bar -----------------------------------------------------------
 */
#define LBAR_X 20
#define LBAR_Y 100
#define LBAR_W 200
#define LBAR_H 20

#define LBAR_BACKGROUND 0x0000  // black (bakcground)
#define LBAR_FILL 0x07E0        // green

#define STAR_RED 63680
#define STAR_BLUE 1119
#define STAR_WHITE 65535

/*
 * TYPE DECLARATIONS
 */

/* --- Bullet / game object types ------------------------------------------ */
typedef enum { BULLET_READY, BULLET_FIRE } BulletState;

typedef struct {
  uint16_t x, y;
  uint16_t oldX, oldY;
} Transform;

typedef struct {
  Transform coords;
  uint16_t speed;
} Ship;

typedef struct {
  Transform coords;
  BulletState state;
  uint16_t speed;
} Bullet;

typedef struct {
  char initials[4]; /* 3 letters + null terminator */
  uint32_t score;
} ScoreRecord;

/* --- Alien formation ------------------------------------------------------ */

typedef struct {
  Transform coords;
  uint16_t speed;
  int8_t dir;              // 1 = right, -1 = left
  uint8_t active;          // 0 = hidden, 1 = active
  uint32_t nextSpawnTime;  // when to appear
  uint32_t lastMoveTime;   // movement timing
} MainAlien;

typedef struct {
  int16_t offsetX, offsetY;
  int16_t oldOffsetX, oldOffsetY;
  int8_t dirX;
  uint32_t lastMoveTime;
  uint8_t status[ALIEN_ROWS][ALIEN_COLS];
  uint16_t basePosX[ALIEN_COLS];
  uint16_t basePosY[ALIEN_ROWS];
  int8_t earToggle;
  Bullet ab[ALIEN_COLS];
  uint32_t nextFireTime[ALIEN_COLS];
} AlienGrid;

/* --- Whole game state ----------------------------------------------------- */
typedef struct {
  Ship ship;
  Bullet bullet;
  AlienGrid aliens;
  int lives;
  int score;
  int highScore;
  int alienSpeed;
  int score_inc;
  MainAlien mysteryAlien;
} GameState;

/* --- App / gameplay states ------------------------------------------------ */
typedef enum { MAINMENU, PLAYING, HELP, RECORD, MODE } AppState;

typedef enum { GAMERUNNING, PAUSE, GAMEOVER } PlayingState;
typedef enum { EASY, MEDIUM, HARD } GameMode;

/*
 * FUNCTION PROTOTYPES
 */

/* --- Sound / timing / interrupts ----------------------------------------- */
void start_sound_effect_ch1(const uint32_t notes[],
                            const uint32_t times[],
                            uint32_t count,
                            uint32_t repeat);
void start_sound_effect_ch2(const uint32_t notes[],
                            const uint32_t times[],
                            uint32_t count,
                            uint32_t repeat);
void SysTick_Handler(void);
static void stop_sound_effect_ch1(void);
static void stop_sound_effect_ch2(void);

/* --- Hardware setup ------------------------------------------------------- */
void pinMode(GPIO_TypeDef* port, uint32_t pin, uint32_t mode);
static void enablePullUp(GPIO_TypeDef* port, uint32_t pin);
static void initClock(void);
static void initSysTick(void);
static void setupIO(void);

/* --- Utility / timing / random ------------------------------------------- */
void delay(volatile uint32_t ms);
uint32_t xorshift32(void);
static uint8_t shouldFire(uint8_t threshold);
static uint32_t randomFireDelay(void);
static uint32_t randomMainAlienDelay(void);
uint8_t get_random_bit(uint32_t* state);
int isInside(uint16_t x1,
             uint16_t y1,
             uint16_t w,
             uint16_t h,
             uint16_t px,
             uint16_t py);

/* --- Background / screen helpers ----------------------------------------- */
void loadBackground(void);
void clearDisplay(void);

/* --- Game initialisation -------------------------------------------------- */
static void parkAlienBullet(int col);
static void initAlienGrid(void);
static void initGameState(void);
static void resetGame(void);

/* --- Menu / app state screens -------------------------------------------- */
void mainMenu(AppState* as);
void help(AppState* as);
void getPause(PlayingState* ps, AppState* as);
void playing(AppState* as);
void showScoreBoard(AppState* as);
void selectMode(AppState* as, GameMode* gm);

/* --- Input
   ---------------------------------------------------------------- */
static void handleInput(PlayingState* ps, AppState* as);

/* --- Alien movement / firing --------------------------------------------- */
static int isAlienGridMt(void);
static uint16_t getLowestAlienY(void);
static void moveAliens(uint32_t now);
static void updateAlienFire(uint32_t now);
static void moveMainAlien(MainAlien* ma);

/* --- Collision / bullet state -------------------------------------------- */
static int checkCollision(uint16_t o1X,
                          uint16_t o1Y,
                          uint16_t o1w,
                          uint16_t o1h,
                          uint16_t o2X,
                          uint16_t o2Y,
                          uint16_t o2w,
                          uint16_t o2h);
static void resetAlienBullet(int col);
static void resetPlayerBullet(void);
static void repairAliensUnderRect(uint16_t rx,
                                  uint16_t ry,
                                  uint16_t rw,
                                  uint16_t rh);
static int updateAlienBulletCollision(void);
static void updatePlayerCollision(void);
static int checkGameOver(void);
static void updateMainAlienBulletCollision(void);

/* --- Rendering
   ------------------------------------------------------------ */
static void renderAliens(void);
static void renderShip(void);
static void renderPlayerBullet(void);
static void renderAlienBullets(void);
static void renderBarricade(void);
static void renderScene(void);
static void renderStats(void);
static void renderGameOverScreen(PlayingState* ps, AppState* as);
static void splashScreen();
static void makeBackground(int starCount);

/*
 * GLOBAL DECLARATIONS
 */

/* --- Sound engine state
   --------------------------------------------------- */
volatile const uint32_t* channel1_notes = 0;
volatile const uint32_t* channel1_times = 0;
volatile uint32_t channel1_note_count = 0;
volatile uint32_t channel1_repeat = 0;
volatile uint32_t channel1_note_index = 0;
volatile int32_t channel1_note_timer = 0;
volatile uint32_t milliseconds = 0;

volatile const uint32_t* channel2_notes = 0;
volatile const uint32_t* channel2_times = 0;
volatile uint32_t channel2_note_count = 0;
volatile uint32_t channel2_repeat = 0;
volatile uint32_t channel2_note_index = 0;
volatile int32_t channel2_note_timer = 0;

/* --- Sound effect data ---------------------------------------------------- */
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

const uint32_t aliens_spawning_notes[1] = {G6, B6, G6};
const uint32_t aliens_spawning_times[1] = {220, 220, 220};
const uint32_t aliens_spawning_note_count = 3;

/* --- Runtime game globals ------------------------------------------------- */
static uint32_t lastUpdate = 0;
static const uint32_t FRAME_DELAY = 16;

AppState currentAppState = MAINMENU;
GameMode currentGameMode = EASY;

static GameState gs;
static uint32_t randState;

static ScoreRecord records[MAX_RECORDS] = {
    {"___", 0000}, {"___", 0000}, {"___", 0000}, {"___", 0000}, {"___", 0000},
};

/*
 * ASSET DECLARATIONS
 */

static const uint16_t spaceShip[] = {

    0,     0,     0,     0,     0,     34882, 34882, 0,     0,     0,     0,
    0,     0,     0,     0,     0,     34882, 37012, 37012, 54701, 0,     0,
    0,     0,     0,     0,     0,     0,     12692, 33585, 33585, 28820, 0,
    0,     0,     0,     0,     0,     0,     9521,  41769, 61181, 61181, 41769,
    42818, 0,     0,     0,     0,     0,     0,     10058, 49961, 18138, 18138,
    41769, 52595, 0,     0,     0,     0,     0,     21933, 52067, 49961, 18138,
    18138, 49961, 28292, 21933, 0,     0,     63414, 28820, 49961, 28027, 41769,
    41769, 41769, 41769, 19299, 41769, 37012, 63670, 35154, 37012, 49961, 28027,
    28820, 37012, 37012, 37012, 19299, 41769, 37012, 35154, 0,     35154, 41769,
    27755, 37012, 35154, 35154, 37012, 28292, 41769, 35154, 0,     0,     0,
    34361, 34361, 34369, 47302, 47302, 34882, 34361, 34361, 0,     0,     0,
    0,     0,     0,     34369, 34882, 33577, 34882, 0,     0,     0,     0,
    0,     0,     0,     0,     0,     45196, 21933, 0,     0,     0,     0,
    0,
};

static const uint16_t blueAlienBoth[][88] = {
    {0,     0,     41187, 0,     0,     0,     0,     0,     41187, 0,
     0,     41187, 0,     0,     41187, 0,     0,     0,     41187, 0,
     0,     41187, 41187, 0,     41187, 41187, 41187, 41187, 41187, 41187,
     41187, 0,     41187, 41187, 41187, 41187, 0,     41187, 41187, 41187,
     0,     41187, 41187, 41187, 41187, 41187, 41187, 41187, 41187, 41187,
     41187, 41187, 41187, 41187, 41187, 0,     41187, 41187, 41187, 41187,
     41187, 41187, 41187, 41187, 41187, 0,     0,     0,     41187, 0,
     0,     0,     0,     0,     41187, 0,     0,     0,     41187, 0,
     0,     0,     0,     0,     0,     0,     41187, 0},
    {0,     0,     41187, 0,     0,     0,     0,     0,     41187, 0,
     0,     0,     0,     0,     41187, 0,     0,     0,     41187, 0,
     0,     0,     0,     0,     41187, 41187, 41187, 41187, 41187, 41187,
     41187, 0,     0,     0,     41187, 41187, 0,     41187, 41187, 41187,
     0,     41187, 41187, 0,     41187, 41187, 41187, 41187, 41187, 41187,
     41187, 41187, 41187, 41187, 41187, 41187, 0,     41187, 41187, 41187,
     41187, 41187, 41187, 41187, 0,     41187, 41187, 0,     41187, 0,
     0,     0,     0,     0,     41187, 0,     41187, 0,     0,     0,
     41187, 41187, 0,     41187, 41187, 0,     0,     0}};
static const uint16_t redAlien[][88] = {

    {
        0,     0,     65320, 0,     0,     0,     0,     0,     65320, 0,
        0,     65320, 0,     0,     65320, 0,     0,     0,     65320, 0,
        0,     65320, 65320, 0,     65320, 65320, 65320, 65320, 65320, 65320,
        65320, 0,     65320, 65320, 65320, 65320, 0,     65320, 65320, 65320,
        0,     65320, 65320, 65320, 65320, 65320, 65320, 65320, 65320, 65320,
        65320, 65320, 65320, 65320, 65320, 0,     65320, 65320, 65320, 65320,
        65320, 65320, 65320, 65320, 65320, 0,     0,     0,     65320, 0,
        0,     0,     0,     0,     65320, 0,     0,     0,     65320, 0,
        0,     0,     0,     0,     0,     0,     65320, 0,
    },
    {
        0,     0,     65320, 0,     0,     0,     0,     0,     65320, 0,
        0,     0,     0,     0,     65320, 0,     0,     0,     65320, 0,
        0,     0,     0,     0,     65320, 65320, 65320, 65320, 65320, 65320,
        65320, 0,     0,     0,     65320, 65320, 0,     65320, 65320, 65320,
        0,     65320, 65320, 0,     65320, 65320, 65320, 65320, 65320, 65320,
        65320, 65320, 65320, 65320, 65320, 65320, 0,     65320, 65320, 65320,
        65320, 65320, 65320, 65320, 0,     65320, 65320, 0,     65320, 0,
        0,     0,     0,     0,     65320, 0,     65320, 0,     0,     0,
        65320, 65320, 0,     65320, 65320, 0,     0,     0,
    }};
static const uint16_t greenAlien[][88] = {
    {0,     0,     51975, 0,     0,     0,     0,     0,     51975, 0,
     0,     51975, 0,     0,     51975, 0,     0,     0,     51975, 0,
     0,     51975, 51975, 0,     51975, 51975, 51975, 51975, 51975, 51975,
     51975, 0,     51975, 51975, 51975, 51975, 0,     51975, 51975, 51975,
     0,     51975, 51975, 51975, 51975, 51975, 51975, 51975, 51975, 51975,
     51975, 51975, 51975, 51975, 51975, 0,     51975, 51975, 51975, 51975,
     51975, 51975, 51975, 51975, 51975, 0,     0,     0,     51975, 0,
     0,     0,     0,     0,     51975, 0,     0,     0,     51975, 0,
     0,     0,     0,     0,     0,     0,     51975, 0},
    {
        0,     0,     51975, 0,     0,     0,     0,     0,     51975, 0,
        0,     0,     0,     0,     51975, 0,     0,     0,     51975, 0,
        0,     0,     0,     0,     51975, 51975, 51975, 51975, 51975, 51975,
        51975, 0,     0,     0,     51975, 51975, 0,     51975, 51975, 51975,
        0,     51975, 51975, 0,     51975, 51975, 51975, 51975, 51975, 51975,
        51975, 51975, 51975, 51975, 51975, 51975, 0,     51975, 51975, 51975,
        51975, 51975, 51975, 51975, 0,     51975, 51975, 0,     51975, 0,
        0,     0,     0,     0,     51975, 0,     51975, 0,     0,     0,
        51975, 51975, 0,     51975, 51975, 0,     0,     0,
    }};
static const uint16_t explosion[88] = {
    0,    0,     0,     0,     0,     0,     21809, 2105,  1073,  2105,  0,
    2105, 61747, 53562, 21809, 53562, 61747, 15139, 15139, 2105,  2105,  2105,
    0,    1073,  21809, 32158, 15139, 64354, 24375, 7013,  24375, 2105,  2105,
    0,    15139, 7013,  15139, 24375, 32158, 24375, 7013,  64354, 53562, 0,
    0,    61747, 7013,  7013,  24375, 24375, 24375, 15139, 7013,  53562, 0,
    0,    15139, 15139, 21809, 15139, 15139, 7013,  7013,  21809, 11313, 0,
    0,    0,     0,     7013,  61747, 7013,  32158, 64354, 11313, 0,     0,
    0,    0,     0,     53562, 1073,  21809, 61747, 2105,  0,     0,     0};

// static const uint16_t mainAlienSpr[352] = {
//     0,     0,     0,     0,     65320, 65320, 0,     0,     0,     0,     0,
//     0,     0,     0,     0,     0,     65320, 65320, 0,     0,     0,     0,
//     0,     0,     0,     0,     65320, 65320, 0,     0,     0,     0,     0,
//     0,     0,     0,     0,     0,     65320, 65320, 0,     0,     0,     0,
//     65320, 65320, 0,     0,     0,     0,     65320, 65320, 0,     0,     0,
//     0,     0,     0,     65320, 65320, 0,     0,     0,     0,     65320,
//     65320, 65320, 65320, 0,     0,     0,     0,     65320, 65320, 0,     0,
//     0, 0,     0,     0,     65320, 65320, 0,     0,     0,     0,     65320,
//     65320, 65320, 65320, 0,     0,     65320, 65320, 65320, 65320, 65320,
//     65320, 65320, 65320, 65320, 65320, 65320, 65320, 65320, 65320, 0,     0,
//     65320, 65320, 65320, 65320, 0,     0,     65320, 65320, 65320, 65320,
//     65320, 65320, 65320, 65320, 65320, 65320, 65320, 65320, 65320, 65320, 0,
//     0,     65320, 65320, 65320, 65320, 65320, 65320, 65320, 65320, 0,     0,
//     65320, 65320, 65320, 65320, 65320, 65320, 0,     0,     65320, 65320,
//     65320, 65320, 65320, 65320, 65320, 65320, 65320, 65320, 65320, 65320, 0,
//     0,     65320, 65320, 65320, 65320, 65320, 65320, 0,     0,     65320,
//     65320, 65320, 65320, 65320, 65320, 65320, 65320, 65320, 65320, 65320,
//     65320, 65320, 65320, 65320, 65320, 65320, 65320, 65320, 65320, 65320,
//     65320, 65320, 65320, 65320, 65320, 65320, 65320, 65320, 65320, 65320,
//     65320, 65320, 65320, 65320, 65320, 65320, 65320, 65320, 65320, 65320,
//     65320, 65320, 65320, 65320, 65320, 65320, 65320, 65320, 65320, 0,     0,
//     65320, 65320, 65320, 65320, 65320, 65320, 65320, 65320, 65320, 65320,
//     65320, 65320, 65320, 65320, 65320, 65320, 65320, 65320, 0,     0, 0, 0,
//     65320, 65320, 65320, 65320, 65320, 65320, 65320, 65320, 65320, 65320,
//     65320, 65320, 65320, 65320, 65320, 65320, 65320, 65320, 0,     0, 0, 0,
//     0,     0,     65320, 65320, 0,     0,     0,     0,     0, 0,     0, 0,
//     0,     0,     65320, 65320, 0,     0,     0,     0, 0,     0,     0, 0,
//     65320, 65320, 0,     0,     0,     0,     0, 0,     0,     0,     0, 0,
//     65320, 65320, 0,     0,     0,     0, 0,     0,     65320, 65320, 0, 0,
//     0,     0,     0,     0,     0, 0,     0,     0,     0,     0,     0, 0,
//     65320, 65320, 0,     0, 0,     0,     65320, 65320, 0,     0,     0, 0,
//     0,     0,     0, 0,     0,     0,     0,     0,     0,     0,     65320,
//     65320, 0,     0,
// };

static const uint16_t mainAlienSpr[352] = {
    0,     0,     0,     0,     65320, 65320, 0,     0,     0,     0,     0,
    0,     0,     0,     0,     0,     65320, 65320, 0,     0,     0,     0,
    0,     0,     0,     0,     65320, 65320, 0,     0,     0,     0,     0,
    0,     0,     0,     0,     0,     65320, 65320, 0,     0,     0,     0,
    65320, 0,     0,     0,     0,     0,     65320, 65320, 0,     65320, 0,
    0,     0,     0,     65320, 65320, 0,     0,     0,     0,     0,     65320,
    65320, 65320, 0,     0,     0,     0,     65320, 65320, 0,     0,     0,
    65320, 0,     0,     65320, 65320, 0,     0,     0,     0,     65320, 65320,
    65320, 65320, 0,     0,     65320, 65320, 65320, 65320, 65320, 65320, 65320,
    65320, 65320, 65320, 65320, 65320, 65320, 65320, 0,     0,     65320, 65320,
    65320, 65320, 0,     0,     65320, 65320, 65320, 65320, 65320, 65320, 65320,
    65320, 65320, 65320, 65320, 65320, 65320, 65320, 0,     0,     65320, 65320,
    65320, 65320, 65320, 65320, 65320, 65321, 27663, 27663, 65320, 65321, 65320,
    65320, 65320, 65320, 27663, 27663, 65320, 65320, 65320, 65320, 65320, 65320,
    65320, 65320, 65320, 65320, 65320, 65321, 27663, 27663, 65320, 65320, 65320,
    65320, 65320, 65320, 27663, 27663, 65320, 65320, 65320, 65320, 65320, 65320,
    65320, 65320, 65320, 65320, 27663, 27663, 57600, 57600, 27663, 27663, 65320,
    65320, 27663, 27663, 57600, 57600, 27663, 27663, 65320, 65320, 65320, 65320,
    65320, 65320, 65320, 65320, 27663, 27663, 57600, 57600, 27663, 27663, 65320,
    65320, 27663, 27663, 57600, 57600, 27663, 27663, 65320, 65320, 65320, 65320,
    65320, 65320, 65320, 65320, 65320, 65320, 27663, 27663, 65321, 65320, 27663,
    27663, 65320, 65320, 27663, 27663, 65320, 65320, 65320, 65320, 65320, 65320,
    0,     65320, 65320, 65320, 65320, 65320, 27663, 27663, 65320, 65320, 27663,
    27663, 65320, 65320, 27663, 27663, 65320, 65320, 65320, 65320, 65320, 0,
    0,     65320, 65320, 65320, 0,     0,     65320, 65320, 65320, 65320, 65320,
    65320, 65320, 65320, 65320, 65320, 0,     0,     65320, 65320, 65320, 0,
    0,     65320, 65320, 65320, 16151, 0,     65320, 65320, 17144, 17144, 65320,
    65320, 17144, 17144, 65320, 65320, 0,     0,     65320, 65320, 65320, 0,
    0,     0,     65320, 65320, 0,     0,     0,     0,     17144, 17144, 0,
    0,     17144, 17144, 0,     0,     0,     0,     65320, 65320, 0,     0,
    0,     0,     65320, 65320, 0,     0,     0,     0,     0,     0,     0,
    0,     0,     0,     0,     0,     0,     0,     65320, 65320, 0,     0,
};

/*
 * 10. MAIN
 */
int main(void) {
  /* Hardware */
  initClock();
  initSysTick();
  setupIO();
  initSerial();
  initSound();
  initSound2();

  splashScreen();
  // Game Begins
  while (1) {
    // Switch statement for our Game State
    switch (currentAppState) {
      case MAINMENU:
        mainMenu(&currentAppState);
        break;
      case HELP:
        help(&currentAppState);
        break;
      case PLAYING:
        start_sound_effect_ch1(enter_game_notes_ch1, enter_game_times_ch1,
                               enter_game_note_count_ch1, 0);
        start_sound_effect_ch2(enter_game_notes_ch2, enter_game_times_ch2,
                               enter_game_note_count_ch2, 0);
        delay(500);
        playing(&currentAppState);
        break;
      case RECORD:
        showScoreBoard(&currentAppState);
        break;
      case MODE:
        delay(500);
        selectMode(&currentAppState, &currentGameMode);
        break;
      default:
        break;
    }
  }

  return 0;
}

/*
 * FUNCTION DEFINITIONS
 */

/* --- Sound / timing / interrupts ----------------------------------------- */
void start_sound_effect_ch1(const uint32_t notes[],
                            const uint32_t times[],
                            uint32_t count,
                            uint32_t repeat) {
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
void start_sound_effect_ch2(const uint32_t notes[],
                            const uint32_t times[],
                            uint32_t count,
                            uint32_t repeat) {
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
void SysTick_Handler(void) {
  milliseconds++;

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

static void stop_sound_effect_ch1(void) {
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
static void stop_sound_effect_ch2(void) {
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
/* --- Hardware setup ------------------------------------------------------- */
void pinMode(GPIO_TypeDef* port, uint32_t pin, uint32_t mode) {
  uint32_t val = port->MODER;
  val &= ~(3u << (pin * 2));
  val |= (mode << (pin * 2));
  port->MODER = val;
}
static void enablePullUp(GPIO_TypeDef* port, uint32_t pin) {
  port->PUPDR &= ~(3u << (pin * 2));
  port->PUPDR |= (1u << (pin * 2));
}
static void initClock(void) {
  RCC->CR &= ~(1u << 24);
  while (RCC->CR & (1 << 25))
    ;

  FLASH->ACR |= (1 << 0); /* 1 wait-state     */
  FLASH->ACR &= ~((1u << 2) | (1u << 1));
  FLASH->ACR |= (1 << 4); /* prefetch on      */

  RCC->CFGR &= ~((1u << 21) | (1u << 20) | (1u << 19) |
                 (1u << 18)); /* PLL x12 -> 48MHz */
  RCC->CFGR |= ((1 << 21) | (1 << 19));
  RCC->CFGR |= (1 << 14); /* ADC /4           */
  RCC->CR |= (1 << 24);   /* PLL on           */
  RCC->CFGR |= (1 << 1);  /* PLL as sysclk    */
}
static void initSysTick(void) {
  SysTick->LOAD = 48000;
  SysTick->CTRL = 7;
  SysTick->VAL = 10;
  __asm(" cpsie i ");
}
static void setupIO(void) {
  RCC->AHBENR |= (1 << 18) | (1 << 17);
  display_begin();
  pinMode(GPIOB, 4, 0);
  enablePullUp(GPIOB, 4); /* right - PB4  */
  pinMode(GPIOB, 5, 0);
  enablePullUp(GPIOB, 5); /* left  - PB5  */
  pinMode(GPIOA, 8, 0);
  enablePullUp(GPIOA, 8); /* fire  - PA8  */
  pinMode(GPIOA, 11, 0);
  enablePullUp(GPIOA, 11); /* down  - PA11 */
  pinMode(GPIOA, 2, 1);    /* Lives indicator 1 - PA9*/
  pinMode(GPIOA, 1, 1);    /* Lives indicator 1 - PA10*/
  pinMode(GPIOB, 3, 1);    /* Lives indicator 1 - PC15*/
  pinMode(GPIOA, 9, 0);    /* Reset Button */
  enablePullUp(GPIOA, 9);
}

// static void lightLivesIndicator(int lives) {
//   // Clear LEDs
//   GPIOA->ODR &= ~((1 << 2) | (1 << 1));
//   GPIOB->ODR &= ~(1 << 3);

//   // temp
//   // GPIOC->ODR |= (1 << 3);

//   // Set LEDs based on lives
//   GPIOA->ODR |= (lives >= 1) ? (1 << 1) : 0;
//   GPIOB->ODR |= (lives >= 2) ? (1 << 3) : 0;
//   GPIOA->ODR |= (lives == 3) ? (1 << 2) : 0;
// }

static void lightLivesIndicator(int lives) {
  // Clear all LEDs
  GPIOA->BSRR = (1 << (1 + 16)) | (1 << (2 + 16));
  GPIOB->BSRR = (1 << (3 + 16));

  // Set LEDs
  if (lives >= 1)
    GPIOA->BSRR = (1 << 1);
  if (lives >= 2)
    GPIOB->BSRR = (1 << 3);
  if (lives >= 3)
    GPIOA->BSRR = (1 << 2);
}
/* --- Utility / timing / random -------------------------------------------
 */
void delay(volatile uint32_t ms) {
  uint32_t end = ms + milliseconds;
  while (milliseconds != end)
    __asm(" wfi ");
}
uint32_t xorshift32() {
  randState ^= randState << 13;
  randState ^= randState >> 17;
  randState ^= randState << 5;
  return randState;
}
static uint8_t shouldFire(uint8_t threshold) {
  return (xorshift32() % threshold) ==
         0;  // threshold => more higher less likely to fire
  /*
  threshold = 3:
  possible results of % 3 → { 0, 1, 2 }
  only 0 returns 1 → 1 in 3 chance = 33%
  */
}
static uint32_t randomFireDelay(void) {
  uint32_t range = ALIEN_FIRE_MAX_MS - ALIEN_FIRE_MIN_MS;
  return ALIEN_FIRE_MIN_MS + (xorshift32() % range);
}

static uint32_t randomMainAlienDelay(void) {
  uint32_t range = MAIN_ALIEN_MAX_DELAY - MAIN_ALIEN_MIN_DELAY;
  return MAIN_ALIEN_MIN_DELAY + (xorshift32() % range);
}

uint8_t get_random_bit(uint32_t* state) {
  (void)state;
  return xorshift32() & 1u;
}
int isInside(uint16_t x1,
             uint16_t y1,
             uint16_t w,
             uint16_t h,
             uint16_t px,
             uint16_t py) {
  return (px >= x1 && px <= x1 + w && py >= y1 && py <= y1 + h) ? 1 : 0;
}

/* --- Background / screen helpers ----------------------------------------- */
void loadBackground() {
  makeBackground(40);
  printTextBold("Space", 10, 10, 1, 0);
  printTextBold("Invader", 40, 20, 1, 0);

  putImage(20, 20, 11, 8, greenAlien[0], 1, 0);
  putImage(80, 10, 11, 8, greenAlien[1], 1, 0);
  for (int i = 0; i < 7; i++) {
    const uint16_t* spr = (i % 3 == 0)   ? redAlien[i % 2]
                          : (i % 3 == 1) ? greenAlien[i % 2]
                                         : blueAlienBoth[i % 2];
    putImage(i * 18, 145, 11, 8, spr, i % 2, 0);
  }
  // putImage(110, 0, 128,50, bottom_background, 1, 0);
}
void clearDisplay() {
  fillRectangle(0, 0, SCREEN_W, SCREEN_H, 0);
}

/* --- Game initialisation -------------------------------------------------- */
static void parkAlienBullet(int col) {
  AlienGrid* ag = &gs.aliens;
  Bullet* b = &ag->ab[col];

  // find the bottom alive alien in the column

  int row = -1;
  for (int i = ALIEN_ROWS - 1; i >= 0; i--) {
    if (ag->status[i][col] == 0) {
      row = i;
      break;
    }
  }

  // no
  if (row == -1) {
    b->state = BULLET_READY;
    return;
  }

  b->coords.x = b->coords.oldX =
      (uint16_t)(ag->offsetX + ag->basePosX[col] + BULLET_OFFSET_X);
  b->coords.y = b->coords.oldY =
      (uint16_t)(ag->offsetY + ag->basePosY[row] + ALIEN_H);

  b->speed = ALIEN_BULLET_SPEED;
  b->state = BULLET_READY;
}
static void initAlienGrid() {
  AlienGrid* ag = &gs.aliens;
  ag->offsetX = ag->oldOffsetX = ALIEN_ORIGIN_X;
  ag->offsetY = ag->oldOffsetY = ALIEN_ORIGIN_Y;
  ag->dirX = -1; /* start moving right */
  ag->earToggle = 0;
  ag->lastMoveTime = 0;

  /* Precompute relative per-cell offsets (never change during play) */
  for (int i = 0; i < ALIEN_ROWS; i++)
    ag->basePosY[i] = (uint16_t)(i * (ALIEN_H + ALIEN_GAP_Y));
  for (int j = 0; j < ALIEN_COLS; j++)
    ag->basePosX[j] = (uint16_t)(j * (ALIEN_W + ALIEN_GAP_X));

  // putImage((uint16_t)(ag->offsetX + ag->basePosX[j]),
  //                (uint16_t)(ag->offsetY + ag->basePosY[i]), ALIEN_W, ALIEN_H,
  //                redAlien[ag->earToggle], 1, 0);

  /* All aliens alive */
  for (int i = 0; i < ALIEN_ROWS; i++)
    for (int j = 0; j < ALIEN_COLS; j++)
      ag->status[i][j] = 0;

  for (int i = 0; i < ALIEN_COLS; i++) {
    parkAlienBullet(i);
    ag->nextFireTime[i] = milliseconds + randomFireDelay();
  }
}

static void initMainAlien(MainAlien* ma) {
  ma->coords.x = 0;
  ma->coords.y = MAIN_ALIEN_Y;
  ma->coords.oldX = 0;
  ma->coords.oldY = MAIN_ALIEN_Y;

  ma->speed = 2;
  ma->dir = 1;
  ma->active = 0;

  ma->nextSpawnTime = milliseconds + randomMainAlienDelay();
  ma->lastMoveTime = 0;
}

static void initGameState() {
  gs.highScore = records[0].score;
  randState = milliseconds | 1; /* Ship */

  if (currentGameMode == EASY) {
    gs.alienSpeed = ALIEN_MOVE_MS_EASY;
    gs.score_inc = SCORE_INC_EASY;
  } else if (currentGameMode == MEDIUM) {
    gs.alienSpeed = ALIEN_MOVE_MS_MED;
    gs.score_inc = SCORE_INC_MED;
  } else {
    gs.alienSpeed = ALIEN_MOVE_MS_HARD;
    gs.score_inc - SCORE_INC_HARD;
  }

  gs.ship.coords.x = gs.ship.coords.oldX = 58;
  gs.ship.coords.y = gs.ship.coords.oldY = 130;
  gs.ship.speed = SHIP_SPEED;

  /* Bullet - parked at muzzle */
  gs.bullet.coords.x = gs.bullet.coords.oldX =
      gs.ship.coords.x + BULLET_OFFSET_X;
  gs.bullet.coords.y = gs.bullet.coords.oldY =
      gs.ship.coords.y - BULLET_OFFSET_Y;
  gs.bullet.speed = BULLET_SPEED;
  gs.bullet.state = BULLET_READY;

  /* Alien grid */
  initAlienGrid();
  /* Main alien*/
  initMainAlien(&gs.mysteryAlien);
}
static void resetGame(void) {
  /* Blank the whole play area above the HUD line */
  fillRectangle(0, 0, SCREEN_W, HUD_LINE_Y, 0);

  /* Re-initialise all game state */
  initGameState();  // 0 to preserve Alien Grid

  /* Redraw the initial scene */
  renderAliens();
  putImage(gs.ship.coords.x, gs.ship.coords.y, SHIP_W, SHIP_H, spaceShip, 1, 0);
  /* HUD line stays - was never erased */
}

/* --- Menu / app state screens -------------------------------------------- */
void mainMenu(AppState* as) {
  clearDisplay();

  loadBackground();

  int selectedOption = 0;

  uint16_t normal = RGBToWord(0xff, 0xff, 0);
  uint16_t highlighted = RGBToWord(211, 211, 211);
  uint16_t startButton = normal;
  uint16_t helpButton = normal;
  uint16_t scoreButton = normal;
  uint16_t modeButton = normal;

  int done = 0;

  while (!done) {
    char c = 0;
    if (serialCharAvailable()) {
      c = serialGetCharNonBlocking();
    }

    if (selectedOption == 0) {
      startButton = highlighted;
      helpButton = normal;
      scoreButton = normal;
      modeButton = normal;
    }
    if (selectedOption == 1) {
      helpButton = highlighted;
      scoreButton = normal;
      startButton = normal;
      modeButton = normal;
    }
    if (selectedOption == 2) {
      scoreButton = highlighted;
      startButton = normal;
      helpButton = normal;
      modeButton = normal;
    }

    if (selectedOption == 3) {
      modeButton = highlighted;
      startButton = normal;
      helpButton = normal;
      scoreButton = normal;
    }

    printText("Start Game", 20, 60, startButton, 0);
    printText("Help", 20, 80, helpButton, 0);
    printText("High Scores", 20, 100, scoreButton, 0);
    printText("Select Mode", 20, 120, modeButton, 0);

    if (((GPIOA->IDR & (1 << 11)) == 0) || c == 's' || c == 'S') {
      while ((GPIOA->IDR & (1 << 11)) == 0)
        ; /* wait for release */
      delay(50);

      if (selectedOption < 3) {
        selectedOption++;
        delay(100);
      }
    }

    if (((GPIOA->IDR & (1 << 8)) == 0) || c == 'w' || c == 'W') {
      while ((GPIOA->IDR & (1 << 11)) == 0)
        ; /* wait for release */
      delay(50);
      if (selectedOption > 0) {
        selectedOption--;
        delay(100);
      }
    }

    if (((GPIOB->IDR & (1 << 4)) == 0) || c == ' ' || c == '\r' || c == '\n' ||
        c == 'd' || c == 'D') {
      if (selectedOption == 0) {
        *as = PLAYING;
        done = 1;
      }
      if (selectedOption == 1) {
        *as = HELP;
        done = 1;
      }
      if (selectedOption == 2) {
        *as = RECORD;
        done = 1;
      }
      if (selectedOption == 3) {
        *as = MODE;
        done = 1;
      }
    }
  }
}
void help(AppState* as) {
  clearDisplay();

  int done = 0;

  while (!done) {
    char c = 0;
    if (serialCharAvailable()) {
      c = serialGetCharNonBlocking();
    }

    printTextX2("HELP", 40, 0, RGBToWord(255, 255, 0), 0);

    printText("Control spaceship:", 0, 20, RGBToWord(255, 255, 255), 0);
    printText("Left/Right buttons", 0, 30, RGBToWord(255, 255, 255), 0);
    printText("FIRE = UP BUTTON", 0, 60, RGBToWord(255, 255, 255), 0);
    printText("PAUSE = DOWN BUTTON", 0, 80, RGBToWord(255, 255, 255), 0);

    printText("EXIT HELP = DOWN BUTTON", 0, 120, RGBToWord(255, 255, 255), 0);

    if (((GPIOA->IDR & (1 << 11)) == 0) || c == 's' || c == 'S' || c == '\r' ||
        c == '\n') {
      *as = MAINMENU;
      done = 1;
    }
  }
}

void selectMode(AppState* as, GameMode* gm) {
  clearDisplay();

  loadBackground();
  printText("Game Level:", 20, 40, 65535, 0);

  int selectedOption = *gm;

  uint16_t normal = RGBToWord(0xff, 0xff, 0);
  uint16_t highlighted = RGBToWord(211, 211, 211);
  uint16_t startButton = normal;
  uint16_t helpButton = normal;
  uint16_t scoreButton = normal;
  uint16_t modeButton = normal;

  while (1) {
    char c = 0;
    if (serialCharAvailable()) {
      c = serialGetCharNonBlocking();
    }

    if (selectedOption == 0) {
      startButton = highlighted;
      helpButton = normal;
      scoreButton = normal;
    }
    if (selectedOption == 1) {
      helpButton = highlighted;
      scoreButton = normal;
      startButton = normal;
    }
    if (selectedOption == 2) {
      scoreButton = highlighted;
      startButton = normal;
      helpButton = normal;
    }

    printText("Easy", 20, 60, startButton, 0);
    printText("Medium", 20, 80, helpButton, 0);
    printText("Hard", 20, 100, scoreButton, 0);

    if (((GPIOA->IDR & (1 << 11)) == 0) || c == 's' || c == 'S') {
      if (selectedOption < 2) {
        selectedOption++;
        delay(200);
      }
    }

    if (((GPIOA->IDR & (1 << 8)) == 0) || c == 'w' || c == 'W') {
      if (selectedOption > 0) {
        selectedOption--;
        delay(200);
      }
    }

    if (((GPIOB->IDR & (1 << 4)) == 0) || c == ' ' || c == '\r' || c == '\n' ||
        c == 'd' || c == 'D') {
      if (selectedOption == 0) {
        *gm = EASY;
        *as = MAINMENU;
        return;
      }
      if (selectedOption == 1) {
        *gm = MEDIUM;
        *as = MAINMENU;
        return;
      }
      if (selectedOption == 2) {
        *gm = HARD;
        *as = MAINMENU;
        return;
      }

      delay(200);
    }
  }
}

void showScoreBoard(AppState* as) {
  clearDisplay();

  /* --- Heading ---------------------------------------------------------- */
  uint16_t headingColor = RGBToWord(255, 215, 0);  /* gold              */
  uint16_t normalColor = RGBToWord(255, 255, 255); /* white             */
  uint16_t topColor = RGBToWord(255, 215, 0);      /* gold for top score*/
  uint16_t rowColor = RGBToWord(180, 180, 180);    /* grey for rest     */

  printTextBold("SCOREBOARD", 5, 5, headingColor, 0);

  /* --- Divider ---------------------------------------------------------- */
  drawLine(0, 20, SCREEN_W, 20, headingColor);

  /* --- Column headers --------------------------------------------------- */
  printText("NAME", 10, 26, normalColor, 0);
  printText("SCORE", 80, 26, normalColor, 0);
  drawLine(0, 35, SCREEN_W, 35, RGBToWord(80, 80, 80));

  /* --- Rows ------------------------------------------------------------- */
  for (int i = 0; i < MAX_RECORDS; i++) {
    uint16_t y = (uint16_t)(42 + i * 18);
    uint16_t color = (i == 0) ? topColor : rowColor;

    char number = (i + 1) + '0';
    char index[2] = {number, '\0'};

    /* Rank number */
    printText(index, 1, y, color, 0);
    printText(".", 8, y, color, 0);

    /* Initials */
    printText(records[i].initials, 18, y, color, 0);

    /* Score – right aligned at x=80 */
    printNumber(records[i].score, 80, y, color, 0);

    /* Underline each row */
    drawLine(0, (uint16_t)(y + 12), SCREEN_W, (uint16_t)(y + 12),
             RGBToWord(40, 40, 40));
  }

  /* --- Footer ----------------------------------------------------------- */
  drawLine(0, 148, SCREEN_W, 148, headingColor);
  printText("DOWN to go back", 10, 152, RGBToWord(100, 100, 100), 0);

  /* --- Wait for down button to exit ------------------------------------- */
  while ((GPIOA->IDR & (1 << 11)) != 0)
    ; /* wait for press  */
  delay(50);
  while ((GPIOA->IDR & (1 << 11)) == 0)
    ; /* wait for release */
  delay(50);

  *as = MAINMENU;

  return;
}

void getPause(PlayingState* ps, AppState* as) {
  uint16_t pinkColor = RGBToWord(255, 20, 147);

  int selected = 0; /* 0 = Resume, 1 = Main Menu */
  if (*ps == GAMEOVER) {
    clearDisplay();
    printTextBold("GAMEOVER!!", 5, 10, RGBToWord(255, 255, 0), 0);

    ScoreRecord newEntry;
    char alpha = 'A';
    char alphaStr[2] = {alpha, '\0'};
    int position = 0;
    char initials[4] = {'_', '_', '_', '\0'};

    if (gs.score > gs.highScore) {
      printText("New Highscore", 20, 22, RGBToWord(255, 255, 255), 0);
      printTextX2(initials, 50, 40, RGBToWord(255, 255, 0), 0);
      printTextX2("<", 40, 60, pinkColor, 0);
      printTextX2(alphaStr, 60, 60, pinkColor, 0);
      printTextX2(">", 80, 60, pinkColor, 0);

      printText("< > : Navigate", 10, 92, RGBToWord(255, 255, 255), 0);
      printText("UP : Select", 10, 102, RGBToWord(255, 255, 255), 0);
      printText("Corner : Reset", 10, 112, RGBToWord(255, 255, 255), 0);

      printText("Press down key", 10, 125, RGBToWord(255, 255, 255), 0);
      printText("To save and exit", 10, 135, RGBToWord(255, 255, 255), 0);

      while (1) {
        if (!(GPIOA->IDR & (1 << 11))) {  // down key to save and exit
          /* Quit to main menu */
          goto save_and_exit;
        }

        /* right button to select letters-------------- PB4*/

        if (!(GPIOB->IDR & (1 << 4))) {  // right key
          alpha = (alpha >= 'Z') ? 'A' : alpha + 1;
          alphaStr[0] = alpha;
          fillRectangle(60, 60, 14, 16, 0);
          printTextX2(alphaStr, 60, 60, pinkColor, 0);
          delay(150);
        }

        /* left button to select letters-------------- PB5*/

        if (!(GPIOB->IDR & (1 << 5))) {  // left key
          alpha = (alpha <= 'A') ? 'Z' : alpha - 1;
          alphaStr[0] = alpha;
          fillRectangle(60, 60, 14, 16, 0);
          printTextX2(alphaStr, 60, 60, pinkColor, 0);
          delay(150);
        }

        /* Corner button to reset the namespace -------------- PA9*/
        if (!(GPIOA->IDR & (1 << 9))) {
          alpha = 'A';
          alphaStr[0] = alpha;
          position = 0;
          initials[0] = '_';
          initials[1] = '_';
          initials[2] = '_';
          printTextX2(initials, 50, 40, RGBToWord(255, 255, 0), 0);
          printTextX2(alphaStr, 60, 60, pinkColor, 0);
        }

        /* UP button to select letter to the current position*/
        if (!(GPIOA->IDR & (1 << 8))) {
          if (position >= 3)
            continue;
          initials[position] = alpha;
          position++;

          fillRectangle(50, 40, 70, 16, 0);
          printTextX2(initials, 50, 40, RGBToWord(255, 255, 0), 0);

          alpha = 'A';
          alphaStr[0] = alpha;
          fillRectangle(60, 60, 14, 16, 0);
          printTextX2(alphaStr, 60, 60, pinkColor, 0);

          delay(200);
        }
      }
    save_and_exit:

      /* Save to leaderboard */

      newEntry.initials[0] = initials[0];
      newEntry.initials[1] = initials[1];
      newEntry.initials[2] = initials[2];
      newEntry.initials[3] = '\0';
      newEntry.score = gs.score;

      for (int i = 0; i < MAX_RECORDS; i++) {
        if (newEntry.score > records[i].score) {
          for (int k = MAX_RECORDS - 1; k > i; k--)
            records[k] = records[k - 1];
          records[i] = newEntry;
          break;
        }
      }

      if (gs.score > gs.highScore) {
        gs.highScore = gs.score;
      }
      gs.score = 0;
      stop_sound_effect_ch1();
      stop_sound_effect_ch2();
      *ps = GAMERUNNING;
      *as = MAINMENU;
      return;
    } else {
      while (1) {
        stop_sound_effect_ch1();
        stop_sound_effect_ch2();
        putImage(55, 20, 11, 8, blueAlienBoth[1], 0, 1);
        printText("Press down key", 10, 125, RGBToWord(255, 255, 255), 0);
        printText("to go to main menu", 10, 135, RGBToWord(255, 255, 255), 0);

        if (!(GPIOA->IDR & (1 << 11))) {  // down key to save and exit
          gs.score = 0;

          *ps = GAMERUNNING;
          *as = MAINMENU;
          return;
        }
      }
    }
  } else {
    stop_sound_effect_ch2();
    /* Draw pause menu */
    fillRectangle(0, 60, SCREEN_W, 50, 0);
    printTextBold("PAUSED", 35, 65, RGBToWord(255, 255, 0), 0);
    printText("> Resume", 10, 82, RGBToWord(255, 255, 255), 0);
    printText("  Main Menu", 10, 94, RGBToWord(255, 255, 255), 0);

    /* Wait for button release first (debounce the PA11 press that got us
     * here)
     */
    while ((GPIOA->IDR & (1 << 11)) == 0)
      ;
    delay(50);

    while (1) {
      /* Highlight selected option */
      printText(selected == 0 ? "> Resume   " : "  Resume   ", 10, 82,
                RGBToWord(255, 255, 255), 0);
      printText(selected == 1 ? "> Main Menu" : "  Main Menu", 10, 94,
                RGBToWord(255, 255, 255), 0);

      /* Down button - move selection down */
      if ((GPIOA->IDR & (1 << 11)) == 0) {
        selected = !selected; /* toggle between 0 and 1 */
        delay(150);           /* debounce                */
      }

      /* Fire/confirm button - PA8 */
      if ((GPIOA->IDR & (1 << 8)) == 0) {
        delay(50);
        if (selected == 0) {
          /* Resume */
          fillRectangle(0, 60, SCREEN_W, 50, 0);
          *ps = GAMERUNNING;
          return;
        } else {
          /* Quit to main menu */
          stop_sound_effect_ch1();
          stop_sound_effect_ch2();
          *as = MAINMENU;
          *ps = GAMERUNNING; /* reset state for next play session */
          return;
        }
      }
    }
  }
}

void playing(AppState* as) {
  clearDisplay();

  /* Game */
  gs.lives = 3;
  gs.score = 0;

  initGameState();

  delay(300);

  /* Initial draw */
  renderAliens();
  putImage(gs.ship.coords.x, gs.ship.coords.y, SHIP_W, SHIP_H, spaceShip, 1, 0);
  drawLine(0, HUD_LINE_Y, SCREEN_W, HUD_LINE_Y, HUD_LINE_COLOR);

  start_sound_effect_ch2(game_loop_notes, game_loop_times, game_loop_note_count,
                         1);

  PlayingState currentPlayingState = GAMERUNNING;
  /* Game loop */
  while (1) {
    uint32_t now = milliseconds;
    if (now - lastUpdate < FRAME_DELAY)
      continue;
    lastUpdate = now;

    if (currentPlayingState == PAUSE) {
      getPause(&currentPlayingState, as);
      continue; /* skip this frame, resume next tick */
    }

    /* Advance bullet */
    if (gs.bullet.state == BULLET_FIRE) {
      gs.bullet.coords.y -= gs.bullet.speed;
      if (gs.bullet.coords.y < 5) {
        resetPlayerBullet();
        // renderPlayerBullet();
      }
    } else {
      gs.bullet.coords.x = gs.ship.coords.x + BULLET_OFFSET_X;
      gs.bullet.coords.y = gs.ship.coords.y - BULLET_OFFSET_Y;
    }

    AlienGrid* ag = &gs.aliens;

    for (int j = 0; j < ALIEN_COLS; j++) {
      Bullet* b = &ag->ab[j];
      if (b->state == BULLET_FIRE) {
        b->coords.y += b->speed; /* alien bullets go DOWN */
        if (b->coords.y > HUD_LINE_Y - 5)
          resetAlienBullet(j);
      }
      /* READY bullets track the grid in moveAliens/parkAlienBullet */
    }

    handleInput(&currentPlayingState, as); /* buttons-> positions*/

    if (*as == MAINMENU)
      return;

    if (currentPlayingState == PAUSE) {
      getPause(&currentPlayingState, as);
      if (*as == MAINMENU)
        return; /* quit was selected */
      continue;
    }
    updateMainAlienBulletCollision();
    moveMainAlien(&gs.mysteryAlien);
    moveAliens(now);         /* now = time, timer-> alien grid shift */
    updateAlienFire(now);    // adds random delay to alien firing
    updatePlayerCollision(); /* bullet    -> alien grid       */
    /*
    TODO :
    implement alien bullet. | redrawing every alien that has overlapped
    with bullet
    */

    if (updateAlienBulletCollision()) {
      gs.lives -= 1;
      putImage(gs.ship.coords.x, gs.ship.coords.y, ALIEN_W, ALIEN_H, explosion,
               1, 0);
      // delay(200);
      start_sound_effect_ch1(lose_life_notes, lose_life_times,
                             lose_life_note_count, 0);
      delay(350);

      resetGame();

      continue;
    }

    if (checkGameOver()) {
      gs.lives -= 1;
      currentPlayingState = GAMEOVER;
      lightLivesIndicator(gs.lives);
      stop_sound_effect_ch1();
      stop_sound_effect_ch2();
      renderGameOverScreen(&currentPlayingState, as);
      continue;
    }
    renderScene(); /* positions -> screen           */
    lightLivesIndicator(gs.lives);
  }
}

/* --- Input ----------------------------------------------------------------
 */

static void handleInput(PlayingState* ps, AppState* as) {
  char c = 0;

  if (serialCharAvailable()) {
    c = serialGetCharNonBlocking();
  }

  if (c != 0) {
    eputchar(c);
  }

  /* Right – PB4 or D */
  if (((GPIOB->IDR & (1 << 4)) == 0) || (c == 'd') || (c == 'D')) {
    if (gs.ship.coords.x < SHIP_MAX_X)
      gs.ship.coords.x += gs.ship.speed;
  }

  /* Left – PB5 or A */
  if (((GPIOB->IDR & (1 << 5)) == 0) || (c == 'a') || (c == 'A')) {
    if (gs.ship.coords.x > SHIP_MIN_X)
      gs.ship.coords.x -= gs.ship.speed;
  }

  /* Fire – PA8 or SPACE or W */
  if (((GPIOA->IDR & (1 << 8)) == 0) || (c == ' ') || (c == 'w') ||
      (c == 'W')) {
    if (gs.bullet.state == BULLET_READY) {
      gs.bullet.state = BULLET_FIRE;
      start_sound_effect_ch1(shoot_notes, shoot_times, shoot_note_count, 0);
      // delay(50);
    }
  }

  /* Pause - PA11 or D*/
  if ((GPIOA->IDR & (1 << 11)) == 0 || (c == 'S') || (c == 's')) {
    delay(50);
    *ps = PAUSE;
  }

  /* Corner Button ==>  Main Menu*/

  if ((GPIOA->IDR & (1 << 9)) == 0) {
    delay(50);
    *ps = PAUSE;
    stop_sound_effect_ch2();
    stop_sound_effect_ch1();
    *as = MAINMENU;
  }
}

/* --- Alien movement / firing ---------------------------------------------
 */
static int isAlienGridMt(void) {
  AlienGrid* ag = &gs.aliens;
  for (int i = 0; i < ALIEN_ROWS; i++) {
    for (int j = 0; j < ALIEN_COLS; j++) {
      if (ag->status[i][j] == 0) {
        return 0;
      }
    }
  }
  return 1;
}
static uint16_t getLowestAlienY(void) {
  AlienGrid* ag = &gs.aliens;
  for (int i = ALIEN_ROWS - 1; i >= 0; i--) {
    for (int j = 0; j < ALIEN_COLS; j++) {
      if (ag->status[i][j] == 0) {
        // lowest alive alien Y position
        return (uint16_t)(ag->offsetY + ag->basePosY[i] + ALIEN_H);
      }
    }
  }

  return 0;  // no-alien alive.
}
static void moveAliens(uint32_t now) {
  AlienGrid* ag = &gs.aliens;

  if (now - ag->lastMoveTime < gs.alienSpeed)
    return;

  ag->lastMoveTime = now;
  if (isAlienGridMt()) {
    stop_sound_effect_ch2();  // stop music briefly
    start_sound_effect_ch1(game_win_notes_ch1, game_win_times_ch1,
                           game_win_note_count_ch1, 0);
    start_sound_effect_ch2(game_win_notes_ch2, game_win_times_ch2,
                           game_win_note_count_ch2, 0);
    delay(900);  // let win jingle finish (4 x 200ms + buffer)
    start_sound_effect_ch2(game_loop_notes, game_loop_times,
                           game_loop_note_count, 1);
    initAlienGrid();
  }

  ag->oldOffsetX = ag->offsetX;
  ag->oldOffsetY = ag->offsetY;

  int16_t nextX = ag->offsetX + (ALIEN_STEP * ag->dirX);
  ag->earToggle = !ag->earToggle;

  if (nextX < ALIEN_MIN_X || nextX > ALIEN_MAX_X) {
    /* Hit a wall: drop down, reverse direction, no horizontal step */

    // random fire on wall hit,
    // TODO: take this function out and only run in certain interval instead
    // of every wall hit...
    for (int j = 0; j < ALIEN_COLS; j++) {
      if (ag->ab[j].state == BULLET_READY && shouldFire(12))
        ag->ab[j].state = BULLET_FIRE;
    }
    ag->offsetY += ALIEN_DROP;
    ag->dirX *= -1;
  } else {
    ag->offsetX = nextX;
  }

  for (int j = 0; j < ALIEN_COLS; j++) {
    if (ag->ab[j].state == BULLET_READY)
      parkAlienBullet(j);
  }
}
static void updateAlienFire(uint32_t now) {
  AlienGrid* ag = &gs.aliens;
  for (int j = 0; j < ALIEN_COLS; j++) {
    if (ag->ab[j].state != BULLET_READY)
      continue; /* already in flight */

    int hasAlive = 0;
    for (int i = 0; i < ALIEN_ROWS; i++)
      if (ag->status[i][j] == 0) {
        hasAlive = 1;
        break;
      }
    if (!hasAlive)
      continue;

    if (now >= ag->nextFireTime[j]) {
      ag->ab[j].state = BULLET_FIRE;
      ag->nextFireTime[j] = now + randomFireDelay();
    }
  }
}

static void moveMainAlien(MainAlien* ma) {
  uint32_t now = milliseconds;

  /* --- Spawn logic --- */
  if (!ma->active) {
    if (now >= ma->nextSpawnTime) {
      ma->active = 1;

      if (channel1_notes == 0) {  // only play if ch1 is free
        start_sound_effect_ch1(aliens_spawning_notes, aliens_spawning_times,
                               aliens_spawning_note_count, 0);
      }

      /* Random direction */
      if (xorshift32() & 1) {
        ma->dir = 1;
        ma->coords.x = 0;
      } else {
        ma->dir = -1;
        ma->coords.x = SCREEN_W - MAIN_ALIEN_W;
      }

      ma->coords.y = MAIN_ALIEN_Y;
      ma->coords.oldX = ma->coords.x;
      ma->coords.oldY = ma->coords.y;

      ma->lastMoveTime = now;
    }
    return;
  }

  /* --- Movement timing --- */
  if (now - ma->lastMoveTime < MAIN_ALIEN_MOVE_DELAY)
    return;

  ma->lastMoveTime = now;

  /* --- Erase old position --- */
  fillRectangle(ma->coords.oldX, ma->coords.oldY, MAIN_ALIEN_W, MAIN_ALIEN_H,
                0);

  /* --- Move --- */
  ma->coords.oldX = ma->coords.x;
  ma->coords.x += ma->speed * ma->dir;

  /* --- Draw --- */
  putImage(ma->coords.x, ma->coords.y, MAIN_ALIEN_W, MAIN_ALIEN_H, mainAlienSpr,
           1, 0);

  /* --- Check bounds --- */
  if (ma->coords.x <= 0 || ma->coords.x >= (SCREEN_W - MAIN_ALIEN_W)) {
    /* erase before disappearing */
    fillRectangle(ma->coords.x, ma->coords.y, MAIN_ALIEN_W, MAIN_ALIEN_H, 0);

    ma->active = 0;
    ma->nextSpawnTime = now + randomMainAlienDelay();
  }
}

/* --- Collision / bullet state --------------------------------------------
 */
static int checkCollision(uint16_t o1X,
                          uint16_t o1Y,
                          uint16_t o1w,
                          uint16_t o1h,
                          uint16_t o2X,
                          uint16_t o2Y,
                          uint16_t o2w,
                          uint16_t o2h) {
  if (o1X + o1w <= o2X)
    return 0;
  if (o1X >= o2X + o2w)
    return 0;
  if (o1Y + o1h <= o2Y)
    return 0;
  if (o1Y >= o2Y + o2h)
    return 0;
  return 1;
}
/*
Alien bullet reset
*/
static void resetAlienBullet(int col) {
  AlienGrid* ag = &gs.aliens;
  Bullet* b = &ag->ab[col];
  /* Erase both positions */
  fillRectangle(b->coords.x, b->coords.y, BULLET_W, BULLET_H, 0);
  fillRectangle(b->coords.oldX, b->coords.oldY, BULLET_W, BULLET_H, 0);
  parkAlienBullet(col);
}
/*
Player Bullet Reset
*/
static void resetPlayerBullet(void) {
  Bullet* b = &gs.bullet;
  gs.bullet.state = BULLET_READY;

  fillRectangle(b->coords.oldX, b->coords.oldY, BULLET_W, BULLET_H + 5, 0);

  gs.bullet.coords.x = gs.bullet.coords.oldX =
      gs.ship.coords.x + BULLET_OFFSET_X;
  gs.bullet.coords.y = gs.bullet.coords.oldY =
      gs.ship.coords.y - BULLET_OFFSET_Y;
}
static void repairAliensUnderRect(uint16_t rx,
                                  uint16_t ry,
                                  uint16_t rw,
                                  uint16_t rh) {
  AlienGrid* ag = &gs.aliens;
  for (int i = 0; i < ALIEN_ROWS; i++) {
    for (int j = 0; j < ALIEN_COLS; j++) {
      if (ag->status[i][j] != 0)
        continue;
      uint16_t ax = (uint16_t)(ag->offsetX + ag->basePosX[j]);
      uint16_t ay = (uint16_t)(ag->offsetY + ag->basePosY[i]);
      if (checkCollision(ax, ay, ALIEN_W, ALIEN_H, rx, ry, rw, rh)) {
        /* Which sprite to use for this row */
        const uint16_t* spr = (i == 0)   ? redAlien[ag->earToggle]
                              : (i == 1) ? greenAlien[ag->earToggle]
                                         : blueAlienBoth[ag->earToggle];
        putImage(ax, ay, ALIEN_W, ALIEN_H, spr, 1, 0);
      }
    }
  }
}
static int updateAlienBulletCollision(void) {
  AlienGrid* ag = &gs.aliens;
  int hit = 0;
  for (int j = 0; j < ALIEN_COLS; j++) {
    Bullet* b = &ag->ab[j];
    if (b->state != BULLET_FIRE)
      continue;
    if (checkCollision(gs.ship.coords.x, gs.ship.coords.y, SHIP_W, SHIP_H,
                       b->coords.x, b->coords.y, BULLET_W, BULLET_H)) {
      resetAlienBullet(j);
      hit = 1;
    }
  }
  return hit;
}

static void updateMainAlienBulletCollision(void) {
  MainAlien* ma = &gs.mysteryAlien;
  Bullet* b = &gs.bullet;
  if (b->state != BULLET_FIRE)
    return;

  if (checkCollision(ma->coords.x, ma->coords.y, MAIN_ALIEN_W, MAIN_ALIEN_H,
                     b->coords.x, b->coords.y, BULLET_W, BULLET_H)) {
    gs.score += 50;
    fillRectangle(ma->coords.x, ma->coords.y, MAIN_ALIEN_W, MAIN_ALIEN_H, 0);
    putImage(ma->coords.x, ma->coords.y, ALIEN_W, ALIEN_H, explosion, 1, 0);
    start_sound_effect_ch1(explode_notes, explode_times, explode_note_count, 0);
    delay(100);
    fillRectangle(ma->coords.x, ma->coords.y, ALIEN_W, ALIEN_H, 0);
    start_sound_effect_ch1(explode_notes, explode_times, explode_note_count, 0);

    initMainAlien(&gs.mysteryAlien);
  }
}

static void updatePlayerCollision(void) {
  if (gs.bullet.state != BULLET_FIRE)
    return;

  AlienGrid* ag = &gs.aliens;
  for (int i = 0; i < ALIEN_ROWS; i++) {
    for (int j = 0; j < ALIEN_COLS; j++) {
      if (ag->status[i][j] != 0)
        continue; /* already dead */

      /* Absolute screen position = grid origin + relative cell offset */
      uint16_t ax = (uint16_t)(ag->offsetX + ag->basePosX[j]);
      uint16_t ay = (uint16_t)(ag->offsetY + ag->basePosY[i]);

      if (checkCollision(ax, ay, ALIEN_W, ALIEN_H, gs.bullet.coords.x,
                         gs.bullet.coords.y, BULLET_W, BULLET_H)) {
        putImage(ax, ay, ALIEN_W, ALIEN_H, explosion, 1, 0);
        delay(50);
        fillRectangle(ax, ay, ALIEN_W, ALIEN_H, 0);

        ag->status[i][j] = 1;
        start_sound_effect_ch1(explode_notes, explode_times, explode_note_count,
                               0);

        /* Park the alien bullet for this column (shooter is dead) */
        resetAlienBullet(j);
        resetPlayerBullet();

        /* Increase the score..... */

        gs.score += gs.score_inc;
        return; /* one hit per frame */
      }
    }
  }
}
static int checkGameOver(void) {
  uint16_t lowestY = getLowestAlienY();

  if (gs.lives == 0) {
    return 1;
  }

  if (lowestY == 0)
    return 0; /* no aliens left - nothing to check */

  /* Condition 1: aliens reached the HUD line */
  if (lowestY >= HUD_LINE_Y)
    return 1;

  /* Condition 2: any alive alien overlaps the ship */
  AlienGrid* ag = &gs.aliens;
  for (int i = 0; i < ALIEN_ROWS; i++) {
    for (int j = 0; j < ALIEN_COLS; j++) {
      if (ag->status[i][j] != 0)
        continue;
      uint16_t ax = (uint16_t)(ag->offsetX + ag->basePosX[j]);
      uint16_t ay = (uint16_t)(ag->offsetY + ag->basePosY[i]);
      if (checkCollision(ax, ay, ALIEN_W, ALIEN_H, gs.ship.coords.x,
                         gs.ship.coords.y, SHIP_W, SHIP_H))
        return 1;
    }
  }
  for (int j = 0; j < ALIEN_COLS; j++) {
    Bullet* b = &ag->ab[j];
    if (b->state != BULLET_FIRE)
      continue;
    if (checkCollision(gs.ship.coords.x, gs.ship.coords.y, SHIP_W, SHIP_H,
                       b->coords.x, b->coords.y, BULLET_W, BULLET_H))
      return 1;
  }
  return 0;
}

/* --- Rendering ------------------------------------------------------------
 */
/*
 * renderAliens - full grid redraw.
 * Used at startup, level reset, and after every move tick.
 *
 * On a move tick the dirty-strip strategy is used:
 *   1. Erase the vacated edge strip (one fillRectangle call).
 *   2. Blit only alive aliens - dead cells stay black automatically
 *      because the strip erase already cleared them.
 */
static void renderAliens(void) {
  AlienGrid* ag = &gs.aliens;

  /* Nothing changed this frame - skip entirely */
  if (ag->offsetX == ag->oldOffsetX && ag->offsetY == ag->oldOffsetY)
    return;

  /* 1. Erase the full bounding box at the OLD position */
  fillRectangle((uint16_t)ag->oldOffsetX, (uint16_t)ag->oldOffsetY,
                ALIEN_GRID_W, ALIEN_GRID_H, 0);

  /* 2. Blit alive aliens at the NEW position */
  for (int i = 0; i < ALIEN_ROWS; i++) {
    for (int j = 0; j < ALIEN_COLS; j++) {
      if (ag->status[i][j] != 0)
        continue;

      if (i == 0) {
        putImage((uint16_t)(ag->offsetX + ag->basePosX[j]),
                 (uint16_t)(ag->offsetY + ag->basePosY[i]), ALIEN_W, ALIEN_H,
                 redAlien[ag->earToggle], 1, 0);
      }
      if (i == 1) {
        putImage((uint16_t)(ag->offsetX + ag->basePosX[j]),
                 (uint16_t)(ag->offsetY + ag->basePosY[i]), ALIEN_W, ALIEN_H,
                 greenAlien[ag->earToggle], 1, 0);
      }
      if (i >= 2) {
        putImage((uint16_t)(ag->offsetX + ag->basePosX[j]),
                 (uint16_t)(ag->offsetY + ag->basePosY[i]), ALIEN_W, ALIEN_H,
                 blueAlienBoth[ag->earToggle], 1, 0);
      }
    }
  }

  /* 3. Sync so next frame knows nothing changed unless moveAliens runs */
  ag->oldOffsetX = ag->offsetX;
  ag->oldOffsetY = ag->offsetY;
}
/*
Dirty-rect ship: erase only the vacated edge strip
*/
static void renderShip(void) {
  if (gs.ship.coords.oldX == gs.ship.coords.x &&
      gs.ship.coords.oldY == gs.ship.coords.y)
    return;

  int16_t dx = (int16_t)gs.ship.coords.x - gs.ship.coords.oldX;

  if (dx > 0)
    fillRectangle(gs.ship.coords.oldX, gs.ship.coords.oldY, (uint16_t)dx,
                  SHIP_H, 0);
  else if (dx < 0)
    fillRectangle(gs.ship.coords.x + SHIP_W, gs.ship.coords.oldY,
                  (uint16_t)(-dx), SHIP_H, 0);

  gs.ship.coords.oldX = gs.ship.coords.x;
  gs.ship.coords.oldY = gs.ship.coords.y;
  putImage(gs.ship.coords.x, gs.ship.coords.y, SHIP_W, SHIP_H, spaceShip, 1, 0);
}
/*
Erase tail strip only, draw new bullet head
*/
static void renderPlayerBullet(void) {
  if (gs.bullet.state != BULLET_FIRE)
    return;

  Bullet* b = &gs.bullet;
  fillRectangle(b->coords.oldX, b->coords.oldY + BULLET_H, BULLET_W, b->speed,
                0);
  b->coords.oldX = b->coords.x;
  b->coords.oldY = b->coords.y;
  fillRectangle(b->coords.x, b->coords.y, BULLET_W, BULLET_H, BULLET_COLOR);
}
static void renderAlienBullets(void) {
  AlienGrid* ag = &gs.aliens;
  for (int j = 0; j < ALIEN_COLS; j++) {
    Bullet* b = &ag->ab[j];
    if (b->state != BULLET_FIRE)
      continue;

    /* 1. Erase old position */
    fillRectangle(b->coords.oldX, b->coords.oldY, BULLET_W, BULLET_H, 0);

    /* 2. Repair aliens whose pixels the erase may have destroyed */
    repairAliensUnderRect(b->coords.oldX, b->coords.oldY, BULLET_W, BULLET_H);

    /* 3. Draw bullet at new position */
    b->coords.oldX = b->coords.x;
    b->coords.oldY = b->coords.y;
    fillRectangle(b->coords.x, b->coords.y, BULLET_W, BULLET_H,
                  ALIEN_BULLET_COLOR);
  }
}
static void renderBarricade() {
  fillRectangle(20, 115, 20, 10, 51975);
}

static void renderGameOverScreen(PlayingState* ps, AppState* as) {
  getPause(ps, as);
}

static void renderStats(void) {
  printText("S:", 83, HUD_LINE_Y + 5, RGBToWord(128, 0, 128), 0);
  printNumber((uint16_t)gs.score, 95, HUD_LINE_Y + 5, RGBToWord(128, 0, 128),

              0);
  printText("HS:", 2, HUD_LINE_Y + 5, RGBToWord(128, 0, 128), 0);
  printNumber((uint16_t)gs.highScore, 22, HUD_LINE_Y + 5,
              RGBToWord(128, 0, 128), 0);
}
/*
Top-level render dispatcher
*/
static void renderScene(void) {
  renderAliens(); /* no-op internally if grid didn't move this frame */
  renderShip();
  renderPlayerBullet();
  renderAlienBullets();
  renderStats();
}

static void makeBackground(int starCount) {
  srand(time(NULL));

  for (uint16_t i = 0; i < starCount; ++i) {
    uint16_t x = (uint16_t)(rand() % SCREEN_W);
    uint16_t y = (uint16_t)(rand() % SCREEN_H);

    int r = rand() % 3;
    uint16_t colour;
    if (r == 0) {
      colour = STAR_RED;
    } else if (r == 1) {
      colour = STAR_BLUE;
    } else {
      colour = STAR_WHITE;
    }

    putPixel(x, y, colour);
  }
}

/*
Splash screen with loading bar
*/

static void splashScreen() {
  // Clear display as precaution
  clearDisplay();

  // Create the starry background
  makeBackground(30);

  // Create loadedBit variable to make the amount of bit variables there will be
  // (Make it progress smoothly)
  const int loadedBit = 50;
  // Pixels loaded per individual loaded bit
  const uint16_t bitWidth = LBAR_W / loadedBit;
  // delay until three seconds
  const uint16_t delayMs = 3000 / loadedBit;

  // Draw the bar background
  drawRectangle(LBAR_X, LBAR_Y, LBAR_W, LBAR_H, LBAR_BACKGROUND);

  // Animate the filling of the background
  for (int i = 0; i <= loadedBit; i++) {
    int w = i * bitWidth;

    // Draw the filled percentage
    drawRectangle(LBAR_X, LBAR_Y, w, LBAR_H, LBAR_FILL);
    // Delay to make look like loading
    delay(delayMs);
  }
}
