#include <inclusive.h>
#include "logique.h"
#include "affichage.h"
#include "interface.h"

/* Timer de fps qui ne dépend pas de la vitesse de rendu */
static volatile int tick_counter = 0;
static void tick_increment(void) { tick_counter++; }
END_OF_FUNCTION(tick_increment);


/* Seeds global state (username/level/score) that end_menu and HUD read from. */
void startgame(BITMAP *buf, const char *initial_username, int initial_level, int initial_score) {
    strcpy((char *)username, initial_username);
    level = initial_level;
    score = initial_score;

    GameAssets assets;
    if (!load_assets(&assets)) {
        allegro_message("failed to load game assets");
        free_assets(&assets);
        return;
    }

    while (!exit_flag) {
        GameState gs;
        init_level(&gs, &assets, level);

        int outcome = game_loop(buf, &assets, &gs);
        if (outcome < 0) break;

        end_menu(buf, outcome == 1);
        if (exit_flag) break;
    }

    free_assets(&assets);
}


/* Freezes world to let player scout spawns. */
static void countdown_intro(BITMAP *buf, GameAssets *a, GameState *gs) {
    static const char * const steps[] = {"3", "2", "1", "Go!"};

    for (int i = 0; i < 4 && !exit_flag; i++) {
        draw_game(buf, a, gs);
        textout_centre_ex(buf, font, steps[i],
                          SCREEN_W / 2, SCREEN_H / 2,
                          makecol(255, 255, 80), -1);
        blit(buf, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);

        /* split into 100ms slices so close-button stays responsive. */
        int slices = COUNTDOWN_STEP_MS / 100;
        for (int slice = 0; slice < slices && !exit_flag; slice++) rest(100);
    }
}


/* Returns: 1=won, 0=lost, -1=quit. */
int game_loop(BITMAP *buf, GameAssets *a, GameState *gs) {
    countdown_intro(buf, a, gs);

    LOCK_VARIABLE(tick_counter);
    LOCK_FUNCTION(tick_increment);
    install_int_ex(tick_increment, BPS_TO_TIMER(GAME_FPS));
    tick_counter = 0;

    int outcome = -1;
    while (!exit_flag) {

        /* run one update step per pending tick. if rendering was slow we catch up here. */
        while (tick_counter > 0) {
            handle_input(gs);
            if (!gs->paused) {
                update_player(&gs->player, a);
                update_bullets(gs, a);
                update_balls(gs, a);
                update_boss(gs);
                update_upgrades(gs, a);
                check_collisions(gs, a);
                if (gs->level_timer > 0) gs->level_timer--;
            }
            tick_counter--;
        }

        /* draw once per loop */
        draw_game(buf, a, gs);
        vsync();
        blit(buf, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);

        if (key[KEY_ESC]) {
            if (pause_menu(buf, gs)) { outcome = -1; break; }
        }
        if (is_player_dead(gs))    { outcome = 0; break; }
        if (gs->level_timer <= 0)  { outcome = 0; break; }
        if (is_level_complete(gs)) {
            /* per-level time bonus: faster wins get less, per the spec formula. */
            int seconds_left = gs->level_timer / GAME_FPS;
            if (seconds_left < 0) seconds_left = 0;
            score += SCORE_LEVEL_BASE - seconds_left;
            outcome = 1;
            break;
        }
    }

    remove_int(tick_increment);
    return outcome;
}


/* Build a blue-tinted copy of a sprite for the second-life visual. */
static BITMAP *make_blue_variant(BITMAP *src) {
    BITMAP *dst = create_bitmap(src->w, src->h);
    if (!dst) return NULL;
    int mask = bitmap_mask_color(src);
    for (int y = 0; y < src->h; y++) {
        for (int x = 0; x < src->w; x++) {
            int c = getpixel(src, x, y);
            if (c == mask) { putpixel(dst, x, y, mask); continue; }
            int r = getr(c), g = getg(c), b = getb(c);
            int nr = r / 2;
            int ng = (g * 7) / 10;
            int nb = b + (255 - b) * 6 / 10;
            if (nb > 255) nb = 255;
            putpixel(dst, x, y, makecol(nr, ng, nb));
        }
    }
    return dst;
}

