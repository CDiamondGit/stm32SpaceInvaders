/*

/*
 * 1. INCLUDES & DEFINES
 *
 */
#include <sound_effects.h>
#include <stdbool.h>
#include <stm32f031x6.h>
#include "display.h"

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
#define BULLET_COLOR 65287

/* --- Alien grid ----------------------------------------------------------- */
#define ALIEN_W 11
#define ALIEN_H 8
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
#define ALIEN_MOVE_MS \
  600 /* decrease speeds up alien and increase to slow down          */

/* Drop distance when hitting a wall (one alien row + gap) */
#define ALIEN_DROP (ALIEN_H + ALIEN_GAP_Y)

/* --- HUD ------------------------------------------------------------------ */
#define HUD_LINE_Y 144
#define HUD_LINE_COLOR 57351

/*
 * 2. structs & ENUMS
 */
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

/*
 * AlienGrid holds everything about the alien formation.
 *
 * offsetX / offsetY   – current top-left origin of the whole grid
 * oldOffsetX/Y        – origin last frame (used by dirty-strip render)
 * dirX                – +1 moving right, -1 moving left
 * lastMoveTime        – timestamp of last move tick
 * status[][]          – 0 = alive, 1 = dead
 * basePosX/Y[]        – per-column / per-row pixel offsets RELATIVE to origin
 *                        (precomputed once, never change)
 */
typedef struct {
  int16_t offsetX, offsetY;
  int16_t oldOffsetX, oldOffsetY;
  int8_t dirX;
  uint32_t lastMoveTime;
  uint8_t status[ALIEN_ROWS][ALIEN_COLS];
  uint16_t basePosX[ALIEN_COLS]; /* relative X per column */
  uint16_t basePosY[ALIEN_ROWS]; /* relative Y per row    */
} AlienGrid;

typedef struct {
  Ship ship;
  Bullet bullet;
  AlienGrid aliens;
} GameState;

/*
 * 3. GLOBALS & SPRITES
 */
volatile uint32_t milliseconds = 0;
static uint32_t lastUpdate = 0;
static const uint32_t FRAME_DELAY = 16; /* ~60 fps */

static GameState gs;

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

static const uint16_t blueAlien[] = {
    0,     0,     41187, 0,     0,     0,     0,     0,     41187, 0,     0,
    0,     0,     0,     41187, 0,     0,     0,     41187, 0,     0,     0,
    0,     0,     41187, 41187, 41187, 41187, 41187, 41187, 41187, 0,     0,
    0,     41187, 41187, 0,     41187, 41187, 41187, 0,     41187, 41187, 0,
    41187, 41187, 41187, 41187, 41187, 41187, 41187, 41187, 41187, 41187, 41187,
    41187, 0,     41187, 41187, 41187, 41187, 41187, 41187, 41187, 0,     41187,
    41187, 0,     41187, 0,     0,     0,     0,     0,     41187, 0,     41187,
    0,     0,     0,     41187, 41187, 0,     41187, 41187, 0,     0,     0,
};

/*
 * 4. STM32 HARDWARE SETUP
 *
 */

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

void SysTick_Handler(void) {
  milliseconds++;
}

static void setupIO(void) {
  RCC->AHBENR |= (1 << 18) | (1 << 17);
  display_begin();
  pinMode(GPIOB, 4, 0);
  enablePullUp(GPIOB, 4); /* right – PB4  */
  pinMode(GPIOB, 5, 0);
  enablePullUp(GPIOB, 5); /* left  – PB5  */
  pinMode(GPIOA, 8, 0);
  enablePullUp(GPIOA, 8); /* fire  – PA8  */
  pinMode(GPIOA, 11, 0);
  enablePullUp(GPIOA, 11); /* down  – PA11 */
}

void delay(volatile uint32_t ms) {
  uint32_t end = ms + milliseconds;
  while (milliseconds != end)
    __asm(" wfi ");
}

int isInside(uint16_t x1,
             uint16_t y1,
             uint16_t w,
             uint16_t h,
             uint16_t px,
             uint16_t py) {
  return (px >= x1 && px <= x1 + w && py >= y1 && py <= y1 + h) ? 1 : 0;
}

/*
 * 5. GAME INIT
 *
 */

