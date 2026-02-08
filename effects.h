#ifndef EFFECTS_H
#define EFFECTS_H

#include "raylib.h"

#define MAX_FLOATING_TEXTS 64

typedef struct {
  Vector2 pos;
  char text[16];
  Color color;
  float life; // 0.0 to 1.0 (or duration in seconds)
  float velocityY;
  bool active;
} FloatingText;

void InitEffects(void);
void StartScreenShake(float intensity, float duration);
void UpdateEffects(void);
Vector2 GetScreenShakeOffset(void);

// Floating Text Functions
void InitFloatingTexts(void);
void SpawnFloatingText(Vector2 pos, const char *text, Color color);
void UpdateFloatingTexts(void);
void DrawFloatingTexts(void);

#endif // EFFECTS_H