bool load_assets(GameAssets *a) {
    char path[256];
    memset(a, 0, sizeof(*a));

    a->map        = load_bitmap("src/character/map.bmp", NULL);
    a->mapalpha   = load_bitmap("src/character/mapalpha.bmp", NULL);
    a->bullet     = load_bitmap("src/character/bullet.bmp", NULL);
    a->boule      = load_bitmap("src/character/boule.bmp", NULL);
    a->upg_speed  = load_bitmap("src/character/upgrade_speed.bmp", NULL);
    a->upg_health = load_bitmap("src/character/upgrade_health.bmp", NULL);
    a->upg_bullet = load_bitmap("src/character/upgrade_doubleshot.bmp", NULL);
    if (!a->map || !a->mapalpha || !a->bullet || !a->boule
        || !a->upg_speed || !a->upg_health || !a->upg_bullet) return false;

    for (int i = 0; i < charframes; i++) {
        sprintf(path, "src/character/character_%d.bmp", i);
        a->character[i] = load_bitmap(path, NULL);
        if (!a->character[i]) return false;
        a->character_blue[i] = make_blue_variant(a->character[i]);
        if (!a->character_blue[i]) return false;
    }
    for (int i = 0; i < bossframes; i++) {
        sprintf(path, "src/character/boss_%d.bmp", i);
        a->boss[i] = load_bitmap(path, NULL);
        if (!a->boss[i]) return false;
    }
    return true;
}

void free_assets(GameAssets *a) {
    for (int i = 0; i < charframes; i++) {
        destroy_bitmap(a->character[i]);
        destroy_bitmap(a->character_blue[i]);
    }
    for (int i = 0; i < bossframes; i++) destroy_bitmap(a->boss[i]);
    destroy_bitmap(a->map);
    destroy_bitmap(a->mapalpha);
    destroy_bitmap(a->bullet);
    destroy_bitmap(a->boule);
    destroy_bitmap(a->upg_speed);
    destroy_bitmap(a->upg_health);
    destroy_bitmap(a->upg_bullet);
}


/* Player initialization. */
void init_player(Player *p, GameAssets *a) {
    int map_left = SCREEN_W / 2 - a->map->w / 2;
    int map_top  = 0;

    p->x = map_left + 80.0f;        
    p->y = map_top  + a->map->h - 100.0f;  

    p->vx = 0;
    p->vy = 0;
    p->on_ground = false;
    p->hp = p->max_hp = PLAYER_START_HP;
    p->speed = PLAYER_START_SPEED;
    p->shots_per_fire = 1;
    p->frame = 0;
    p->frame_timer = 0;
    p->facing_right = true;
    p->shoot_cooldown = 0;
    p->drop_timer = 0;
    p->has_second_life = false;
    p->invuln_timer = 0;
}


