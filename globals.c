#include "globals.h"

SaveData gameData = {
    0, 0,          {true, false, false, false}, 1, 1, 1, 1, 0, 0,
    0, MODE_NORMAL}; // Defaults
Color playerColors[] = {MAROON, GOLD, LIME, SKYBLUE};
const char *colorNames[] = {"STANDART", "ALTIN (500)", "DOGA (1000)",
                            "SIBER (2000)"};
int colorCosts[] = {0, 500, 1000, 2000};
int upgradeCosts[] = {100,  250,   500,   1000,  2500,
                      5000, 10000, 25000, 50000, 100000}; // 10 Levels
