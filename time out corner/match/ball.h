#ifndef FOOTIE_LIFE_BALL_H
#define FOOTIE_LIFE_BALL_H

#include "../common.h"
#include "../football/player.h"

typedef struct Ball {
    Vector3 position;
    Vector3 velocity;
    Vector3 angular_velocity;
    Vector3 rotation;

    float radius;
    float mass;

    Model model;
} Ball;

void apply_strike(Ball *ball, const StrikeAction *strike);
void update_ball(Ball *ball, float dt);

#endif //FOOTIE_LIFE_BALL_H