/* Level configuration. Edit each case to set spawns, boss, upgrades. */
static void populate_level(GameState *gs, int level_num) {
    switch (level_num) {
        case 1:
            spawn_ball(gs, SCREEN_W / 2.0f, 170.0f, 2.0f, 0.0f, 1);
            break;

        case 2:
            spawn_ball(gs, SCREEN_W / 2.0f - 200.0f, 170.0f,  2.0f, 0.0f, 2);
            spawn_ball(gs, SCREEN_W / 2.0f + 200.0f, 170.0f, -2.0f, 0.0f, 2);
            break;

        case 3:
            spawn_ball(gs, SCREEN_W / 2.0f,         170.0f,  2.0f, 0.0f, 1);
            spawn_ball(gs, SCREEN_W / 2.0f - 150.0f, 200.0f, -2.0f, 0.0f, 2);
            spawn_ball(gs, SCREEN_W / 2.0f + 150.0f, 200.0f,  2.0f, 0.0f, 3);
            break;

        case 4:
            /* TODO: update_boss() moves and attacks. */
            gs->boss.active = true;
            gs->boss.hp     = 25;
            gs->boss.hp_max = 25;
            gs->boss.x      = SCREEN_W / 2.0f;
            gs->boss.y      = 150.0f;
            gs->boss.vx     = 1.5f;
            gs->boss.phase  = 0;
            gs->boss.violent = false;
            gs->boss.spawn_timer = 2 * GAME_FPS;
            break;
        
        case 10:
            /* TODO: update_boss() moves and attacks. */
            gs->boss.active = true;
            gs->boss.hp     = 45;
            gs->boss.hp_max = 45;
            gs->boss.x      = SCREEN_W / 2.0f;
            gs->boss.y      = 150.0f;
            gs->boss.vx     = 1.5f;
            gs->boss.phase  = 0;
            gs->boss.violent = true;
            gs->boss.spawn_timer = 2 * GAME_FPS;
            break;

        default:
            for (int i = 0; i < (level_num < 8 ? level_num : 8); i++) {
                float dir = (i % 2 == 0) ? 2.0f : -2.0f;
                spawn_ball(gs, SCREEN_W / 2.0f + (i - 2) * 100.0f, 150.0f, dir, 0.0f, 1);
            }
            break;
    }
}


/* Initialize level state and run level-specific spawn logic. */
void init_level(GameState *gs, GameAssets *a, int level_num) {
    init_player(&gs->player, a);

    memset(gs->bullets,  0, sizeof(gs->bullets));
    memset(gs->balls,    0, sizeof(gs->balls));
    memset(gs->upgrades, 0, sizeof(gs->upgrades));
    memset(&gs->boss,    0, sizeof(gs->boss)); 
    gs->boss.active = false;
    gs->active_balls = 0;
    gs->paused = false;

    gs->level_timer = LEVEL_TIME_SEC * GAME_FPS;
    gs->pipe_spawn_timer[0] = 60;
    gs->pipe_spawn_timer[1] = 75;
    gs->pipe_spawn_timer[2] = 90;
    populate_level(gs, level_num);
}


/* Level complete when all balls killed and boss dead. */
bool is_level_complete(const GameState *gs) {
    return gs->active_balls == 0 && !gs->boss.active;
}

bool is_player_dead(const GameState *gs) { return gs->player.hp <= 0; }


/* Read keyboard and update player movement/shooting. */
void handle_input(GameState *gs) {
    Player *p = &gs->player;

    p->vx = 0;
    if (key[KEY_LEFT])  { p->vx = -(float)p->speed; p->facing_right = false; }
    if (key[KEY_RIGHT]) { p->vx =  (float)p->speed; p->facing_right = true;  }

    if (key[KEY_UP]) {
        if (p->on_ground) { p->vy += PLAYER_JUMP_IMPULSE; p->on_ground = false; }
        else if (p->vy > 0) p->vy += PLAYER_JUMP_HOLD;
    }
    if (key[KEY_DOWN] && p->on_ground) {
        p->drop_timer = PLAYER_DROP_TICKS;
        p->on_ground = false;
    }

    if (key[KEY_SPACE] && p->shoot_cooldown == 0) {
        int n = p->shots_per_fire;
        for (int k = 0; k < n; k++) {
            float dx = (k - (n - 1) / 2.0f) * PLAYER_SHOT_SPACING;
            spawn_bullet(gs, p->x + dx, p->y - 10);
        }
        p->shoot_cooldown = PLAYER_SHOOT_COOLDOWN;
    }
}


/* ---------- updates: each runs once per tick ---------- */

