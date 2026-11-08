#include "main_menu.h"
#include "loading_screen.h"
#include "../game/game.h"

#include <raylib.h>
#define RAYGUI_IMPLEMENTATION
#include <raygui.h>
#include <stdlib.h>

#include "match_scene.h"

typedef struct Button {
    Rectangle bounding_box;

    Color button_color;
    Color button_hover_color;
    Color button_press_color;
    Color font_color;

    float font_size;
    float font_spacing;

    bool pressed;
    
    const char *text;

    Font font;
} Button;

typedef struct MainMenuData {
    Vector2 screen_scale;

    Button play;
    Button exit;
} MainMenuData;

void DrawButton(Button *button) {
    GuiSetFont(button->font);
    GuiSetStyle(DEFAULT, TEXT_SIZE, (int)button->font_size);
    GuiSetStyle(DEFAULT, TEXT_SPACING, (int)button->font_spacing);

    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, ColorToInt(button->button_color));
    GuiSetStyle(BUTTON, BASE_COLOR_FOCUSED, ColorToInt(button->button_hover_color));
    GuiSetStyle(BUTTON, BASE_COLOR_PRESSED, ColorToInt(button->button_press_color));

    GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL, ColorToInt(button->font_color));
    GuiSetStyle(BUTTON, TEXT_COLOR_FOCUSED, ColorToInt(button->font_color));
    GuiSetStyle(BUTTON, TEXT_COLOR_PRESSED, ColorToInt(button->font_color));

    button->pressed = GuiButton(
        button->bounding_box,
        button->text
    );
}


static bool menu_generated = false;
static MainMenuData menu_data = {0};

static void main_menu_exit(Scene *scene) {
    const MainMenuData *data = (MainMenuData *)scene->data;

    UnloadFont(data->exit.font);
}

static void main_menu_update(Scene *scene, float dt) {
    (void)dt;

    const MainMenuData *data = (MainMenuData *)scene->data;

    if (data->exit.pressed) {
        main_menu_exit(scene);
        CloseWindow();

        exit(EXIT_SUCCESS);
    }

    if (data->play.pressed) {
        LoadingJob *jobs = malloc(sizeof(LoadingJob) * 2);

        jobs = (LoadingJob[]){
            {
                .type = ASSET_MODEL,
                .path = "assets/match/Ball.glb",
                .async = false
            },
            {
                .type = ASSET_MODEL,
                .path = "assets/match/Pitch.glb",
                .async = false
            }
        };

        LoadingScreenInput *input = malloc(sizeof(LoadingScreenInput));

        *input = (LoadingScreenInput){
            .next_scene = match_scene(&scene->game->settings),
            .next_data = malloc(sizeof(MatchEnterData)),
            .assets = jobs,
            .num_jobs = 2
        };

        GameSetScene(scene->game, loading_screen(&scene->game->settings), input);

        free(input);
    }
}

static void main_menu_draw(Scene *scene) {
    MainMenuData *data = scene->data;

    ClearBackground(WHITE);
    DrawFPS(10, 10);

    DrawButton(&data->exit);
    DrawButton(&data->play);
}

Scene main_menu_scene(Settings *settings) {
    Scene result = {
        .enter = nullptr,
        .exit = main_menu_exit,
        .update = main_menu_update,
        .draw = main_menu_draw,
        .settings = settings
    };

    if (!menu_generated) {
        menu_data.screen_scale = (Vector2) {
            (float)settings->resolution.width,
            (float)settings->resolution.height
        };

        const float font_size = 0.025f * menu_data.screen_scale.x;
        constexpr int font_spacing = 2;

        menu_data.play = (Button){
            .bounding_box = {
                0.35f * menu_data.screen_scale.x,
                0.2f * menu_data.screen_scale.y,
                0.3f * menu_data.screen_scale.x,
                0.1f * menu_data.screen_scale.y
            },
            .font_size = font_size,
            .font_spacing = font_spacing,
            .button_color = BLUE,
            .button_hover_color = DARKBLUE,
            .font_color = BLACK,
            .button_press_color = WHITE,
            .text = "Play match",
        };

        menu_data.exit = (Button){
            .bounding_box = {
                0.05f * menu_data.screen_scale.x,
                0,
                0.9f * menu_data.screen_scale.x,
                0.1f * menu_data.screen_scale.y
            },
            .font_size = font_size,
            .font_spacing = font_spacing,
            .button_color = BLUE,
            .button_hover_color = DARKBLUE,
            .font_color = BLACK,
            .button_press_color = WHITE,
            .text = "Exit Championship Football",
        };

        menu_data.exit.bounding_box.y = 0.9f * menu_data.screen_scale.y - menu_data.exit.bounding_box.height / 2;

        menu_generated = true;
    }

    menu_data.exit.font = menu_data.exit.font = LoadFont("assets/fonts/cruyff-sans/CruyffSans-Bold.ttf");

    result.data = &menu_data;

    return result;
}
