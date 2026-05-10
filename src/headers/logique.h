#ifndef LOGIQUE_H
#define LOGIQUE_H

#include <inclusive.h>

#define MAX_BULLETS    8
#define MAX_BALLS      15
#define MAX_UPGRADES   4
#define BALL_SIZES     3
#define GAME_FPS       60
#define LEVEL_TIME_SEC 100   

/* ---------- Tweakable game tuning (non per-level) ---------- */

/* Player */
#define PLAYER_START_HP             3
#define PLAYER_START_SPEED          3
#define PLAYER_JUMP_IMPULSE         -11.5f
#define PLAYER_JUMP_HOLD            -0.5f   
#define PLAYER_GRAVITY              0.5f
#define PLAYER_MAX_FALL             12.0f
#define PLAYER_DROP_TICKS           15      
#define PLAYER_WALK_FRAME_TICKS     8
#define PLAYER_SHOOT_COOLDOWN       (GAME_FPS / 3)
#define PLAYER_SHOT_SPACING         10  /* horizontal gap between adjacent bullets */
#define PLAYER_MAX_SHOTS            4   /* cap on bullets-per-fire from stacking UPG_BULLET */
#define PLAYER_INVULN_TICKS         GAME_FPS   /* 1s grace after shield absorbs a hit */

/* Bullet */
#define BULLET_SPEED                -7.0f

/* Ball */
#define BALL_GRAVITY                0.15f
#define BALL_FLOOR_POP              -10.0f
#define BALL_SPLIT_VX               2.0f
#define BALL_SPLIT_VY               -4.0f

/* Upgrade */
#define UPGRADE_DROP_CHANCE_PCT     15      
#define UPGRADE_DROP_MIN_LEVEL      2       
#define UPGRADE_FALL_SPEED          1.5f
#define UPGRADE_HALF_SIZE           8
#define UPGRADE_SPEED_BONUS         1

/* Score */
#define SCORE_BALL_SPLIT            25
#define SCORE_BALL_DESTROY          100
#define SCORE_BOSS_KILL             500
#define SCORE_LEVEL_BASE            100     

/* Countdown */
#define COUNTDOWN_STEP_MS           1000

/* Boss generic (per-level HP/start position stay in populate_level) */
#define BOSS_PHASE0_SPEED_BASE      1.5f
#define BOSS_PHASE1_SPEED_BASE      3.0f
#define BOSS_PHASE2_SPEED           18.0f
#define BOSS_PHASE0_SPAWN_SEC       4
#define BOSS_PHASE1_SPAWN_SEC       3
#define BOSS_PHASE2_SPAWN_SEC       2

typedef enum { UPG_SPEED, UPG_HEALTH, UPG_BULLET, UPG_COUNT } UpgradeType;

typedef struct {
    float x, y;
    float vx, vy;
    bool  on_ground;
    int   hp;
    int   max_hp;
    int   speed;
    int   shots_per_fire;   /* 1 by default, +1 per UPG_BULLET picked up */
    int   frame;
    int   frame_timer;
    bool  facing_right;
    int   shoot_cooldown;
    int   drop_timer;   /* >0: falling through red platforms */
    bool  has_second_life;  /* UPG_HEALTH: absorbs one fatal hit, tints player blue */
    int   invuln_timer; /* >0: hit-absorb grace after shield break */
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
} Ball;

typedef struct {
    float x, y;
    float vx;
    float vy;
    int   hp;
    int   hp_max;
    int   frame;
    int   frame_timer;
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
    BITMAP *character[charframes];
    BITMAP *character_blue[charframes];  /* pre-tinted variant shown while shielded */
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
    int      pipe_spawn_timer[3];
    int      level_timer;   /* ticks remaining; / GAME_FPS = seconds left. */
} GameState;

/* entry point called by the menu. seeds the run state from params,
   then loops levels until the player quits or hits the close button. */
void startgame(BITMAP *buf, const char *initial_username, int initial_level, int initial_score);

/* assets */
bool load_assets(GameAssets *a);
void free_assets(GameAssets *a);

/* main game loop: returns 1=won, 0=lost, -1=quit */
int  game_loop(BITMAP *buf, GameAssets *a, GameState *gs);

/* setup */
void init_player(Player *p, GameAssets *a);
void init_level(GameState *gs, GameAssets *a, int level_num);
bool is_level_complete(const GameState *gs);
bool is_player_dead(const GameState *gs);

/* per-tick updates */
void handle_input(GameState *gs);
void update_player(Player *p, GameAssets *a);
void update_bullets(GameState *gs, GameAssets *a);
void update_balls(GameState *gs, GameAssets *a);

/* visual + collision radius of a ball, derived from its size tier.
   size acts as a denominator: 1 = native boule sprite, 2 = half, 3 = third. */
int ball_radius(GameAssets *a, int size);
void update_boss(GameState *gs);
void update_upgrades(GameState *gs, GameAssets *a);

/* spawn / lifecycle */
void spawn_bullet(GameState *gs, float x, float y);
void spawn_ball(GameState *gs, float x, float y, float vx, float vy, int size);
void split_ball(GameState *gs, int idx);
void spawn_from_pipe(GameState *gs, float pipe_x, float pipe_y);  /* ← AJOUTER */
void spawn_upgrade(GameState *gs, float x, float y);
void apply_upgrade(Player *p, UpgradeType t);

/* collisions + pause */
void check_collisions(GameState *gs, GameAssets *a);

/* returns true if the player asked to quit the current game (back to main menu).
   sets exit_flag if they want to quit the whole app. */
bool pause_menu(BITMAP *buf, GameState *gs);







#endif
