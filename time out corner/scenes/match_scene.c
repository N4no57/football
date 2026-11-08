#include "match_scene.h"
#include "../game/game.h"
#include "../match/match.h"

#include <raylib.h>
#include <stdlib.h>

typedef struct MatchData {
    Match match;
} MatchData;

void match_enter(Scene *scene, void *input) {
    MatchData *data = malloc(sizeof(MatchData));
    const MatchEnterData *input_data = input;

    data->match = match_init(input_data->home, input_data->away);

    // data->match.ball.model = *(Model *)asset_get_id(input_data->assets[0]);
    // data->match.pitch = *(Model *)asset_get_id(input_data->assets[1]);

    scene->data = data;
    TraceLog(LOG_INFO, "finished initializing the match");

    data->match.camera.fovy = 5;

    StrikeAction test = {
        .power = 1,
        .strike_point = {0, -0.45f, 0},
        .strike_vector = {1, 0.f, 0}
    };

    apply_strike(&data->match.ball, &test);
}

void match_exit(Scene *scene) {
    const MatchData *data = scene->data;
    UnloadModel(data->match.ball.model);
    UnloadModel(data->match.pitch);
}

void match_update(Scene *scene, float dt) {
    MatchData *data = scene->data;

    if (dt > 0.5f)
        dt = 0.5f;
    
    MatchUpdate(&data->match, dt);

    data->match.camera.target = data->match.ball.position;
}

static void match_draw(Scene *scene) {
    MatchData *data = scene->data;

    ClearBackground(WHITE);
    DrawFPS(10, 10);
    BeginMode3D(data->match.camera);
    DrawPitch(&data->match);
    EndMode3D();
}

Scene match_scene(Settings *settings) {
    const Scene result = {
        .enter = match_enter,
        .exit = match_exit,
        .update = match_update,
        .draw = match_draw,
        .settings = settings
    };

    return result;
}
