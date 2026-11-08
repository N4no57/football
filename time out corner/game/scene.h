#ifndef FOOTIE_LIFE_SCENE_H
#define FOOTIE_LIFE_SCENE_H

#include "settings.h"

typedef struct Scene Scene;

typedef void (*SceneEnterFn)(Scene *, void *);
typedef void (*SceneExitFn)(Scene *);
typedef void (*SceneUpdateFn)(Scene *, float dt);
typedef void (*SceneDrawFn)(Scene *);

typedef struct Game Game;

typedef struct Scene {
    Settings *settings;

    SceneEnterFn enter;
    SceneExitFn exit;
    SceneUpdateFn update;
    SceneDrawFn draw;

    void *data;

    Game *game;
} Scene;

#endif //FOOTIE_LIFE_SCENE_H
