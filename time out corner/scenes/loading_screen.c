#include "loading_screen.h"
#include "../game/game.h"

#include <stdlib.h>
#include <time.h>
#include <raylib.h>
#include <rextras.h>

typedef struct SceneData {
    double percentage;
    int step;
    bool finished;

    Scene next_scene;
    void *next_data;

    LoadingJob *jobs;
    size_t job_count;
} SceneData;

static void loading_screen_enter(Scene *scene, void *input) {
    LoadingScreenInput *input_data = input;

    scene->data = malloc(sizeof(SceneData));

    SceneData *data = scene->data;

    data->percentage = 0;
    data->step = 0;
    data->finished = false;

    data->next_scene = input_data->next_scene;
    data->next_data = input_data->next_data;

    data->jobs = input_data->assets;
    data->job_count = input_data->num_jobs;
}

static void loading_screen_exit(Scene *scene) {
    const SceneData *data = scene->data;

    free(data->jobs);
    free(data->next_data);
    free(scene->data);
}

static void loading_screen_update(Scene *scene, float dt) {
    SceneData *data = scene->data;

    data->percentage = (float)data->step / (float)data->job_count;

    if (data->percentage >= 1) {
        data->percentage = 1;
        data->finished = true;
    }

    if (data->finished) {
        GameSetScene(scene->game, data->next_scene, data->next_data);
    }

    LoadingJob *job = &data->jobs[data->step++];

    if (job->async) {
        asset_load_async(job->path, job->type);
    } else {
        asset_load(job->path, job->type);
    }
}

static void loading_screen_draw(Scene *scene) {
    const SceneData *data = scene->data;

    ClearBackground(WHITE);

    const int width = GetScreenWidth();
    const int height = GetScreenHeight();

    const Rectangle box = {
        0.25f * (float)width,
        0.5f * (float)height,
        0.5f * (float)width,
        0.05f * (float)height,
    };

    DrawRectangle(
        (int)box.x,
        (int)box.y,
        (int)(box.width * data->percentage),
        (int)box.height,
        GREEN
    );

    DrawRectangleLinesEx(
        box,
        0.005f * (float)height,
        BLACK
    );
}


Scene loading_screen(Settings *settings) {
    const Scene result = {
        .settings = settings,
        .enter = loading_screen_enter,
        .exit = loading_screen_exit,
        .update = loading_screen_update,
        .draw = loading_screen_draw,
        .data = nullptr,
        .game = nullptr
    };

    return result;
}
