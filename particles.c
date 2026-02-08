#include "particles.h"
#include <math.h>
#include <stdlib.h>

static Particle particles[MAX_PARTICLES];

void InitParticles(void) {
  for (int i = 0; i < MAX_PARTICLES; i++) {
    particles[i].active = false;
  }
}

void SpawnParticle(Vector2 position, Vector2 velocity, Color color, float size,
                   float decay, ParticleType type) {
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (!particles[i].active) {
      particles[i].position = position;
      particles[i].velocity = velocity;
      particles[i].color = color;
      particles[i].life = 1.0f;
      particles[i].decay = decay;
      particles[i].size = size;
      particles[i].type = type;
      particles[i].active = true;
      break;
    }
  }
}

void SpawnParticleExplosion(Vector2 position, Color color, int count) {
  // Shockwave at center
  SpawnShockwave(position, color, 30.0f);

  for (int i = 0; i < count; i++) {
    float angle = (float)(rand() % 360) * DEG2RAD;
    float speed = (float)(rand() % 100) / 10.0f + 2.0f;
    Vector2 vel = {cosf(angle) * speed, sinf(angle) * speed};
    float size = (float)(rand() % 5 + 3);
    float decay = (float)(rand() % 5 + 2) / 100.0f; // 0.02 - 0.07

    // Mix shapes
    ParticleType pType = (rand() % 2 == 0) ? PARTICLE_CIRCLE : PARTICLE_SQUARE;
    SpawnParticle(position, vel, color, size, decay, pType);
  }
}

void SpawnTrailParticle(Vector2 position, Color color, float size) {
  float decay = 0.05f;
  Vector2 vel = {(float)(rand() % 10 - 5) / 10.0f,
                 (float)(rand() % 10 - 5) / 10.0f}; // Slight drift
  SpawnParticle(position, vel, color, size, decay, PARTICLE_SQUARE);
}

void SpawnShockwave(Vector2 position, Color color, float size) {
  SpawnParticle(position, (Vector2){0, 0}, color, size, 0.05f, PARTICLE_RING);
}

void UpdateParticles(void) {
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (particles[i].active) {
      particles[i].position.x += particles[i].velocity.x;
      particles[i].position.y += particles[i].velocity.y;
      particles[i].life -= particles[i].decay;

      if (particles[i].life <= 0) {
        particles[i].active = false;
      }
    }
  }
}

void DrawParticles(void) {
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (particles[i].active) {
      Color c = particles[i].color;
      c.a = (unsigned char)(255.0f * particles[i].life);

      if (particles[i].type == PARTICLE_SQUARE) {
        DrawRectangleV(particles[i].position,
                       (Vector2){particles[i].size, particles[i].size}, c);
      } else if (particles[i].type == PARTICLE_CIRCLE) {
        DrawCircleV(particles[i].position, particles[i].size / 2.0f, c);
      } else if (particles[i].type == PARTICLE_RING) {
        float outer =
            particles[i].size * (2.0f - particles[i].life); // Expand as it dies
        float inner = outer - 2.0f;
        if (inner < 0)
          inner = 0;
        DrawRing(particles[i].position, inner, outer, 0, 360, 0, c);
      }
    }
  }
}
