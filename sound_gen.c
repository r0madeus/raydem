#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SAMPLE_RATE 44100
#define PI 3.14159265359

// WAV Header struct
typedef struct {
  char riff_tag[4];
  int32_t riff_length;
  char wave_tag[4];
  char fmt_tag[4];
  int32_t fmt_length;
  int16_t audio_format;
  int16_t num_channels;
  int32_t sample_rate;
  int32_t byte_rate;
  int16_t block_align;
  int16_t bits_per_sample;
  char data_tag[4];
  int32_t data_length;
} WavHeader;

void write_wav(const char *filename, int16_t *buffer, int num_samples) {
  FILE *file = fopen(filename, "wb");
  if (!file) {
    printf("Error opening file %s\n", filename);
    return;
  }

  WavHeader header;
  strncpy(header.riff_tag, "RIFF", 4);
  strncpy(header.wave_tag, "WAVE", 4);
  strncpy(header.fmt_tag, "fmt ", 4);
  strncpy(header.data_tag, "data", 4);

  header.riff_length = 36 + num_samples * 2;
  header.fmt_length = 16;
  header.audio_format = 1; // PCM
  header.num_channels = 1; // Mono
  header.sample_rate = SAMPLE_RATE;
  header.byte_rate = SAMPLE_RATE * 2;
  header.block_align = 2;
  header.bits_per_sample = 16;
  header.data_length = num_samples * 2;

  fwrite(&header, sizeof(WavHeader), 1, file);
  fwrite(buffer, 2, num_samples, file);
  fclose(file);
  printf("Generated %s\n", filename);
}

// Generators

void gen_shoot(const char *filename) {
  int duration_ms = 150;
  int num_samples = (SAMPLE_RATE * duration_ms) / 1000;
  int16_t *buffer = malloc(num_samples * sizeof(int16_t));

  for (int i = 0; i < num_samples; i++) {
    double t = (double)i / SAMPLE_RATE;
    // Frequency sweep from 800Hz down to 200Hz
    double freq = 800.0 - (600.0 * (double)i / num_samples);

    // Triangle wave
    double v = asin(sin(2.0 * PI * freq * t));

    // Envelope (Fast attack, exp decay)
    double env = exp(-8.0 * t);

    buffer[i] = (int16_t)(v * 32000.0 * env);
  }
  write_wav(filename, buffer, num_samples);
  free(buffer);
}

void gen_explosion(const char *filename) {
  int duration_ms = 400;
  int num_samples = (SAMPLE_RATE * duration_ms) / 1000;
  int16_t *buffer = malloc(num_samples * sizeof(int16_t));

  for (int i = 0; i < num_samples; i++) {
    double t = (double)i / SAMPLE_RATE;
    // White noise
    double v = ((double)rand() / RAND_MAX) * 2.0 - 1.0;

    // Lowpass filter approx (Simple moving average would be better but lets
    // just modulate amp)

    // Envelope (Exponential decay)
    double env = exp(-10.0 * t);

    buffer[i] = (int16_t)(v * 32000.0 * env);
  }
  write_wav(filename, buffer, num_samples);
  free(buffer);
}

void gen_hit(const char *filename) {
  int duration_ms = 50;
  int num_samples = (SAMPLE_RATE * duration_ms) / 1000;
  int16_t *buffer = malloc(num_samples * sizeof(int16_t));

  for (int i = 0; i < num_samples; i++) {
    double t = (double)i / SAMPLE_RATE;
    // High pitch noise burst
    double v = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
    if (i % 2 == 0)
      v *= -1; // Makes it harsher

    double env = 1.0 - ((double)i / num_samples);

    buffer[i] = (int16_t)(v * 20000.0 * env);
  }
  write_wav(filename, buffer, num_samples);
  free(buffer);
}

void gen_upgrade(const char *filename) {
  int duration_ms = 400;
  int num_samples = (SAMPLE_RATE * duration_ms) / 1000;
  int16_t *buffer = malloc(num_samples * sizeof(int16_t));

  // Major Triad Arpeggio (C - E - G - C)
  double freqs[] = {523.25, 659.25, 783.99, 1046.50};
  int note_len = num_samples / 4;

  for (int i = 0; i < num_samples; i++) {
    double t = (double)i / SAMPLE_RATE;
    int note_idx = i / note_len;
    if (note_idx > 3)
      note_idx = 3;

    double freq = freqs[note_idx];

    // Square wave (RETRO!)
    double v = sin(2.0 * PI * freq * t) > 0 ? 0.5 : -0.5;

    buffer[i] = (int16_t)(v * 20000.0);
  }
  write_wav(filename, buffer, num_samples);
  free(buffer);
}

int main() {
  // Create resources/sounds directory if not exists (assume calling from
  // project root or handle manual) Actually standard C doesn't have mkdir
  // easily cross platform without headers, but we can assume the directory
  // provided in args or just write to current and move. Let's write to
  // "resources/sounds" and assume the build script creates the dir.

  printf("Generating sounds...\n");
  gen_shoot("resources/sounds/shoot.wav");
  gen_explosion("resources/sounds/explosion.wav");
  gen_hit("resources/sounds/hit.wav");
  gen_upgrade("resources/sounds/upgrade.wav");

  return 0;
}
