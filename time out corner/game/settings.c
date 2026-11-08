#include "settings.h"

#include <raylib.h>

void update_settings(const Settings *settings) {
    SetTargetFPS(settings->target_fps);
}
