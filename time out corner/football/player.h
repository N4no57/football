#ifndef FOOTIE_LIFE_PLAYER_H
#define FOOTIE_LIFE_PLAYER_H

#include "../common.h"

typedef enum Position {
    POSITION_GK,
    POSITION_FB,
    POSITION_WB,
    POSITION_CB,
    POSITION_DM,
    POSITION_CM,
    POSITION_AM,
    POSITION_WINGER,
    // TODO
} Position;

typedef enum TacticalRole {
    TACTICAL_ROLE_NONE,

    TACTICAL_ROLE_GOALKEEPER,

    TACTICAL_ROLE_FULLBACK,
    TACTICAL_ROLE_WINGBACK,
    TACTICAL_ROLE_CENTER_BACK,

    TACTICAL_ROLE_DEFENSIVE_MIDFIELDER,
    TACTICAL_ROLE_CENTRAL_MIDFIELDER,
    TACTICAL_ROLE_ATTACKING_MIDFIELDER,

    TACTICAL_ROLE_WINGER,
    TACTICAL_ROLE_STRIKER
} TacticalRole;

typedef enum SquadRole {
    SQUAD_ROLE_STARTER,
    SQUAD_ROLE_ROTATION,
    SQUAD_ROLE_SQUAD,
    SQUAD_ROLE_RESERVE,
    SQUAD_ROLE_YOUTH
} SquadRole;

typedef enum DevelopmentStage {
    DEVELOPMENT_YOUTH,
    DEVELOPMENT_EARLY,
    DEVELOPMENT_PRIME,
    DEVELOPMENT_DECLINE
} DevelopmentStage;

typedef enum Foot {
    FOOT_LEFT,
    FOOT_RIGHT
} Foot;

typedef enum PlayerActionKind {
    PLAYER_ACTION_NONE,
    PLAYER_ACTION_MOVE,
    PLAYER_ACTION_PASS,
    PLAYER_ACTION_SHOOT,
    PLAYER_ACTION_TACKLE
} PlayerActionKind;

typedef struct Contract {
    Date start;
    Date expiry;

    float wage;
    float release_clause_amount;
} Contract;

typedef struct PlayerIdentity {
    PlayerID id;

    char *first_name;
    char *last_name;

    Date birth_date;
    NationID nationality;

    Position preferred_position;
    unsigned secondary_positions;
} PlayerIdentity;

typedef struct PlayerBody {
    float height;
    float mass;

    float leg_length;
    float stride_length;

    Foot preferred_foot;
    float weak_foot_ability;
} PlayerBody;

typedef struct TechnicalAttributes {
    float first_touch;

    float short_pass;
    float long_pass;
    float crossing;

    float finishing;
    float shot_power;

    float dribbling;
    float ball_control;

    float heading;
    float tackling;

    float weak_foot;
} TechnicalAttributes;

typedef struct PhysicalAttributes {
    float acceleration;
    float top_speed;

    float agility;
    float balance;

    float strength;
    float jumping;

    float stamina;
} PhysicalAttributes;

typedef struct PlayerAttributes {
    TechnicalAttributes technical;
    PhysicalAttributes physical;
} PlayerAttributes;

typedef struct PlayerMentalProfile {
    float awareness;
    float anticipation;

    float decision_speed;
    float decision_quality;

    float composure;
    float concentration;

    float creativity;
    float risk_tolerance;

    float aggression;
    float work_rate;

    float discipline;
} PlayerMentalProfile;

typedef struct TacticalAttributes {
    float positioning;
    float defensive_awareness;
    float attacking_movement;

    float pressing_intelligence;
    float marking;
    float space_interpretation;

    float tactical_discipline;
} TacticalAttributes;

typedef struct PlayerPersonality {
    float ambition;
    float loyalty;
    float professionalism;
    float adaptability;

    float competitiveness;
    float confidence;

    float patience;
} PlayerPersonality;

typedef struct PlayerCareer {
    ClubID current_club;

    Contract contract;

    SquadRole squad_role;

    float reputation_local;
    float reputation_national;
    float reputation_international;

    Vector career_history;
} PlayerCareer;

typedef struct PlayerSeasonRecord {
    SeasonID season;
    ClubID club;

    unsigned appearances;
    unsigned starts;

    unsigned goals;
    unsigned assists;

    unsigned minutes;
} PlayerSeasonRecord;

typedef struct PlayerDevelopment {
    float potential;

    float development_rate;

    float professionalism_modifier;
    float training_response;

    DevelopmentStage stage;
} PlayerDevelopment;

typedef struct MoveAction {
    Vector3 target;
} MoveAction;

typedef struct StrikeAction {
    float power;
    Vector3 strike_point;
    Vector3 strike_vector;
} StrikeAction;

typedef struct ShootAction {
    StrikeAction strike;
} ShootAction;

typedef struct PassAction {
    StrikeAction strike;

    PlayerID target;
} PassAction;

typedef struct TackleAction {
    PlayerID target;
} TackleAction;

typedef struct PlayerAction {
    PlayerActionKind kind;

    union {
        MoveAction move;
        ShootAction shoot;
        PassAction pass;
        TackleAction tackle;
    };
} PlayerAction;

typedef struct PerceivedPlayer {
    PlayerID id;

    Vector3 estimated_position;
    Vector3 estimated_velocity;

    float confidence;
} PerceivedPlayer;

typedef struct PerceivedMatchState {
    Vector ball_position;

    Vector visible_players;
    Vector known_players;

    float information_quality;
} PerceivedMatchState;

typedef struct PlayerMatchState {
    Vector3 position;
    Vector3 velocity;

    float orientation;
    float fatigue;

    PlayerAction current_action;

    PlayerID mark_target;

    TacticalRole role;

    PerceivedMatchState perception;
} PlayerMatchState;

typedef struct Player {
    PlayerIdentity identity;
    PlayerBody body;
    PlayerAttributes attributes;
    PlayerMentalProfile mental;
    PlayerCareer career;
    PlayerDevelopment development;

    PlayerMatchState match_state;
} Player;

#endif //FOOTIE_LIFE_PLAYER_H
