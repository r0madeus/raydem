#ifndef PARTICLES_H
#define PARTICLES_H

#include "raylib.h"

#define MAX_PARTICLES 1000

typedef enum { PARTICLE_SQUARE, PARTICLE_CIRCLE, PARTICLE_RING } ParticleType;

typedef struct {
  Vector2 position;
  Vector2 velocity;
  Color color;
  float life;  // 1.0 -> 0.0
  float decay; // amount to subtract from life per frame (e.g., 0.02)
  float size;
  bool active;
  ParticleType type; // New Field
} Particle;

void InitParticles(void);
void UpdateParticles(void);
void DrawParticles(void);
void SpawnParticle(Vector2 position, Vector2 velocity, Color color, float size,
                   float decay, ParticleType type);
void SpawnParticleExplosion(Vector2 position, Color color, int count);
void SpawnTrailParticle(Vector2 position, Color color, float size);
void SpawnShockwave(Vector2 position, Color color, float size);

#endif // PARTICLES_H
