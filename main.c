#include "raylib.h"
#include "raymath.h"

#include <stdlib.h>
#include <time.h>

#include "constants.h"
#include "effects.h"
#include "globals.h"
#include "obstacle.h"
#include "particles.h"
#include "storage.h"
#include "types.h"

/**
 * MIS Perspektifi: 'Dynamic Difficulty Scaling' ve 'State Persistence' ilkeleri
 * birleştirilmiştir. Dalga Sistemi (Wave System): Nesneler grup halinde (batch)
 * yönetilir ve sistem durumuna göre (skor) parametreler gerçek zamanlı
 * güncellenir. Kaynakça: Raylib Cheatsheet (2024), Swink, S. (2008) "Game
 * Feel".
 */

int main(void) {
  SetConfigFlags(FLAG_MSAA_4X_HINT);
  InitWindow(KARE_BOYUT, KARE_BOYUT, "Raylib - Wave Physics & Store");
  if (IsFileDropped()) {
    // Dummy check, LoadStorageValue below
  }

  srand(time(NULL));
  LoadGame();

  // Initialize defaults if fresh save
  if (gameData.fireRateLevel < 1)
    gameData.fireRateLevel = 1;
  if (gameData.multishotLevel < 1)
    gameData.multishotLevel = 1;
  if (gameData.damageLevel < 1)
    gameData.damageLevel = 1;
  if (gameData.rangeLevel < 1)
    gameData.rangeLevel = 1;
  InitParticles();
  InitEffects();
  InitFloatingTexts(); // Init Floating Texts

  InitAudioDevice();
  Sound fxShoot = LoadSound("resources/sounds/shoot.wav");
  Sound fxExplosion = LoadSound("resources/sounds/explosion.wav");
  Sound fxHit = LoadSound("resources/sounds/hit.wav");
  Sound fxUpgrade = LoadSound("resources/sounds/upgrade.wav");

  // -- INIT SHADER & RENDER TARGET --
  Shader bloom = LoadShader(0, "resources/shaders/bloom.fs");
  RenderTexture2D target = LoadRenderTexture(KARE_BOYUT, KARE_BOYUT);

  // -- INIT GAME STATE --
  GameScreen currentScreen = SCREEN_TITLE;

  // Combo State
  int comboCount = 0;
  float comboPitch = 1.0f;
  float comboScale = 1.0f; // For text animation

  // Tower & Player State
  Vector2 centerPos = {KARE_BOYUT / 2.0f, KARE_BOYUT / 2.0f};
  float fireTimer = 0.0f;
  float towerScale = 1.0f;        // For Pulse Animation
  float shockwaveCooldown = 0.0f; // Active Skill Cooldown

  // Projectile Pool
  Projectile projectiles[MAX_PROJECTILES] = {0};

  // Wave System State
  int currentTurn = 1;
  int subWavesRemaining = 0; // Will be set at start of game
  float waveTimer = 0.0f;
  bool waveInProgress = false;

  Obstacle engeller[MAX_OBSTACLES];
  for (int i = 0; i < MAX_OBSTACLES; i++)
    engeller[i].active = false;

  double sessionScore = 0;

  SetTargetFPS(60);

  while (!WindowShouldClose()) {
    int activeCount = 0;
    // Dinamik Zorluk Parametreleri
    int currentMaxObstacles = (sessionScore >= 50) ? 4 : 3;
    float speedMultiplier = (sessionScore >= 100) ? 1.05f : 1.0f;

    switch (currentScreen) {
    case SCREEN_TITLE:
      if (IsKeyPressed(KEY_ENTER))
        currentScreen = SCREEN_GAME;
      if (IsKeyPressed(KEY_S))
        currentScreen = SCREEN_STORE;
      if (IsKeyPressed(KEY_T))
        currentScreen = SCREEN_TALENTS;
      break;

    case SCREEN_STORE:
      // Logic moved to Draw phase (immediate mode GUI style) for simplicity or
      // keep here? The Draw phase logic above handles input. So we just need
      // minimal handling here or empty. Actually, my previous replaced chunk
      // for DRAW put the input logic inside drawing. That is valid in immediate
      // mode style, but this is a switch(currentScreen) in update loop? Wait,
      // the main loop has Update Logic then Draw Logic. The chunk I just
      // replaced (236-251) was in the DRAWING switch. I ALSO need to replace
      // the logic in the UPDATE switch (lines 69-85).
      if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_S))
        currentScreen = SCREEN_TITLE;
      break;

    case SCREEN_TALENTS:
      // Logic handled in Draw Phase
      break;

    case SCREEN_GAME:
      // --- WAVE LOGIC ---
      // Start of a new turn
      if (!waveInProgress && activeCount == 0 && subWavesRemaining <= 0) {
        // If all sub-waves are done and no enemies left, start next turn
        // Wait a bit or invalid state? initial state handle
        if (currentTurn == 1 && subWavesRemaining == 0) {
          subWavesRemaining = 3; // First turn 3 sub-waves
        } else {
          currentTurn++;
          subWavesRemaining = currentTurn + 2; // Linear difficulty increase
        }
        waveInProgress = true;
      }

      // Spawning Sub-waves
      if (waveInProgress && subWavesRemaining > 0) {
        waveTimer += GetFrameTime();
        if (waveTimer > 2.0f) { // 2 seconds between sub-waves
          // Spawn a batch
          int spawnCount = (currentTurn / 2) + 1; // Increase density slightly
          if (spawnCount > MAX_OBSTACLES)
            spawnCount = MAX_OBSTACLES; // Cap spawn

          for (int k = 0; k < spawnCount; k++) {
            // Find empty slot
            for (int i = 0; i < MAX_OBSTACLES; i++) {
              if (!engeller[i].active) {
                SpawnObstacleFromSide(&engeller[i], rand() % 4, currentTurn);
                break;
              }
            }
          }

          subWavesRemaining--;
          waveTimer = 0.0f;
        }
      }

      // Check if wave is effectively over (no subwaves left)
      if (subWavesRemaining <= 0) {
        waveInProgress = false;
      }

      // --- TOWER LOGIC (AUTO FIRE) ---
      // --- TOWER LOGIC (AUTO FIRE) ---
      // Calculate Stats based on Level
      float currentFireRate = 0.5f - (gameData.fireRateLevel * 0.04f);
      if (currentFireRate < 0.1f)
        currentFireRate = 0.1f; // Cap max fire rate

      fireTimer += GetFrameTime();

      // Calculate Range: Base 100 + (Level * 20)
      float currentRange = TOWER_RANGE + (gameData.rangeLevel * 20.0f);

      if (fireTimer >= currentFireRate) {
        int targetIndex = -1;
        float minDist = currentRange;

        // Find nearest active enemy
        for (int i = 0; i < MAX_OBSTACLES; i++) {
          if (engeller[i].active) {
            float dist = Vector2Distance(
                centerPos, (Vector2){engeller[i].rect.x, engeller[i].rect.y});
            if (dist < minDist) {
              minDist = dist;
              targetIndex = i;
            }
          }
        }

        if (targetIndex != -1) {
          // Fire Logic with Multishot
          towerScale = 1.2f; // Pulse!
          PlaySound(fxShoot);

          int shotCount =
              1 + (gameData.multishotLevel / 2); // Every 2 levels get +1 shot
          float spreadAngle = 10.0f * (DEG2RAD);

          Vector2 targetPos = {engeller[targetIndex].rect.x +
                                   engeller[targetIndex].rect.width / 2,
                               engeller[targetIndex].rect.y +
                                   engeller[targetIndex].rect.height / 2};
          Vector2 dirToTarget =
              Vector2Normalize(Vector2Subtract(targetPos, centerPos));
          float baseAngle = atan2f(dirToTarget.y, dirToTarget.x);

          float startAngle = baseAngle - (spreadAngle * (shotCount - 1)) / 2.0f;

          for (int s = 0; s < shotCount; s++) {
            // Find free projectile
            for (int j = 0; j < MAX_PROJECTILES; j++) {
              if (!projectiles[j].active) {
                projectiles[j].active = true;
                projectiles[j].pos = centerPos;
                projectiles[j].radius = 4.0f;
                projectiles[j].color = GOLD;

                float currentAngle = startAngle + spreadAngle * s;
                Vector2 shotDir = {cosf(currentAngle), sinf(currentAngle)};
                float speed =
                    PROJECTILE_SPEED + (gameData.projectileSpeedLevel * 1.0f);
                projectiles[j].velocity = Vector2Scale(shotDir, speed);

                fireTimer = 0.0f;
                break;
              }
            }
          }
        }
      }

      // Mouse Click Effect (Visual Feedback)
      if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        // Visual Feedback: Shockwave + Sparks
        Vector2 mPos = GetMousePosition();
        SpawnShockwave(mPos, WHITE, 20.0f);
        SpawnParticleExplosion(mPos, GOLD, 5); // Add sparks

        // Sound Feedback
        PlaySound(fxShoot); // Use shoot sound for feedback
      }

      // --- INTERACTION ---
      // 1. Shockwave [SPACE]
      if (shockwaveCooldown > 0)
        shockwaveCooldown -= GetFrameTime();
      if (IsKeyPressed(KEY_SPACE) && shockwaveCooldown <= 0) {
        shockwaveCooldown = 5.0f; // 5 Seconds Cooldown
        StartScreenShake(5.0f, 0.2f);

        for (int i = 0; i < MAX_OBSTACLES; i++) {
          if (engeller[i].active) {
            // Push back
            Vector2 toEnemy = Vector2Subtract(
                (Vector2){engeller[i].rect.x, engeller[i].rect.y}, centerPos);
            toEnemy = Vector2Normalize(toEnemy);
            engeller[i].velocity = Vector2Add(
                engeller[i].velocity,
                Vector2Scale(toEnemy, 50.0f)); // Strong push (was 5.0)
            engeller[i].hp -= 5;

            if (engeller[i].hp <= 0) {
              engeller[i].active = false;
              sessionScore += 10;
              gameData.totalCredits += 10;
              PlaySound(fxExplosion);
            }
          }
        }
      }

      // 2. Click Damage [LEFT CLICK]
      if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mousePos = GetMousePosition();
        for (int i = 0; i < MAX_OBSTACLES; i++) {
          if (engeller[i].active) {
            // Circle Hitbox for Mouse
            Vector2 enemyCenter = {
                engeller[i].rect.x + engeller[i].rect.width / 2,
                engeller[i].rect.y + engeller[i].rect.height / 2};

            if (CheckCollisionPointCircle(mousePos, enemyCenter,
                                          engeller[i].rect.width / 2.0f)) {
              int clickDmg = 10 + (gameData.damageLevel * 2);
              engeller[i].hp -= clickDmg;
              // Use Center for Text if clicked
              SpawnFloatingText(enemyCenter, TextFormat("%d!", clickDmg),
                                ORANGE);
              PlaySound(fxHit);

              if (engeller[i].hp <= 0) {
                engeller[i].active = false;
                sessionScore += 10;
                gameData.totalCredits += 10;
                PlaySound(fxExplosion);
              }
              break; // Click one at a time
            }
          }
        }
      }

      // 3. Tower Mode Switch [RIGHT CLICK]
      if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
        gameData.activeTowerMode = (gameData.activeTowerMode + 1) % 3;
        SaveGame(); // Persist mode
                    // Show mode text? (Will do in Draw)
      }

      // --- UPDATES ---
      UpdateFloatingTexts();

      // Tower Pulse Decay
      towerScale = Lerp(towerScale, 1.0f, 0.1f);

      // Update Projectiles
      for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (projectiles[i].active) {
          projectiles[i].pos =
              Vector2Add(projectiles[i].pos, projectiles[i].velocity);

          // Bounds check
          if (projectiles[i].pos.x < 0 || projectiles[i].pos.x > KARE_BOYUT ||
              projectiles[i].pos.y < 0 || projectiles[i].pos.y > KARE_BOYUT) {
            projectiles[i].active = false;
            // Combo Reset (Miss)
            if (comboCount > 0)
              comboCount = 0;
          }

          // Collision with Enemies
          for (int j = 0; j < MAX_OBSTACLES; j++) {
            if (engeller[j].active) {
              // Circle Collision for better accuracy with rotation
              // Enemy Radius approx width/2. Projectile Radius 4.
              Vector2 enemyCenter = {
                  engeller[j].rect.x + engeller[j].rect.width / 2,
                  engeller[j].rect.y + engeller[j].rect.height / 2};

              if (CheckCollisionCircles(projectiles[i].pos,
                                        projectiles[i].radius, enemyCenter,
                                        engeller[j].rect.width / 2.0f)) {
                // Collision Logic
                if (projectiles[i].type == PROJ_ROCKET) {
                  // Rocket Explosion
                  SpawnParticleExplosion(projectiles[i].pos, ORANGE, 30);
                  PlaySound(fxExplosion);

                  // Area Damage
                  float explosionRadius = 60.0f;
                  DrawCircleLines(projectiles[i].pos.x, projectiles[i].pos.y,
                                  explosionRadius, ORANGE);

                  for (int k = 0; k < MAX_OBSTACLES; k++) {
                    if (engeller[k].active &&
                        CheckCollisionCircleRec(projectiles[i].pos,
                                                explosionRadius,
                                                engeller[k].rect)) {
                      int damage = gameData.damageLevel;

                      // Critical Strike Logic (Rocket)
                      bool isCrit = false;
                      if (GetRandomValue(0, 100) <
                          (gameData.criticalLevel * 5)) {
                        damage *= 2;
                        isCrit = true;
                      }

                      engeller[k].hp -= damage;
                      engeller[k].hitFlashTimer = 0.1f;
                      SpawnFloatingText(
                          (Vector2){engeller[k].rect.x, engeller[k].rect.y},
                          TextFormat("%d", damage), isCrit ? RED : YELLOW);

                      if (engeller[k].hp <= 0) {
                        engeller[k].active = false;

                        // Combo Increment
                        comboCount++;
                        if (comboCount > 50)
                          comboCount = 50;
                        comboScale = 1.5f;

                        int gain = 10 + currentTurn;
                        // Scavenger Logic
                        gain += (int)(gain * (gameData.scavengerLevel * 0.1f));
                        // Combo Multiplier
                        if (comboCount > 5)
                          gain += (int)(gain * (comboCount * 0.1f));

                        sessionScore += gain;
                        gameData.totalCredits += gain;
                      }
                    }
                  }
                } else {
                  // Normal Projectile
                  int damage = gameData.damageLevel;
                  if (gameData.activeTowerMode == MODE_SNIPER)
                    damage *= 5;

                  // Critical Strike Logic (Normal)
                  bool isCrit = false;
                  if (GetRandomValue(0, 100) < (gameData.criticalLevel * 5)) {
                    damage *= 2;
                    isCrit = true;
                  }

                  SpawnFloatingText(
                      engeller[j].rect.x > 0
                          ? (Vector2){engeller[j].rect.x, engeller[j].rect.y}
                          : projectiles[i].pos,
                      TextFormat("%d", damage), isCrit ? RED : WHITE);

                  engeller[j].hp -= damage;
                  engeller[j].hitFlashTimer = 0.1f;

                  if (engeller[j].hp <= 0) {
                    engeller[j].active = false;

                    // Combo Increment
                    comboCount++;
                    if (comboCount > 50)
                      comboCount = 50;
                    comboScale = 1.5f;

                    int gain = 10 + currentTurn;
                    // Scavenger Logic
                    gain += (int)(gain * (gameData.scavengerLevel * 0.1f));
                    // Combo Multiplier
                    if (comboCount > 5)
                      gain += (int)(gain * (comboCount * 0.1f));

                    sessionScore += gain;
                    gameData.totalCredits += gain;
                    SpawnParticleExplosion(
                        (Vector2){engeller[j].rect.x, engeller[j].rect.y},
                        ORANGE, 20);

                    float pitchObj = 1.0f + (comboCount * 0.05f);
                    if (pitchObj > 1.5f)
                      pitchObj = 1.5f;
                    SetSoundPitch(fxExplosion, pitchObj);
                    PlaySound(fxExplosion);
                  }
                }

                projectiles[i].active = false; // Destroy Projectile
                break;
              }
              break;
            }
          }
        }
      }

      // Update Enemies
      for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (!engeller[i].active)
          continue;

        engeller[i].rect.x += engeller[i].velocity.x;
        engeller[i].rect.y += engeller[i].velocity.y;
        engeller[i].rotation += engeller[i].rotSpeed;

        if (engeller[i].hitFlashTimer > 0) {
          engeller[i].hitFlashTimer -= GetFrameTime();
        }

        // Collision with Tower (Center)
        Rectangle towerRect = {centerPos.x - 20, centerPos.y - 20, 40, 40};
        if (CheckCollisionRecs(engeller[i].rect, towerRect)) {
          currentScreen = SCREEN_GAMEOVER;
          gameData.totalCredits += (int)sessionScore;
          SaveGame();
          // StartScreenShake(10.0f, 0.5f);
          SpawnParticleExplosion(centerPos, RED, 100);
          comboCount = 0; // Reset Combo
        }

        // Bounds check (remove if too far? usually they hit center or active
        // is false)
      }
      break;

    case SCREEN_GAMEOVER:
      if (IsKeyPressed(KEY_R)) {
        // Reset Game State
        centerPos = (Vector2){KARE_BOYUT / 2.0f, KARE_BOYUT / 2.0f};
        fireTimer = 0.0f;
        sessionScore = 0;
        currentScreen = SCREEN_GAME;

        // Reset Wave
        currentTurn = 1;
        subWavesRemaining = 3;
        waveTimer = 0.0f;
        waveInProgress = false;

        // Clear Entities
        for (int i = 0; i < MAX_OBSTACLES; i++)
          engeller[i].active = false;
        for (int i = 0; i < MAX_PROJECTILES; i++)
          projectiles[i].active = false;
      }
      if (IsKeyPressed(KEY_M))
        currentScreen = SCREEN_TITLE;
      break;
    }

    // --- Çizim ---
    // 1. Sahneyi Texture'a çiz
    BeginTextureMode(target);
    ClearBackground((Color){40, 42, 54, 255});

    Camera2D camera = {0};
    camera.zoom = 1.0f;
    camera.offset = (Vector2){0, 0};

    BeginMode2D(camera);

    switch (currentScreen) {
    case SCREEN_TITLE:
      DrawText("RAY-PHYSICS", 140, 150, 50, WHITE);
      DrawText(TextFormat("TOPLAM KREDI: %d", gameData.totalCredits), 180, 220,
               20, GOLD);
      DrawText("BASLA [ENTER] | MARKET [S] | YETENEKLER [T]", 80, 350, 18,
               LIGHTGRAY);
      // Arka planda uçuşan partiküller olsun
      if (rand() % 10 == 0)
        SpawnTrailParticle((Vector2){rand() % KARE_BOYUT, rand() % KARE_BOYUT},
                           SKYBLUE, 2.0f);
      DrawParticles();
      break;
    case SCREEN_STORE:
      DrawText("UPGRADE CENTER", 180, 50, 40, SKYBLUE);
      DrawText(TextFormat("KREDI: %d", gameData.totalCredits), 20, 20, 20,
               GOLD);

      int startY = 150;
      int gapY = 80;

      // 1. FIRE RATE
      DrawText(TextFormat("[1] ATIS HIZI (Lvl %d)", gameData.fireRateLevel),
               100, startY, 20, WHITE);
      if (gameData.fireRateLevel < 10) {
        int cost = upgradeCosts[gameData.fireRateLevel]; // Cost for next level
        DrawText(TextFormat("BEDEL: %d", cost), 400, startY, 20,
                 (gameData.totalCredits >= cost) ? GREEN : RED);
        if (IsKeyPressed(KEY_ONE) && gameData.totalCredits >= cost) {
          gameData.totalCredits -= cost;
          gameData.fireRateLevel++;
          SaveGame();
        }
      } else {
        DrawText("MAX", 400, startY, 20, GOLD);
      }

      // 2. MULTISHOT
      DrawText(TextFormat("[2] COKLU ATIS (Lvl %d)", gameData.multishotLevel),
               100, startY + gapY, 20, WHITE);
      if (gameData.multishotLevel < 10) {
        int cost = upgradeCosts[gameData.multishotLevel];
        DrawText(TextFormat("BEDEL: %d", cost), 400, startY + gapY, 20,
                 (gameData.totalCredits >= cost) ? GREEN : RED);
        if (IsKeyPressed(KEY_TWO) && gameData.totalCredits >= cost) {
          gameData.totalCredits -= cost;
          gameData.multishotLevel++;
          SaveGame();
        }
      } else {
        DrawText("MAX", 400, startY + gapY, 20, GOLD);
      }

      // 3. DAMAGE (or PROJECTILE POWER)
      DrawText(TextFormat("[3] HASAR (Lvl %d)", gameData.damageLevel), 100,
               startY + gapY * 2, 20, WHITE);
      if (gameData.damageLevel < 10) {
        int cost = upgradeCosts[gameData.damageLevel];
        DrawText(TextFormat("BEDEL: %d", cost), 400, startY + gapY * 2, 20,
                 (gameData.totalCredits >= cost) ? GREEN : RED);
        if (IsKeyPressed(KEY_THREE) && gameData.totalCredits >= cost) {
          gameData.totalCredits -= cost;
          gameData.damageLevel++;
          SaveGame();
        }
      } else {
        DrawText("MAX", 400, startY + gapY * 2, 20, GOLD);
      }

      // 4. RANGE
      DrawText(TextFormat("[4] MENZIL (Lvl %d)", gameData.rangeLevel), 100,
               startY + gapY * 3, 20, WHITE);
      if (gameData.rangeLevel < 10) {
        int cost = upgradeCosts[gameData.rangeLevel];
        DrawText(TextFormat("BEDEL: %d", cost), 400, startY + gapY * 3, 20,
                 (gameData.totalCredits >= cost) ? GREEN : RED);
        if (IsKeyPressed(KEY_FOUR) && gameData.totalCredits >= cost) {
          gameData.totalCredits -= cost;
          gameData.rangeLevel++;
          SaveGame();
        }
      } else {
        DrawText("MAX", 400, startY + gapY * 3, 20, GOLD);
      }

      // 5. NUKE (500)
      DrawText("[5] NUKE (500)", 100, startY + gapY * 4, 20, RED);
      if (gameData.totalCredits >= 500) {
        DrawText("BEDEL: 500", 400, startY + gapY * 4, 20, GREEN);
        if (IsKeyPressed(KEY_FIVE)) {
          gameData.totalCredits -= 500;
          for (int i = 0; i < MAX_OBSTACLES; i++) {
            if (engeller[i].active) {
              engeller[i].active = false;
              SpawnParticleExplosion(
                  (Vector2){engeller[i].rect.x, engeller[i].rect.y}, ORANGE,
                  50);
              sessionScore += 10;
            }
          }
          StartScreenShake(10.0f, 0.5f);
          PlaySound(fxExplosion);
          SaveGame();
        }
      } else {
        DrawText("BEDEL: 500", 400, startY + gapY * 4, 20, RED);
      }

      DrawText("[B] GERI DON", 250, 500, 18, GRAY);

      if (IsKeyPressed(KEY_B))
        currentScreen = SCREEN_TITLE;
      break;
    case SCREEN_GAME:
      // Grid
      for (int i = 0; i < KARE_BOYUT; i += 40) {
        DrawLine(i, 0, i, KARE_BOYUT, (Color){68, 71, 90, 80});
        DrawLine(0, i, KARE_BOYUT, i, (Color){68, 71, 90, 80});
      }

      DrawParticles(); // Partikülleri çiz

      // Combo Visuals
      if (comboCount > 0) {
        comboScale = Lerp(comboScale, 1.0f, 0.1f);
        Color comboColor = WHITE;
        if (comboCount > 5)
          comboColor = YELLOW;
        if (comboCount > 10)
          comboColor = ORANGE;
        if (comboCount > 20)
          comboColor = RED;

        DrawText(TextFormat("x%d COMBO!", comboCount), 20, 100,
                 (int)(30 * comboScale), comboColor);

        if (comboCount > 5) {
          float alpha = (sinf(GetTime() * 5.0f) + 1.0f) * 0.5f; // 0.0 to 1.0
          Color borderColor = comboColor;
          borderColor.a = (unsigned char)(alpha * 100);
          DrawRectangleLinesEx((Rectangle){0, 0, KARE_BOYUT, KARE_BOYUT},
                               5 + (comboCount / 5), borderColor);
        }
      }

      // Projectiles
      for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (projectiles[i].active) {
          DrawCircleV(projectiles[i].pos, projectiles[i].radius,
                      projectiles[i].color);
        }
      }

      // Engeller
      for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (engeller[i].active) {
          Vector2 origin = {engeller[i].rect.width / 2,
                            engeller[i].rect.height / 2};

          if (engeller[i].hitFlashTimer > 0) {
            // FLASH WHITE
            if (engeller[i].type == ENEMY_FAST) {
              Vector2 origin = {engeller[i].rect.width / 2,
                                engeller[i].rect.height / 2};
              DrawPoly((Vector2){engeller[i].rect.x + origin.x,
                                 engeller[i].rect.y + origin.y},
                       3, engeller[i].rect.width / 2, engeller[i].rotation,
                       WHITE);
            } else if (engeller[i].type == ENEMY_BOSS) {
              Vector2 origin = {engeller[i].rect.width / 2,
                                engeller[i].rect.height / 2};
              DrawPoly((Vector2){engeller[i].rect.x + origin.x,
                                 engeller[i].rect.y + origin.y},
                       6, engeller[i].rect.width / 2 + 5, engeller[i].rotation,
                       WHITE);
            } else {
              DrawRectanglePro(engeller[i].rect, origin, engeller[i].rotation,
                               WHITE);
            }
          } else {
            // NORMAL DRAW
            if (engeller[i].type == ENEMY_FAST) {
              // Triangle for Fast
              // DrawTriangle(v1, v2, v3, engeller[i].color);
              DrawPoly((Vector2){engeller[i].rect.x + origin.x,
                                 engeller[i].rect.y + origin.y},
                       3, engeller[i].rect.width / 2, engeller[i].rotation,
                       engeller[i].color);
            } else if (engeller[i].type == ENEMY_BOSS) {
              // Hexagon for Boss
              DrawPoly((Vector2){engeller[i].rect.x + origin.x,
                                 engeller[i].rect.y + origin.y},
                       6, engeller[i].rect.width / 2 + 5, engeller[i].rotation,
                       engeller[i].color);
              DrawPolyLines((Vector2){engeller[i].rect.x + origin.x,
                                      engeller[i].rect.y + origin.y},
                            6, engeller[i].rect.width / 2 + 5,
                            engeller[i].rotation, WHITE);
            } else {
              // Rectangle for Normal/Tank
              DrawRectanglePro(engeller[i].rect, origin, engeller[i].rotation,
                               engeller[i].color);
            }
          }

          // Health Bar (Dynamic Color)
          if (engeller[i].hp < engeller[i].maxHp) {
            float healthPct = (float)engeller[i].hp / (float)engeller[i].maxHp;
            Color barColor = GREEN;
            if (healthPct < 0.5f)
              barColor = YELLOW;
            if (healthPct < 0.2f)
              barColor = RED;

            DrawRectangle(engeller[i].rect.x, engeller[i].rect.y - 12,
                          engeller[i].rect.width, 6, BLACK);
            DrawRectangle(engeller[i].rect.x + 1, engeller[i].rect.y - 11,
                          (engeller[i].rect.width - 2) * healthPct, 4,
                          barColor);
          }
        }
      }

      // 2. Shockwave Cooldown
      if (shockwaveCooldown > 0) {
        float cdPct = shockwaveCooldown / 5.0f;
        DrawRectangle(centerPos.x - 20, centerPos.y + 30, 40, 5, GRAY);
        DrawRectangle(centerPos.x - 20, centerPos.y + 30, 40 * cdPct, 5, WHITE);
      } else {
        DrawText("SPACE TO SHOCK", centerPos.x - 60, centerPos.y + 30, 10,
                 WHITE);
      }

      // 3. Tower Mode Indicator
      const char *modeText = "NORMAL";
      Color modeColor = WHITE;
      if (gameData.activeTowerMode == MODE_SNIPER) {
        modeText = "SNIPER";
        modeColor = GREEN;
      } else if (gameData.activeTowerMode == MODE_ROCKET) {
        modeText = "ROCKET";
        modeColor = ORANGE;
      }

      DrawText(TextFormat("MOD: %s [R-CLICK]", modeText), 20, 80, 20,
               modeColor);

      // TOWER (Center)
      DrawCircleV(centerPos, 20 * towerScale,
                  playerColors[gameData.selectedColorIndex]);
      DrawCircleLines(centerPos.x, centerPos.y, 25 * towerScale, WHITE);
      DrawCircleLines(centerPos.x, centerPos.y,
                      TOWER_RANGE + (gameData.rangeLevel * 20.0f),
                      Fade(WHITE, 0.1f)); // Range indicator

      // HUD
      DrawText(TextFormat("SKOR: %d", (int)sessionScore), 20, 20, 20, WHITE);
      DrawText(TextFormat("TUR: %d", currentTurn), 20, 50, 20, SKYBLUE);

      DrawFloatingTexts();
      break;

    case SCREEN_TALENTS:
      ClearBackground(DARKBLUE);
      DrawText("YETENEK AGACI (PRESTIGE)", 150, 50, 30, GOLD);
      DrawText("B: Don", 10, 10, 20, LIGHTGRAY);
      DrawText(TextFormat("KREDI: %d", gameData.totalCredits), 400, 10, 20,
               GOLD);

      int tStartY = 150;
      int tGapY = 100;

      // 1. SCAVENGER
      DrawText(TextFormat("[1] YAGMACI (Lvl %d)", gameData.scavengerLevel), 100,
               tStartY, 25, WHITE);
      DrawText("+%10 Altin Kazanimi", 100, tStartY + 30, 20, LIGHTGRAY);

      if (gameData.scavengerLevel < 5) {
        int cost = 1000 * (gameData.scavengerLevel + 1);
        DrawText(TextFormat("BEDEL: %d", cost), 450, tStartY, 25,
                 (gameData.totalCredits >= cost) ? GREEN : RED);
        if (IsKeyPressed(KEY_ONE) && gameData.totalCredits >= cost) {
          gameData.totalCredits -= cost;
          gameData.scavengerLevel++;
          SaveGame();
          PlaySound(fxUpgrade);
        }
      } else {
        DrawText("MAX", 450, tStartY, 25, GOLD);
      }

      // 2. CRITICAL
      DrawText(TextFormat("[2] KRITIK VURUS (Lvl %d)", gameData.criticalLevel),
               100, tStartY + tGapY, 25, WHITE);
      DrawText("+%5 Kritik Sansi (x2 Hasar)", 100, tStartY + tGapY + 30, 20,
               LIGHTGRAY);

      if (gameData.criticalLevel < 5) {
        int cost = 2000 * (gameData.criticalLevel + 1);
        DrawText(TextFormat("BEDEL: %d", cost), 450, tStartY + tGapY, 25,
                 (gameData.totalCredits >= cost) ? GREEN : RED);
        if (IsKeyPressed(KEY_TWO) && gameData.totalCredits >= cost) {
          gameData.totalCredits -= cost;
          gameData.criticalLevel++;
          SaveGame();
          PlaySound(fxUpgrade);
        }
      } else {
        DrawText("MAX", 450, tStartY + tGapY, 25, GOLD);
      }

      // 3. PROJECTILE SPEED
      DrawText(
          TextFormat("[3] HIZLI MERMI (Lvl %d)", gameData.projectileSpeedLevel),
          100, tStartY + tGapY * 2, 25, WHITE);
      DrawText("+1.0 Mermi Hizi", 100, tStartY + tGapY * 2 + 30, 20, LIGHTGRAY);

      if (gameData.projectileSpeedLevel < 5) {
        int cost = 500 * (gameData.projectileSpeedLevel + 1);
        DrawText(TextFormat("BEDEL: %d", cost), 450, tStartY + tGapY * 2, 25,
                 (gameData.totalCredits >= cost) ? GREEN : RED);
        if (IsKeyPressed(KEY_THREE) && gameData.totalCredits >= cost) {
          gameData.totalCredits -= cost;
          gameData.projectileSpeedLevel++;
          SaveGame();
          PlaySound(fxUpgrade);
        }
      } else {
        DrawText("MAX", 450, tStartY + tGapY * 2, 25, GOLD);
      }

      if (IsKeyPressed(KEY_B))
        currentScreen = SCREEN_TITLE;
      break;

    case SCREEN_GAMEOVER:
      DrawParticles(); // Arkada patlamalar devam etsin
      DrawRectangle(0, 0, KARE_BOYUT, KARE_BOYUT, Fade(BLACK, 0.8f));
      DrawText("SISTEM COKTU!", 150, 200, 40, RED);
      DrawText(TextFormat("KAZANILAN KREDI: %d", (int)sessionScore), 180, 260,
               20, GOLD);
      DrawText("TEKRAR: [R] | MENU: [M]", 180, 350, 18, LIGHTGRAY);
      break;
    }
    EndMode2D();
    EndTextureMode();

    // 2. Texture'ı Ekrana Shader ile çiz
    BeginDrawing();
    ClearBackground(BLACK);
    BeginShaderMode(bloom);
    // Texture dikey olarak ters gelir, düzeltmek lazım (sourceRec height
    // negatif)
    DrawTextureRec(target.texture,
                   (Rectangle){0, 0, (float)target.texture.width,
                               (float)-target.texture.height},
                   (Vector2){0, 0}, WHITE);
    EndShaderMode();
    DrawFPS(KARE_BOYUT - 80, 10);
    EndDrawing();
  } // End while

  UnloadSound(fxShoot);
  UnloadSound(fxExplosion);
  UnloadSound(fxHit);
  UnloadSound(fxUpgrade);
  CloseAudioDevice();

  UnloadShader(bloom);
  UnloadRenderTexture(target);
  CloseWindow();

  return 0;
}