/* mapalpha convention: black=wall, white=empty, red=platform (player only). */
static bool is_solid_pixel(GameAssets *a, int sx, int sy, bool platforms_solid) {
    BITMAP *m = a->mapalpha;
    int ox = SCREEN_W / 2 - a->map->w / 2;
    int lx = sx - ox, ly = sy;
    if (lx < 0 || lx >= m->w || ly < 0 || ly >= m->h) return true;

    int depth = bitmap_color_depth(m);
    int c     = getpixel(m, lx, ly);
    if (c == makecol_depth(depth, 0, 0, 0)) return true;
    if (platforms_solid && c == makecol_depth(depth, 255, 0, 0)) return true;
    return false;
}

/* Perimeter collision sampling. */
static bool box_hits(GameAssets *a, int x, int y, int w, int h, bool platforms_solid) {
    const int step  = 4;
    const int x_end = x + w - 1;
    const int y_end = y + h - 1;

    for (int sx = x; sx < x_end; sx += step) {
        if (is_solid_pixel(a, sx, y,     platforms_solid)) return true;
        if (is_solid_pixel(a, sx, y_end, platforms_solid)) return true;
    }
    if (is_solid_pixel(a, x_end, y,     platforms_solid)) return true;
    if (is_solid_pixel(a, x_end, y_end, platforms_solid)) return true;

    for (int sy = y + step; sy < y_end; sy += step) {
        if (is_solid_pixel(a, x,     sy, platforms_solid)) return true;
        if (is_solid_pixel(a, x_end, sy, platforms_solid)) return true;
    }
    return false;
}

void update_player(Player *p, GameAssets *a) {
    BITMAP *spr = a->character[0];
    int hw = spr->w / 2;
    int hh = spr->h / 2;
    bool platforms_solid = (p->drop_timer == 0);

    {
        float new_x = p->x + p->vx;
        int   left  = (int)new_x - hw;
        int   top   = (int)p->y  - hh;
        if (!box_hits(a, left, top, spr->w, spr->h, platforms_solid))
            p->x = new_x;
    }

    p->vy += PLAYER_GRAVITY;
    if (p->vy > PLAYER_MAX_FALL) p->vy = PLAYER_MAX_FALL;

    {
        float new_y = p->y + p->vy;
        int   left  = (int)p->x  - hw;
        int   top   = (int)new_y - hh;
        if (box_hits(a, left, top, spr->w, spr->h, platforms_solid)) {
            if (p->vy > 0) p->on_ground = true;
            p->vy = 0;
        } else {
            p->y = new_y;
            p->on_ground = false;
        }
    }

    if (p->vx != 0 && ++p->frame_timer >= PLAYER_WALK_FRAME_TICKS) {
        p->frame = (p->frame + 1) % charframes;
        p->frame_timer = 0;
    }
    if (p->drop_timer     > 0) p->drop_timer--;
    if (p->shoot_cooldown > 0) p->shoot_cooldown--;
    if (p->invuln_timer   > 0) p->invuln_timer--;
}

void update_bullets(GameState *gs, GameAssets *a) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (gs->bullets[i].active) {
            gs->bullets[i].y += gs->bullets[i].vy;
            if (gs->bullets[i].y < 0) gs->bullets[i].active = false;
            if (is_solid_pixel(a, (int)gs->bullets[i].x, (int)gs->bullets[i].y, false)) gs->bullets[i].active = false;
        }
    }
}

/* Size is denominator: 1=full, 2=half, 3=third of sprite. */
int ball_radius(GameAssets *a, int size) {
    return (a->boule->w / 2) / size;
}

/* 8-point compass sample of ball perimeter. */
static bool ball_hits_map(GameAssets *a, int cx, int cy, int r) {
    static const int dx[8] = { 0,  1, 1, 1, 0, -1, -1, -1 };
    static const int dy[8] = {-1, -1, 0, 1, 1,  1,  0, -1 };
    for (int i = 0; i < 8; i++) {
        if (is_solid_pixel(a, cx + dx[i] * r, cy + dy[i] * r, false)) return true;
    }
    return false;
}