static void initAlienGrid() {
  AlienGrid* ag = &gs.aliens;
  ag->offsetX = ag->oldOffsetX = ALIEN_ORIGIN_X;
  ag->offsetY = ag->oldOffsetY = ALIEN_ORIGIN_Y;
  ag->dirX = -1; /* start moving right */
  ag->lastMoveTime = 0;

  /* Precompute relative per-cell offsets (never change during play) */
  for (int i = 0; i < ALIEN_ROWS; i++)
    ag->basePosY[i] = (uint16_t)(i * (ALIEN_H + ALIEN_GAP_Y));
  for (int j = 0; j < ALIEN_COLS; j++)
    ag->basePosX[j] = (uint16_t)(j * (ALIEN_W + ALIEN_GAP_X));

  /* All aliens alive */
  for (int i = 0; i < ALIEN_ROWS; i++)
    for (int j = 0; j < ALIEN_COLS; j++)
      ag->status[i][j] = 0;
}

static void initGameState(void) {
  /* Ship */
  gs.ship.coords.x = gs.ship.coords.oldX = 58;
  gs.ship.coords.y = gs.ship.coords.oldY = 130;
  gs.ship.speed = SHIP_SPEED;

  /* Bullet – parked at muzzle */
  gs.bullet.coords.x = gs.bullet.coords.oldX =
      gs.ship.coords.x + BULLET_OFFSET_X;
  gs.bullet.coords.y = gs.bullet.coords.oldY =
      gs.ship.coords.y - BULLET_OFFSET_Y;
  gs.bullet.speed = BULLET_SPEED;
  gs.bullet.state = BULLET_READY;

  /* Alien grid */
  initAlienGrid();
}

/*
 * 6. INPUT
 *
 */
static void handleInput(void) {
  /* Right – PB4 */
  if ((GPIOB->IDR & (1 << 4)) == 0) {
    if (gs.ship.coords.x < SHIP_MAX_X)
      gs.ship.coords.x += gs.ship.speed;
  }

  /* Left – PB5 */
  if ((GPIOB->IDR & (1 << 5)) == 0) {
    if (gs.ship.coords.x > SHIP_MIN_X)
      gs.ship.coords.x -= gs.ship.speed;
  }

  /* Fire – PA8 */
  if ((GPIOA->IDR & (1 << 8)) == 0) {
    if (gs.bullet.state == BULLET_READY)
      gs.bullet.state = BULLET_FIRE;
  }
}

/*
 * 7. ALIEN MOVEMENT
 *
 */

/*
 *@isAlienGridMt(void)
 *returns 1 if none of the alien is alive, else 0
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

/*
 *@getLowestAlienY(void) -> return-type: uint16_t
 *returns Y position of lowest alive alien if not then 0 (ie no alien alive)
 */

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

/*
 * moveAliens – called every frame, internally rate-limited to ALIEN_MOVE_MS.
 *
 * The whole grid shifts as one unit.  On a wall hit, the grid drops one row
 * and direction reverses; no horizontal step happens that tick.
 * oldOffset is saved so renderAliens knows which strip to erase.
 */

static void moveAliens(uint32_t now) {
  AlienGrid* ag = &gs.aliens;

  if (now - ag->lastMoveTime < ALIEN_MOVE_MS)
    return;

  ag->lastMoveTime = now;
  if (isAlienGridMt()) {
    initAlienGrid();
  }

  ag->oldOffsetX = ag->offsetX;
  ag->oldOffsetY = ag->offsetY;

  int16_t nextX = ag->offsetX + (ALIEN_STEP * ag->dirX);

  if (nextX < ALIEN_MIN_X || nextX > ALIEN_MAX_X) {
    /* Hit a wall: drop down, reverse direction, no horizontal step */
    ag->offsetY += ALIEN_DROP;
    ag->dirX *= -1;
  } else {
    ag->offsetX = nextX;
  }
}

/*
 * 8. COLLISION
 *
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

static void resetBullet(void) {
  fillRectangle(gs.bullet.coords.oldX, gs.bullet.coords.oldY, BULLET_W,
                BULLET_SPEED, 0);
  gs.bullet.coords.x = gs.bullet.coords.oldX =
      gs.ship.coords.x + BULLET_OFFSET_X;
  gs.bullet.coords.y = gs.bullet.coords.oldY =
      gs.ship.coords.y - BULLET_OFFSET_Y;
  gs.bullet.state = BULLET_READY;
}

static void updateCollision(void) {
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

      printNumber(ay + ALIEN_H, 0, 0, 1, 0);

      if (checkCollision(ax, ay, ALIEN_W, ALIEN_H, gs.bullet.coords.x,
                         gs.bullet.coords.y, BULLET_W, BULLET_H)) {
        fillRectangle(ax, ay, ALIEN_W, ALIEN_H, 0);
        ag->status[i][j] = 1;
        resetBullet();
        return; /* one hit per frame */
      }
    }
  }
}

