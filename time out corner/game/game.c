#include "game.h"

#include <raylib.h>
#include <string.h>

void GameInit(Game *game) {
    memset(game, 0, sizeof(Game));

    game->settings.flags =
            FLAG_MSAA_4X_HINT | FLAG_BORDERLESS_WINDOWED_MODE;

    game->settings.target_fps = 0; // unlimited

    game->settings.resolution = (Resolution){1920, 1080};

    game->settings.dynamic_fps = true;

    ClearWindowState((unsigned)-1);
    SetConfigFlags(game->settings.flags);

    TraceLog(LOG_INFO, "Initializing game");

    InitWindow(game->settings.resolution.width, game->settings.resolution.height, "FOOTIE");
    SetTargetFPS(game->settings.target_fps);

    SetExitKey(KEY_NULL);
}

void GameUpdate(Game *game, float dt) {
    game->scene.update(&game->scene, dt);
}

void GameDraw(Game *game) {
    game->scene.draw(&game->scene);
}

void GameShutdown(Game *game) {
    TraceLog(LOG_INFO, "Shutting down game");
    game->settings = (Settings){0};

    game->scene.exit(&game->scene);
    CloseWindow();
}

void GameSetScene(Game *game, Scene scene, void *input_data) {
    if (game->scene.exit) game->scene.exit(&game->scene);

    if (scene.enter) scene.enter(&scene, input_data);
    scene.game = game;

    if (
        scene.draw == nullptr ||
        scene.update == nullptr
    ) TraceLog(LOG_FATAL, "Desired scene is missing core functions");

    game->scene = scene;
}

// TODO for when using a scene stack
void GamePushScene(Game *game, Scene scene);
void GamePopScene(Game *game, Scene scene);