void update_balls(GameState *gs, GameAssets *a) {
    const float gravity   = BALL_GRAVITY;
    const float floor_pop = BALL_FLOOR_POP;

    for (int i = 0; i < MAX_BALLS; i++) {
        Ball *b = &gs->balls[i];
        if (!b->active) continue;

        b->vy += gravity;

        float new_x = b->x + b->vx;
        if (ball_hits_map(a, (int)new_x, (int)b->y, ball_radius(a, b->size))) {
            b->vx = -b->vx;
        } else {
            b->x = new_x;
        }

        float new_y = b->y + b->vy;
        if (ball_hits_map(a, (int)b->x, (int)new_y, ball_radius(a, b->size))) {
            if (b->vy > 0) b->vy = floor_pop;
            else           b->vy = -b->vy;
        } else {
            b->y = new_y;
        }
    }
}

void update_boss(GameState *gs) {
    Boss *b = &gs->boss;
    if (!b->active) return;

    if(b->phase == 0) {
        b->attack_timer--;
        if (b->attack_timer <= 0) {
            b->attack_timer = GAME_FPS + rand() % (2 * GAME_FPS);
            float speed = BOSS_PHASE0_SPEED_BASE + (rand() % 30) / 10.0f;
            b->vx = (rand() % 2 == 0) ? speed : -speed;
        }

        b->x += b->vx;
        if (b->x < 0)        { b->x = 0;        b->vx =  fabsf(b->vx); }
        if (b->x > SCREEN_W) { b->x = SCREEN_W; b->vx = -fabsf(b->vx); }

    } else if(b->phase == 1) {
        b->attack_timer--;
        if (b->attack_timer <= 0) {
            b->attack_timer = GAME_FPS / 2 + rand() % GAME_FPS;
            float speed = BOSS_PHASE1_SPEED_BASE + (rand() % 30) / 10.0f;
            b->vx = (rand() % 2 == 0) ?  speed : -speed;
            b->vy = (rand() % 2 == 0) ?  speed : -speed;
        }

        b->x += b->vx;
        b->y += b->vy;
        if (b->x < 0)        { b->x = 0;        b->vx =  fabsf(b->vx); }
        if (b->x > SCREEN_W) { b->x = SCREEN_W; b->vx = -fabsf(b->vx); }
        if (b->y < 0)        { b->y = 0;        b->vy =  fabsf(b->vy); }
        if (b->y > SCREEN_H) { b->y = SCREEN_H; b->vy = -fabsf(b->vy); }

    } else if (b->phase == 2) {
        b->attack_timer--;
        if (b->attack_timer <= 0) {
            b->attack_timer = (int)(7.5f * GAME_FPS);

            float spd = BOSS_PHASE2_SPEED;
            if (rand() % 2 == 0) {
                b->vx =  spd; b->vy =  spd;
            } else {
                b->vx = -spd; b->vy =  spd;
            }
        }

        b->x += b->vx;
        b->y += b->vy;
        if (b->x < 0)        { b->x = 0;        b->vx =  fabsf(b->vx); }
        if (b->x > SCREEN_W) { b->x = SCREEN_W; b->vx = -fabsf(b->vx); }
        if (b->y < 0)        { b->y = 0;        b->vy =  fabsf(b->vy); }
        if (b->y > SCREEN_H) { b->y = SCREEN_H; b->vy = -fabsf(b->vy); }
    }

    b->spawn_timer--;
    if (b->spawn_timer <= 0) {
        if (b->phase == 0) {
            b->spawn_timer = BOSS_PHASE0_SPAWN_SEC * GAME_FPS;
            float vx = (rand() % 2 == 0) ? 2.0f : -2.0f;
            spawn_ball(gs, b->x, b->y, vx, 0.0f, 2);

        } else if (b->phase == 1) {
            b->spawn_timer = BOSS_PHASE1_SPAWN_SEC * GAME_FPS;
            spawn_ball(gs, b->x, b->y,  2.5f, -1.0f, 2);
            spawn_ball(gs, b->x, b->y, -2.5f, -1.0f, 2);

        } else if (b->phase == 2) {
            b->spawn_timer = BOSS_PHASE2_SPAWN_SEC * GAME_FPS;
            spawn_ball(gs, b->x, b->y,  3.0f,  0.0f, 3);
            spawn_ball(gs, b->x, b->y, -3.0f,  0.0f, 3);
            spawn_ball(gs, b->x, b->y,  0.0f,  3.0f, 3);
            spawn_ball(gs, b->x, b->y,  0.0f, -3.0f, 3);
        }
    }
}

