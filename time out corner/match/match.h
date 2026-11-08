#ifndef FOOTIE_LIFE_MATCH_H
#define FOOTIE_LIFE_MATCH_H

#include "../common.h"
#include "ball.h"
#include "../football/club.h"

typedef enum FootballMatchPhase {
    FOOTBALL_PHASE_PRE_KICKOFF,
    FOOTBALL_PHASE_KICKOFF,
    FOOTBALL_PHASE_PLAYING,
    FOOTBALL_PHASE_STOPPED,
    FOOTBALL_PHASE_HALFTIME,
    FOOTBALL_PHASE_GOAL,
    FOOTBALL_PHASE_OVERTIME,
    FOOTBALL_PHASE_WHISTLE,
    FOOTBALL_PHASE_FINISHED
} FootballMatchPhase;

typedef enum Team {
    TEAM_HOME,
    TEAM_AWAY,
    TEAM_NONE,
} Team;

typedef struct Match {
    Club home;
    Club away;

    Ball ball;
    Model pitch;

    FootballMatchPhase phase;

    float phase_time;

    float match_time;
    float regulation_duration;
    float overtime_duration;

    unsigned score[2];

    Team kickoff_team;
    Team scoring_team;

    bool overtime_played;

    Camera3D camera;
} Match;

Match match_init(Club home, Club away);

void match_enter_phase(Match *match, FootballMatchPhase phase);

void DrawPitch(Match *match);
void MatchUpdate(Match *match, float dt);

#endif //FOOTIE_LIFE_MATCH_H
