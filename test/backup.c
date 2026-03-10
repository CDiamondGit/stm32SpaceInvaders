#include <stdbool.h>
#include <stdio.h>

// ─────────────────────────────────────────
//  Data Structures
// ─────────────────────────────────────────

typedef struct {
  float x, y;  // top-left position
  float width, height;
  bool active;  // is this bullet still alive?
} Bullet;

typedef struct {
  float x, y;
  float width, height;
  int health;
  bool alive;
} Enemy;

// ─────────────────────────────────────────
//  AABB Collision Detection
//  Returns true if rect A overlaps rect B
// ─────────────────────────────────────────

bool check_collision(float ax, float ay, float aw, float ah, float bx, float by,
                     float bw, float bh) {
  // No collision if one rectangle is outside the other on any axis
  if (ax + aw <= bx) return false;  // A is left  of B
  if (ax >= bx + bw) return false;  // A is right of B
  if (ay + ah <= by) return false;  // A is above  B
  if (ay >= by + bh) return false;  // A is below  B

  return true;  // overlapping on both axes → collision!
}

// ─────────────────────────────────────────
//  Handle bullet ↔ enemy collision
// ─────────────────────────────────────────

void handle_bullet_enemy_collision(Bullet* bullet, Enemy* enemy, int damage) {
  if (!bullet->active || !enemy->alive) return;

  if (check_collision(bullet->x, bullet->y, bullet->width, bullet->height,
                      enemy->x, enemy->y, enemy->width, enemy->height)) {
    printf("HIT! Bullet struck enemy at (%.0f, %.0f)\n", enemy->x, enemy->y);

    bullet->active = false;  // bullet is consumed
    enemy->health -= damage;

    if (enemy->health <= 0) {
      enemy->alive = false;
      printf("Enemy DESTROYED!\n");
    } else {
      printf("Enemy health remaining: %d\n", enemy->health);
    }
  }
}

// ─────────────────────────────────────────
//  Process all bullets against all enemies
//  (call this every game frame)
// ─────────────────────────────────────────

void update_collisions(Bullet* bullets, int bullet_count, Enemy* enemies,
                       int enemy_count, int damage) {
  for (int i = 0; i < bullet_count; i++) {
    for (int j = 0; j < enemy_count; j++) {
      handle_bullet_enemy_collision(&bullets[i], &enemies[j], damage);
    }
  }
}

// ─────────────────────────────────────────
//  Demo / Test
// ─────────────────────────────────────────

int main(void) {
  // Define some bullets  (x, y, width, height, active)
  Bullet bullets[] = {
      {100, 150, 5, 10, true},  // should HIT  enemy 0
      {300, 300, 5, 10, true},  // should MISS
      {55, 60, 5, 10, true},    // should HIT  enemy 1
  };
  int bullet_count = sizeof(bullets) / sizeof(bullets[0]);

  // Define some enemies  (x, y, width, height, health, alive)
  Enemy enemies[] = {
      {90, 140, 40, 40, 30, true},  // 30 HP enemy
      {50, 50, 50, 50, 10, true},   // 10 HP enemy
  };
  int enemy_count = sizeof(enemies) / sizeof(enemies[0]);

  int bullet_damage = 15;

  printf("=== Collision Detection Test ===\n\n");
  update_collisions(bullets, bullet_count, enemies, enemy_count, bullet_damage);

  // ── Status report ──
  printf("\n--- Final State ---\n");
  for (int i = 0; i < bullet_count; i++)
    printf("Bullet %d: %s\n", i, bullets[i].active ? "active" : "spent");

  for (int j = 0; j < enemy_count; j++)
    printf("Enemy  %d: %s (HP: %d)\n", j, enemies[j].alive ? "alive" : "dead",
           enemies[j].health);

  return 0;
}