#ifndef FOOTIE_LIFE_COMMON_H
#define FOOTIE_LIFE_COMMON_H

#include <ints.h>
#include <math.h>
#include <vector.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>

typedef size_t CompetitionID;
typedef size_t PlayerID;
typedef size_t NationID;
typedef size_t ClubID;
typedef size_t SeasonID;

typedef struct Date {
    size_t year;
    size_t month;
    size_t day;
} Date;

#endif //FOOTIE_LIFE_COMMON_H
