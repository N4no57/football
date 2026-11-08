#ifndef FOOTIE_LIFE_LOADING_SCREEN_H
#define FOOTIE_LIFE_LOADING_SCREEN_H

#include "../game/scene.h"
#include "../game/settings.h"
#include "../asset manager/asset_manager.h"

#include <ints.h>

typedef struct LoadingJob {
    AssetType type;
    const char *path;
    bool async;
} LoadingJob;

typedef struct LoadingScreenInput {
    Scene next_scene;
    void *next_data;

    LoadingJob *assets;
    size_t num_jobs;
} LoadingScreenInput;

Scene loading_screen(Settings *settings);

#endif //FOOTIE_LIFE_LOADING_SCREEN_H
