#!/bin/bash

SOURCE_FILE="main.c"
OUTPUT_FILE="game"

echo "Derleniyor: $SOURCE_FILE..."

gcc main.c globals.c storage.c obstacle.c particles.c effects.c -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -o "$OUTPUT_FILE"

if [ $? -eq 0 ]; then
  echo "ÇOK Başarılı bir şekilde demlendi. Servis başlıyor..."
  ./"$OUTPUT_FILE"
else
  echo "Bu kod tam bir pislik çıktı Rıza Baba!!!"
fi
