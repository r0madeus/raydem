#ifndef TYPES_H
#define TYPES_H

#include "raylib.h"
#include <stdbool.h>

typedef enum {
  SCREEN_TITLE,
  SCREEN_GAME,
  SCREEN_STORE,
  SCREEN_TALENTS,
  SCREEN_GAMEOVER
} GameScreen;

typedef enum { MODE_NORMAL, MODE_SNIPER, MODE_ROCKET } TowerMode;

typedef struct {
  int totalCredits;
  int selectedColorIndex;
  bool unlockedColors[4];
  int fireRateLevel;
  int multishotLevel;
  int damageLevel;
  int rangeLevel;
  int scavengerLevel;       // +10% Gold per level
  int criticalLevel;        // +5% Crit Chance per level
  int projectileSpeedLevel; // +1.0 Speed per level
  TowerMode activeTowerMode;
} SaveData;

typedef enum { ENEMY_NORMAL, ENEMY_FAST, ENEMY_TANK, ENEMY_BOSS } EnemyType;
typedef enum { PROJ_NORMAL, PROJ_ROCKET } ProjectileType; // New

typedef struct Obstacle {
  Rectangle rect;
  Vector2 velocity; // Replacing separate speedX/speedY with Vector2 for better
                    // handling
  float rotation;
  float rotSpeed;
  Color color;
  bool active;
  int hp;              // Health points
  int maxHp;           // Max Health for UI
  float hitFlashTimer; // For visual feedback
  bool nearMissAwarded;
  EnemyType type;
} Obstacle;

typedef struct {
  Vector2 pos;
  Vector2 velocity;
  bool active;
  float radius;
  Color color;
  ProjectileType type; // New
} Projectile;

typedef struct TrailFrame {
  Vector2 pos;
  float alpha;
} TrailFrame;

#endif // TYPES_H
