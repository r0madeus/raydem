#ifndef GLOBALS_H
#define GLOBALS_H

#include "types.h"

extern SaveData gameData;
extern Color playerColors[];
extern const char *colorNames[];
extern int colorCosts[];
extern int upgradeCosts[]; // Flat cost curve for now or formula? Let's use
                           // array for simple levels up to 10
// Or better: Base cost + Level * Scale. Let's put in globals.c

#endif // GLOBALS_H
