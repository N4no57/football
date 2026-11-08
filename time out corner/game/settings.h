#ifndef FOOTIE_LIFE_SETTINGS_H
#define FOOTIE_LIFE_SETTINGS_H

typedef struct Resolution {
    int width;
    int height;
} Resolution;

typedef struct Settings {
    unsigned flags;

    Resolution resolution;
    int target_fps;

    bool dynamic_fps;
} Settings;

void update_settings(const Settings *settings);

#endif //FOOTIE_LIFE_SETTINGS_H