void update_upgrades(GameState *gs, GameAssets *a) {
    for (int i = 0; i < MAX_UPGRADES; i++) {
        Upgrade *u = &gs->upgrades[i];
        if (!u->active) continue;

        float new_y = u->y + UPGRADE_FALL_SPEED;
        if (!box_hits(a, (int)u->x - UPGRADE_HALF_SIZE, (int)new_y - UPGRADE_HALF_SIZE,
                      UPGRADE_HALF_SIZE * 2, UPGRADE_HALF_SIZE * 2, true)) {
            u->y = new_y;
        }
        if (u->y > SCREEN_H) u->active = false;
    }
}



void spawn_bullet(GameState *gs, float x, float y) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (gs->bullets[i].active == false) {
            gs->bullets[i].x = x;
            gs->bullets[i].y = y;
            gs->bullets[i].vy = BULLET_SPEED;  /* shoot upward */
            gs->bullets[i].active = true;
            break;
        }
    }
}

void spawn_ball(GameState *gs, float x, float y, float vx, float vy, int size) {
    for (int i = 0; i < MAX_BALLS; i++) {
        if (!gs->balls[i].active) {
            gs->balls[i].x = x;
            gs->balls[i].y = y;
            gs->balls[i].vx = vx;
            gs->balls[i].vy = vy;
            gs->balls[i].active = true;
            gs->balls[i].size = size;
            gs->active_balls++;
            break;
        }
    }
}

void spawn_from_pipe(GameState *gs, float pipe_x, float pipe_y) {
    float angle = (rand() % 360) * 3.14159f / 180.0f;
    float speed = 2.0f + (rand() % 3);
    float vx = cosf(angle) * speed;
    float vy = sinf(angle) * speed;
    spawn_ball(gs, pipe_x, pipe_y, vx, vy, 1);
}

/* Bullet hit ball: destroy if size==3, else spawn two smaller children. */
void split_ball(GameState *gs, int idx) {
    Ball *b = &gs->balls[idx];
    int parent_size = b->size;

    b->active = false;
    gs->active_balls--;

    /* Size 3 is smallest. */
    if (parent_size < 3) {
        spawn_ball(gs, b->x, b->y,  BALL_SPLIT_VX, BALL_SPLIT_VY, parent_size + 1);
        spawn_ball(gs, b->x, b->y, -BALL_SPLIT_VX, BALL_SPLIT_VY, parent_size + 1);
        score += SCORE_BALL_SPLIT;
    } else {
        score += SCORE_BALL_DESTROY;
        /* Smallest ball fully destroyed: chance to drop an upgrade from level 2 onward. */
        if (level >= UPGRADE_DROP_MIN_LEVEL && rand() % 100 < UPGRADE_DROP_CHANCE_PCT) {
            spawn_upgrade(gs, b->x, b->y);
        }
    }
}

void spawn_upgrade(GameState *gs, float x, float y) {
    for (int i = 0; i < MAX_UPGRADES; i++) {
        if (!gs->upgrades[i].active) {
            gs->upgrades[i].x = x;
            gs->upgrades[i].y = y;
            gs->upgrades[i].type = (UpgradeType)(rand() % UPG_COUNT);
            gs->upgrades[i].active = true;
            break;
        }
    }
}