/*
 *@checkGameOver(void)
 *checks whether game is over or not, if lowest Y of alive alien touches HUD
 * line then returns 1 or If alive alien touches ship returns 1; else 0
 */

static int checkGameOver(void) {
  uint16_t lowestY = getLowestAlienY();

  if (lowestY == 0)
    return 0; /* no aliens left – nothing to check */

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
  return 0;
}
/*
 * 9. RENDER
 *
 */

/*
 * renderAliens – full grid redraw.
 * Used at startup, level reset, and after every move tick.
 *
 * On a move tick the dirty-strip strategy is used:
 *   1. Erase the vacated edge strip (one fillRectangle call).
 *   2. Blit only alive aliens – dead cells stay black automatically
 *      because the strip erase already cleared them.
 */

static void renderAliens(void) {
  AlienGrid* ag = &gs.aliens;

  /* Nothing changed this frame – skip entirely */
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
      putImage((uint16_t)(ag->offsetX + ag->basePosX[j]),
               (uint16_t)(ag->offsetY + ag->basePosY[i]), ALIEN_W, ALIEN_H,
               blueAlien, 1, 0);
    }
  }

  /* 3. Sync so next frame knows nothing changed unless moveAliens runs */
  ag->oldOffsetX = ag->offsetX;
  ag->oldOffsetY = ag->offsetY;
}
/* Dirty-rect ship: erase only the vacated edge strip */
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

/* Erase tail strip only, draw new bullet head */
static void renderBullet(void) {
  if (gs.bullet.state != BULLET_FIRE)
    return;

  Bullet* b = &gs.bullet;
  fillRectangle(b->coords.oldX, b->coords.oldY + BULLET_H, BULLET_W, b->speed,
                0);
  b->coords.oldX = b->coords.x;
  b->coords.oldY = b->coords.y;
  fillRectangle(b->coords.x, b->coords.y, BULLET_W, BULLET_H, BULLET_COLOR);
}

/* Top-level render dispatcher */
static void renderScene(void) {
  renderAliens(); /* no-op internally if grid didn't move this frame */
  renderShip();
  renderBullet();
}

static void resetGame(void) {
  /* Blank the whole play area above the HUD line */
  fillRectangle(0, 0, SCREEN_W, HUD_LINE_Y, 0);

  /* Re-initialise all game state */
  initGameState();

  /* Redraw the initial scene */
  renderAliens();
  putImage(gs.ship.coords.x, gs.ship.coords.y, SHIP_W, SHIP_H, spaceShip, 1, 0);
  /* HUD line stays – was never erased */
}

/*
 * 10. MAIN
 *
 */
int main(void) {
  /* Hardware */
  initClock();
  initSysTick();
  setupIO();
  initSound();

  /* Game */
  initGameState();

  /* Initial draw */
  renderAliens();
  putImage(gs.ship.coords.x, gs.ship.coords.y, SHIP_W, SHIP_H, spaceShip, 1, 0);
  drawLine(0, HUD_LINE_Y, SCREEN_W, HUD_LINE_Y, HUD_LINE_COLOR);

  /* Game loop */
  while (1) {
    // twinkle_twinkle();
    uint32_t now = milliseconds;
    if (now - lastUpdate < FRAME_DELAY)
      continue;
    lastUpdate = now;

    /* Advance bullet */
    if (gs.bullet.state == BULLET_FIRE) {
      gs.bullet.coords.y -= gs.bullet.speed;
      if (gs.bullet.coords.y < 5) {
        resetBullet();
        renderBullet();
      }
    } else {
      gs.bullet.coords.x = gs.ship.coords.x + BULLET_OFFSET_X;
      gs.bullet.coords.y = gs.ship.coords.y - BULLET_OFFSET_Y;
    }

    handleInput();     /* buttons   -> positions        */
    moveAliens(now);   /* timer     -> alien grid shift */
    updateCollision(); /* bullet    -> alien grid       */

    if (checkGameOver()) {
      resetGame();
      continue;
    }
    renderScene(); /* positions -> screen           */
  }

  return 0;
}