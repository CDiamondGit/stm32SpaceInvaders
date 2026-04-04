#include "platform.h"
#include "display.h"
#include "sound_engine.h"

volatile uint32_t milliseconds = 0;

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
void initClock(void) {
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
void initSysTick(void) {
  SysTick->LOAD = 48000;
  SysTick->CTRL = 7;
  SysTick->VAL = 10;
  __asm(" cpsie i ");
}
void SysTick_Handler(void) {
  milliseconds++;
  sound_engine_tick_isr();
}
void setupIO(void) {
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
  pinMode(GPIOA, 10, 1);   /* Lives indicator 1 - PA10*/
  pinMode(GPIOA, 1, 1);    /* Lives indicator 1 - PA1*/
  pinMode(GPIOB, 3, 1);    /* Lives indicator 1 - PB3*/
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

void lightLivesIndicator(int lives) {
  // Clear all LEDs
  GPIOA->BSRR = (1 << (1 + 16));
  GPIOA->BSRR = (1 << (10 + 16));
  GPIOB->BSRR = (1 << (3 + 16));

  // Set LEDs
  if (lives >= 1)
    GPIOA->BSRR = (1 << 1);
  if (lives >= 2)
    GPIOB->BSRR = (1 << 3);
  if (lives >= 3)
    GPIOA->BSRR = (1 << 10);
}
/* --- Utility / timing / random -------------------------------------------
 */
void delay(volatile uint32_t ms) {
  uint32_t end = ms + milliseconds;
  while (milliseconds != end)
    __asm(" wfi ");
}