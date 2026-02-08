#include "effects.h"
#include <stdlib.h>

static float shakeDuration = 0.0f;
static float shakeIntensity = 0.0f;

void InitEffects(void) {
  shakeDuration = 0.0f;
  shakeIntensity = 0.0f;
}

void StartScreenShake(float intensity, float duration) {
  shakeIntensity = intensity;
  shakeDuration = duration;
}

void UpdateEffects(void) {
  if (shakeDuration > 0) {
    shakeDuration -= GetFrameTime();
    if (shakeDuration < 0)
      shakeDuration = 0;
  }
}

Vector2 GetScreenShakeOffset(void) {
  if (shakeDuration > 0) {
    float x = ((float)(rand() % 200 - 100) / 100.0f) * shakeIntensity;
    float y = ((float)(rand() % 200 - 100) / 100.0f) * shakeIntensity;
    return (Vector2){x, y};
  }
  return (Vector2){0, 0};
}

// --- FLOATING TEXT SYSTEM ---
static FloatingText floatingTexts[MAX_FLOATING_TEXTS];

void InitFloatingTexts(void) {
  for (int i = 0; i < MAX_FLOATING_TEXTS; i++) {
    floatingTexts[i].active = false;
  }
}

void SpawnFloatingText(Vector2 pos, const char *text, Color color) {
  for (int i = 0; i < MAX_FLOATING_TEXTS; i++) {
    if (!floatingTexts[i].active) {
      floatingTexts[i].active = true;
      floatingTexts[i].pos = pos;
      TextCopy(floatingTexts[i].text, text);
      floatingTexts[i].color = color;
      floatingTexts[i].life = 1.0f;        // 1 second duration
      floatingTexts[i].velocityY = -50.0f; // Move up
      break;
    }
  }
}

void UpdateFloatingTexts(void) {
  float dt = GetFrameTime();
  for (int i = 0; i < MAX_FLOATING_TEXTS; i++) {
    if (floatingTexts[i].active) {
      floatingTexts[i].pos.y += floatingTexts[i].velocityY * dt;
      floatingTexts[i].life -= dt;
      if (floatingTexts[i].life <= 0) {
        floatingTexts[i].active = false;
      }
    }
  }
}

void DrawFloatingTexts(void) {
  for (int i = 0; i < MAX_FLOATING_TEXTS; i++) {
    if (floatingTexts[i].active) {
      float alpha = floatingTexts[i].life; // Fade out
      if (alpha < 0)
        alpha = 0;
      Color c = floatingTexts[i].color;
      c.a = (unsigned char)(255 * alpha);
      DrawText(floatingTexts[i].text, (int)floatingTexts[i].pos.x,
               (int)floatingTexts[i].pos.y, 20, c);
    }
  }
}
