#ifndef FOOTIE_LIFE_COMPETITION_H
#define FOOTIE_LIFE_COMPETITION_H

#include "../common.h"

typedef struct CompetitionRules {
    unsigned squad_size;
    unsigned matchday_squad_size;

    unsigned points_for_win;
    unsigned points_for_draw;
    unsigned points_for_loss;

    unsigned promotion_places;
    unsigned relegation_places;
} CompetitionRules;

typedef struct Competition {
    CompetitionID id;
    CompetitionRules rules;
    Vector clubs;

    CompetitionID promotion_target; // always one promotion target

    // sometimes more than one but it depends on region of the club or some bs
    CompetitionID *relegation_target;
    size_t relegation_count;
} Competition;

#endif //FOOTIE_LIFE_COMPETITION_H
