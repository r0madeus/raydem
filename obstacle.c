#include "obstacle.h"
#include "constants.h"
#include "raymath.h"
#include <stdlib.h>

void SpawnObstacleFromSide(Obstacle *o, int side, int waveNumber) {
  // Determine Enemy Type
  o->type = ENEMY_NORMAL;

  // Wave Logic
  if (waveNumber % 10 == 0) {
    // Boss Wave
    o->type = ENEMY_BOSS;
  } else if (waveNumber > 10) {
    int r = rand() % 100;
    if (r < 20)
      o->type = ENEMY_TANK;
    else if (r < 40)
      o->type = ENEMY_FAST;
    else
      o->type = ENEMY_NORMAL;
  } else if (waveNumber > 3) {
    if (rand() % 100 < 20)
      o->type = ENEMY_FAST;
  }

  // Difficulty Scaling
  float speedMult = 1.0f + (waveNumber * 0.05f);
  if (speedMult > 3.0f)
    speedMult = 3.0f;

  float baseSpeed = (float)(rand() % 2 + 1) * 1.5f;

  // Type Stat Overrides
  if (o->type == ENEMY_FAST) {
    baseSpeed *= 1.5f;
    o->color = LIME;
  } else if (o->type == ENEMY_TANK) {
    baseSpeed *= 0.6f;
    o->color = PURPLE;
  } else if (o->type == ENEMY_BOSS) {
    baseSpeed *= 0.4f;
    o->color = RED;
  } else {
    Color colors[] = {SKYBLUE, BLUE, DARKBLUE}; // Normals are Blue-ish
    o->color = colors[rand() % 3];
  }

  float finalSpeed = baseSpeed * speedMult;

  float size = 20 + (rand() % 15);
  int baseHP = 1 + (waveNumber / 5);

  if (o->type == ENEMY_BOSS) {
    size = 60.0f; // Huge
    baseHP = baseHP * 15;
  } else if (o->type == ENEMY_TANK) {
    size = 35.0f;
    baseHP = baseHP * 3;
  } else if (o->type == ENEMY_FAST) {
    size = 15.0f;
    baseHP = (int)(baseHP * 0.5f);
    if (baseHP < 1)
      baseHP = 1;
  }

  o->rect = (Rectangle){0, 0, size, size};
  o->rotation = (float)(rand() % 360);
  o->rotSpeed = (float)(rand() % 5 + 2);
  o->active = true;
  o->hp = baseHP;
  o->maxHp = baseHP;
  o->hitFlashTimer = 0.0f;
  o->hitFlashTimer = 0.0f;

  o->nearMissAwarded = false;

  Vector2 center = {KARE_BOYUT / 2.0f, KARE_BOYUT / 2.0f};
  Vector2 spawnPos = {0, 0};

  switch (side) {
  case 0: // Top
    spawnPos.x = (float)(rand() % KARE_BOYUT);
    spawnPos.y = -50;
    break;
  case 1: // Bottom
    spawnPos.x = (float)(rand() % KARE_BOYUT);
    spawnPos.y = KARE_BOYUT + 50;
    break;
  case 2: // Left
    spawnPos.x = -50;
    spawnPos.y = (float)(rand() % KARE_BOYUT);
    break;
  case 3: // Right
    spawnPos.x = KARE_BOYUT + 50;
    spawnPos.y = (float)(rand() % KARE_BOYUT);
    break;
  }

  o->rect.x = spawnPos.x;
  o->rect.y = spawnPos.y;

  // Calculate direction towards center
  Vector2 dir = Vector2Normalize(Vector2Subtract(center, spawnPos));
  o->velocity = Vector2Scale(dir, finalSpeed);
}
