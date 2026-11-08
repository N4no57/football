#ifndef FOOTIE_LIFE_GAME_H
#define FOOTIE_LIFE_GAME_H

#include "settings.h"
#include "scene.h"

typedef struct Game {
    Scene scene;

    // persistent data
    Settings settings;
    // TODO
} Game;

void GameInit(Game *game);
void GameUpdate(Game *game, float dt);
void GameDraw(Game *game);
void GameShutdown(Game *game);

void GameSetScene(Game *game, Scene scene, void *input_data);

#endif //FOOTIE_LIFE_GAME_H
