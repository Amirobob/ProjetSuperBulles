#ifndef LOGIQUE_H
#define LOGIQUE_H

#include <inclusive.h>

#define MAX_BULLETS    8
#define MAX_BALLS      15
#define MAX_UPGRADES   4
#define GAME_FPS       60
#define LEVEL_TIME_SEC 100
#define FINAL_LEVEL    4

#define PLAYER_START_HP             3
#define PLAYER_START_SPEED          3
#define PLAYER_JUMP_IMPULSE         -11.5f
#define PLAYER_JUMP_HOLD            -0.5f
#define PLAYER_GRAVITY              0.5f
#define PLAYER_MAX_FALL             12.0f
#define PLAYER_DROP_TICKS           15
#define PLAYER_WALK_FRAME_TICKS     8
#define PLAYER_SHOOT_COOLDOWN       (GAME_FPS / 3)
#define PLAYER_SHOT_SPACING         10
#define PLAYER_MAX_SHOTS            4
#define PLAYER_INVULN_TICKS         GAME_FPS

#define BULLET_SPEED                -7.0f

#define BALL_GRAVITY                0.15f
#define BALL_FLOOR_POP              -10.0f
#define BALL_SPLIT_VX               2.0f
#define BALL_SPLIT_VY               -4.0f

#define BALL_BEAM_MIN_LEVEL         3
#define BALL_ATTACK_COOLDOWN_TICKS  (5 * GAME_FPS)
#define BALL_WARNING_TICKS          (2 * GAME_FPS)
#define BALL_BEAM_DURATION_TICKS    (GAME_FPS / 3)

#define UPGRADE_DROP_CHANCE_PCT     40
#define UPGRADE_DROP_MIN_LEVEL      2
#define UPGRADE_FALL_SPEED          1.5f
#define UPGRADE_HALF_SIZE           8
#define UPGRADE_SPEED_BONUS         1

#define SCORE_BALL_SPLIT            25
#define SCORE_BALL_DESTROY          100
#define SCORE_BOSS_KILL             500
#define SCORE_LEVEL_BASE            100

#define COUNTDOWN_STEP_MS           1000

#define BOSS_PHASE0_SPEED_BASE      1.5f
#define BOSS_PHASE1_SPEED_BASE      3.0f
#define BOSS_PHASE2_SPEED           18.0f
#define BOSS_PHASE0_SPAWN_SEC       4
#define BOSS_PHASE1_SPAWN_SEC       3
#define BOSS_PHASE2_SPAWN_SEC       2
#define BOSS_HURT_SPEED_BOOST       0.6f
#define BOSS_NUDGE_BIAS_PCT         25

typedef enum { UPG_SPEED, UPG_HEALTH, UPG_BULLET, UPG_COUNT } UpgradeType;
typedef enum { BALL_MOVING = 0, BALL_WARNING, BALL_FIRING } BallAttackState;

typedef struct {
    float x, y;
    float vx, vy;
    bool  on_ground;
    int   hp;
    int   max_hp;
    int   speed;
    int   shots_per_fire;
    int   frame;
    int   frame_timer;
    bool  facing_right;
    int   shoot_cooldown;
    int   drop_timer;
    bool  has_second_life;
    int   invuln_timer;
} Player;

typedef struct {
    float x, y;
    float vy;
    bool  active;
} Bullet;

typedef struct {
    float x, y;
    float vx, vy;
    int   size;
    bool  active;
    BallAttackState attack_state;
    int   attack_timer;
} Ball;

typedef struct {
    float x, y;
    float vx;
    float vy;
    int   hp;
    int   hp_max;
    int   phase;
    int   attack_timer;
    int   spawn_timer;
    bool  violent;
    bool  active;
} Boss;

typedef struct {
    float       x, y;
    UpgradeType type;
    bool        active;
} Upgrade;

typedef struct {
    BITMAP *map;
    BITMAP *mapalpha;
    BITMAP *bullet;
    BITMAP *boule;
    BITMAP *upg_speed;
    BITMAP *upg_health;
    BITMAP *upg_bullet;
    BITMAP *beam;
    BITMAP *exclamation;
    BITMAP *character[charframes];
    BITMAP *character_blue[charframes];
    BITMAP *boss[bossframes];
} GameAssets;

typedef struct {
    Player   player;
    Bullet   bullets[MAX_BULLETS];
    Ball     balls[MAX_BALLS];
    Boss     boss;
    Upgrade  upgrades[MAX_UPGRADES];
    int      active_balls;
    bool     paused;
    int      level_timer;
} GameState;

void startgame(BITMAP *buf, const char *initial_username, int initial_level, int initial_score);
void dim_bitmap(BITMAP *bm, int factor_pct);

bool load_assets(GameAssets *a);
void free_assets(GameAssets *a);

int  game_loop(BITMAP *buf, GameAssets *a, GameState *gs);

void init_player(Player *p, GameAssets *a);
void init_level(GameState *gs, GameAssets *a, int level_num);
bool is_level_complete(const GameState *gs);
bool is_player_dead(const GameState *gs);

void handle_input(GameState *gs);
void update_player(Player *p, GameAssets *a);
void update_bullets(GameState *gs, GameAssets *a);
void update_balls(GameState *gs, GameAssets *a);

int  ball_radius(GameAssets *a, int size);
void update_boss(GameState *gs);
void update_upgrades(GameState *gs, GameAssets *a);

void spawn_bullet(GameState *gs, float x, float y);
void spawn_ball(GameState *gs, float x, float y, float vx, float vy, int size);
void split_ball(GameState *gs, int idx);
void spawn_upgrade(GameState *gs, float x, float y);
void apply_upgrade(Player *p, UpgradeType t);

void check_collisions(GameState *gs, GameAssets *a);

bool pause_menu(BITMAP *buf, GameState *gs);

#endif
