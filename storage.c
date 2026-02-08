#include "storage.h"
#include "globals.h"
#include <stdio.h>

void SaveGame(void) {
  FILE *file = fopen("save.dat", "wb");
  if (file) {
    fwrite(&gameData, sizeof(SaveData), 1, file);
    fclose(file);
  }
}

void LoadGame(void) {
  FILE *file = fopen("save.dat", "rb");
  if (file) {
    fread(&gameData, sizeof(SaveData), 1, file);
    fclose(file);
  }
}