void apply_upgrade(Player *p, UpgradeType t) {
    switch (t) {
        case UPG_SPEED:  p->speed += UPGRADE_SPEED_BONUS; break;
        case UPG_HEALTH: p->has_second_life = true; break;
        case UPG_BULLET:
            if (p->shots_per_fire < PLAYER_MAX_SHOTS) p->shots_per_fire++;
            break;
        default: break;
    }
}


void check_collisions(GameState *gs, GameAssets *a) {
    const int player_r = a->character[0]->w / 2;
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!gs->bullets[i].active) continue;
        for (int j = 0; j < MAX_BALLS; j++) {
            if (!gs->balls[j].active) continue;
            float dx = gs->bullets[i].x - gs->balls[j].x;
            float dy = gs->bullets[i].y - gs->balls[j].y;
            int   r  = ball_radius(a, gs->balls[j].size);
            if (dx*dx + dy*dy < (float)(r * r)) {
                gs->bullets[i].active = false;
                split_ball(gs, j);
                break;
            }
        }
    }

    if (gs->boss.active) {
        int boss_r = a->boss[0]->w / 2;
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (!gs->bullets[i].active) continue;
            float dx = gs->bullets[i].x - gs->boss.x;
            float dy = gs->bullets[i].y - gs->boss.y;
            if (dx*dx + dy*dy < (float)(boss_r * boss_r)) {
                gs->bullets[i].active = false;
                gs->boss.hp--;
                if((gs->boss.hp <= 35 && gs->boss.phase == 0 && gs->boss.violent )|| (gs->boss.hp <= 15 && gs->boss.phase == 1 && !gs->boss.violent)) {
                    gs->boss.phase = 1;
                }
                if(gs->boss.hp <= 20 && gs->boss.phase == 1 && gs->boss.violent) {
                    gs->boss.phase = 2;
                }
                if (gs->boss.hp <= 0) {
                    gs->boss.active = false;
                    score += SCORE_BOSS_KILL;
                }
            }
        }
    }

    for (int j = 0; j < MAX_BALLS; j++) {
        if (!gs->balls[j].active) continue;
        float dx = gs->player.x - gs->balls[j].x;
        float dy = gs->player.y - gs->balls[j].y;
        int   r  = ball_radius(a, gs->balls[j].size) + player_r;
        if (dx*dx + dy*dy < (float)(r * r)) {
            if (gs->player.invuln_timer > 0) break;
            if (gs->player.has_second_life) {
                gs->player.has_second_life = false;
                gs->player.invuln_timer = PLAYER_INVULN_TICKS;
                gs->balls[j].active = false;
                gs->active_balls--;
            } else {
                gs->player.hp = 0;
            }
            return;
        }
    }

    if (gs->boss.active) {
        int   boss_r = a->boss[0]->w / 2;
        float dx = gs->player.x - gs->boss.x;
        float dy = gs->player.y - gs->boss.y;
        int   r  = player_r + boss_r;
        if (dx*dx + dy*dy < (float)(r * r) && gs->player.invuln_timer == 0) {
            if (gs->player.has_second_life) {
                gs->player.has_second_life = false;
                gs->player.invuln_timer = PLAYER_INVULN_TICKS;
            } else {
                gs->player.hp = 0;
            }
        }
    }

    for (int i = 0; i < MAX_UPGRADES; i++) {
        Upgrade *u = &gs->upgrades[i];
        if (!u->active) continue;
        int   upg_r = a->upg_speed->w / 2;
        float dx = gs->player.x - u->x;
        float dy = gs->player.y - u->y;
        int   r  = player_r + upg_r;
        if (dx*dx + dy*dy < (float)(r * r)) {
            apply_upgrade(&gs->player, u->type);
            u->active = false;
        }
    }

}
