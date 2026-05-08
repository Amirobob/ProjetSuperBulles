#ifndef LOGIQUE_H
#define LOGIQUE_H

#include <inclusive.h>

#define MAX_BULLETS   8
#define MAX_BALLS     15
#define MAX_UPGRADES  4
#define BALL_SIZES    3
#define GAME_FPS      60

typedef enum { UPG_SPEED, UPG_HEALTH, UPG_BULLET, UPG_COUNT } UpgradeType;

typedef struct {
    float x, y;
    float vx, vy;
    bool  on_ground;
    int   hp;
    int   max_hp;
    int   speed;
    bool  double_shot;
    int   frame;
    int   frame_timer;
    bool  facing_right;
    bool  shoot_held;
    int   iframes;
    int   drop_timer;   /* >0: falling through red platforms */
    int   shoot_cooldown;
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
    int   hp;
    int   frame;
    int   frame_timer;
    int   phase;
    int   attack_timer;
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
} GameState;

/* entry point called by the menu */
void startgame(BITMAP *buf);

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
void update_bullets(GameState *gs);
void update_balls(GameState *gs);
void update_boss(GameState *gs);
void update_upgrades(GameState *gs);

/* spawn / lifecycle */
void spawn_bullet(GameState *gs, float x, float y);
void spawn_ball(GameState *gs, float x, float y, float vx, float vy, int size);
void split_ball(GameState *gs, int idx);
void spawn_upgrade(GameState *gs, float x, float y);
void apply_upgrade(Player *p, UpgradeType t);

/* collisions + pause */
void check_collisions(GameState *gs);
void pause_menu(BITMAP *buf, GameState *gs);

#endif
