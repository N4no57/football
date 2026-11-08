#include "match.h"

Match match_init(const Club home, const Club away) {
    const Match result = {
        .home = home,
        .away = away,
        .ball = {
            .position = {
                0,
                0.55f,
                0
            },
            .velocity = Vector3Zero(),
            .angular_velocity = Vector3Zero(),
            .rotation = Vector3Zero(),
            .radius = 0.11f,
            .mass = 0.42f,
        },
        .phase = FOOTBALL_PHASE_PRE_KICKOFF,
        .phase_time = 0,
        .match_time = 0,
        .regulation_duration = 2700,
        .overtime_duration = 0,
        .score = {0, 0},
        .kickoff_team = TEAM_HOME,
        .scoring_team = TEAM_NONE,
        .overtime_played = false,
        .camera = {
            .position = {0, 10, -32},
            .target = {0, 0, 0},
            .up = {0, 1, 0},
            .fovy = 45,
            .projection = CAMERA_PERSPECTIVE
        }
    };

    return result;
}

void DrawPitch(Match *match) {

    Ball *ball = &match->ball;

    const Matrix rotation = MatrixRotateXYZ((Vector3){
            ball->rotation.x,
            ball->rotation.y,
            ball->rotation.z
    });

    ball->model.transform = rotation;

    DrawModel(
        ball->model,
        ball->position,
        1.f,
        WHITE
    );

    DrawModel(
        match->pitch,
        Vector3Zero(),
        1.f,
        WHITE
    );

    // DrawCylinder(
    //     Vector3Zero(),
    //     0.41f,
    //     0.41f,
    //     1.8f,
    //     2,
    //     GRAY
    // );
}

void MatchUpdate(Match *match, const float dt) {
    update_ball(&match->ball, dt);
}
