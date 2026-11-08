#ifndef FOOTIE_LIFE_MATCH_SCENE_H
#define FOOTIE_LIFE_MATCH_SCENE_H

#include "../football/club.h"
#include "../game/scene.h"
#include "../game/settings.h"

typedef struct MatchEnterData {
    // AssetID *assets;

    Club home;
    Club away;
} MatchEnterData;

Scene match_scene(Settings *settings);

#endif //FOOTIE_LIFE_MATCH_SCENE_H
