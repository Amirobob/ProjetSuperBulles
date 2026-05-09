// la logique du jeu : boucle principale, entités, collisions.
#include <inclusive.h>
#include "logique.h"
#include "affichage.h"
#include "interface.h"

/* Timer de fps qui ne dépend pas de la vitesse de rendu */
static volatile int tick_counter = 0;
static void tick_increment(void) { tick_counter++; }
END_OF_FUNCTION(tick_increment);


/* Ce fonction démarre le jeu et gère la boucle principale */
void startgame(BITMAP *buf) {
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
        if (outcome < 0) break;            /* user quit */

        end_menu(buf, outcome == 1);       /* outcome ==1 retourne TRUE or FALSE */
        if (outcome == 1) level++;         /* celui ici nous laisse gérer les niveaux */
        if (exit_flag) break;
    }

    free_assets(&assets);
}


/* Le loop principal du jeu.
   retourne 1 = won, 0 = lost, -1 = quit. */
int game_loop(BITMAP *buf, GameAssets *a, GameState *gs) {
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
                update_upgrades(gs);
                check_collisions(gs);
            }
            tick_counter--;
        }

        /* draw once per loop */
        draw_game(buf, a, gs);
        vsync();
        blit(buf, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);

        if (key[KEY_ESC])           pause_menu(buf, gs);
        if (is_player_dead(gs))    { outcome = 0; break; }
        if (is_level_complete(gs)) { outcome = 1; break; }
    }

    remove_int(tick_increment);
    return outcome;
}


/* assets */
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
    }
    for (int i = 0; i < bossframes; i++) {
        sprintf(path, "src/character/boss_%d.bmp", i);
        a->boss[i] = load_bitmap(path, NULL);
        if (!a->boss[i]) return false;
    }
    return true;
}

void free_assets(GameAssets *a) {
    /* destroy_bitmap accepts NULL */
    for (int i = 0; i < charframes; i++) destroy_bitmap(a->character[i]);
    for (int i = 0; i < bossframes; i++) destroy_bitmap(a->boss[i]);
    destroy_bitmap(a->map);
    destroy_bitmap(a->mapalpha);
    destroy_bitmap(a->bullet);
    destroy_bitmap(a->boule);
    destroy_bitmap(a->upg_speed);
    destroy_bitmap(a->upg_health);
    destroy_bitmap(a->upg_bullet);
}


/* ---------- setup ---------- */

/* put the player at the starting position and reset their stats.
   call this at the start of each level (or only on retry, if you want
   speed/double-shot upgrades to carry over between levels). */
void init_player(Player *p, GameAssets *a) {
    /* the map is drawn centred horizontally at the top of the screen.
       compute its left/top edge, then place the player relative to that —
       this way the spawn moves with the map if its size changes. */
    int map_left = SCREEN_W / 2 - a->map->w / 2;
    int map_top  = 0;

    p->x = map_left + 80.0f;        
    p->y = map_top  + a->map->h - 100.0f;  

    p->vx = 0;
    p->vy = 0;
    p->on_ground = false;
    p->hp = p->max_hp = 3;
    p->speed = 3;
    p->double_shot = false;
    p->frame = 0;
    p->frame_timer = 0;
    p->facing_right = true;
    p->shoot_cooldown = 0;
    p->iframes = 0;
    p->drop_timer = 0;
}


/* set up everything for a level: player, bullets all inactive, balls all inactive,
   then spawn the right number of balls and a boss if the level calls for one. */
void init_level(GameState *gs, GameAssets *a, int level_num) {
    init_player(&gs->player, a);

    memset(gs->bullets,  0, sizeof(gs->bullets));
    memset(gs->balls,    0, sizeof(gs->balls));
    memset(gs->upgrades, 0, sizeof(gs->upgrades));
    gs->boss.active = false;
    gs->active_balls = 0;
    gs->paused = false;

    /* TODO later: spawn balls based on level_num, and a boss every Nth level. */

    (void)level_num;  /* unused for now */
}


/* level is "done" when there are no balls left and the boss (if any) is dead. */
bool is_level_complete(const GameState *gs) {
    /* always false in the minimal demo — there's nothing to clear yet. */
    (void)gs;
    return false;
}

bool is_player_dead(const GameState *gs) { return gs->player.hp <= 0; }


/* ---------- input ---------- */

/* read the keyboard once per tick and act on it.
   moves the player and fires bullets. */
void handle_input(GameState *gs) {
    Player *p = &gs->player;

    p->vx = 0;
    if (key[KEY_LEFT])  { p->vx = -(float)p->speed; p->facing_right = false; }
    if (key[KEY_RIGHT]) { p->vx =  (float)p->speed; p->facing_right = true;  }

    if (key[KEY_UP]) {
        if (p->on_ground) { p->vy -= 11.5f; p->on_ground = false; }
        else if (p->vy > 0) p->vy -= 0.5f;  /* held = glide-fall */
    }

    /* drop through red platforms */
    if (key[KEY_DOWN] && p->on_ground) {
        p->drop_timer = 15;
        p->on_ground = false;
    }

    if (key[KEY_SPACE] && p->shoot_cooldown == 0) {
        if (p->double_shot) {
            spawn_bullet(gs, p->x + 5, p->y - 10);
            spawn_bullet(gs, p->x - 5, p->y - 10);
        }
        else {
            spawn_bullet(gs, p->x, p->y - 10);
        }
        p->shoot_cooldown = GAME_FPS / 2;
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

/* perimeter sample of a w×h box. step ≤ thinnest platform you expect. */
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

    /* horizontal */
    {
        float new_x = p->x + p->vx;
        int   left  = (int)new_x - hw;
        int   top   = (int)p->y  - hh;
        if (!box_hits(a, left, top, spr->w, spr->h, platforms_solid))
            p->x = new_x;
    }

    /* gravity, capped to avoid tunneling through thin floors */
    p->vy += 0.5f;
    if (p->vy > 12.0f) p->vy = 12.0f;

    /* vertical */
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

    if (p->vx != 0 && ++p->frame_timer >= 8) {
        p->frame = (p->frame + 1) % charframes;
        p->frame_timer = 0;
    }
    if (p->iframes        > 0) p->iframes--;
    if (p->drop_timer     > 0) p->drop_timer--;
    if (p->shoot_cooldown > 0) p->shoot_cooldown--;
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

int ball_radius(int size) {
    /* size 0 smallest, size BALL_SIZES-1 biggest. tweak the multiplier to taste. */
    return 8 * (size + 1);
}

/* compass sample of 8 points on the ball's perimeter against the map. */
static bool ball_hits_map(GameAssets *a, int cx, int cy, int r) {
    static const int dx[8] = { 0,  1, 1, 1, 0, -1, -1, -1 };
    static const int dy[8] = {-1, -1, 0, 1, 1,  1,  0, -1 };
    for (int i = 0; i < 8; i++) {
        if (is_solid_pixel(a, cx + dx[i] * r, cy + dy[i] * r, false)) return true;
    }
    return false;
}

void update_balls(GameState *gs, GameAssets *a) {
    const float gravity   = 0.15f;
    const float floor_pop = -8.0f;   /* upward velocity given on floor bounce */

    for (int i = 0; i < MAX_BALLS; i++) {
        Ball *b = &gs->balls[i];
        if (!b->active) continue;

        b->vy += gravity;

        /* horizontal axis: try to move; if blocked, flip vx instead. */
        float new_x = b->x + b->vx;
        if (ball_hits_map(a, (int)new_x, (int)b->y, ball_radius(b->size))) {
            b->vx = -b->vx;
        } else {
            b->x = new_x;
        }

        /* vertical axis: floor pops back up, ceiling just inverts. */
        float new_y = b->y + b->vy;
        if (ball_hits_map(a, (int)b->x, (int)new_y, ball_radius(b->size))) {
            if (b->vy > 0) b->vy = floor_pop;
            else           b->vy = -b->vy;
        } else {
            b->y = new_y;
        }
    }
}

void update_boss(GameState *gs) {
    if (!gs->boss.active) return;
    /* TODO:
       - move the boss left/right; flip vx at the screen edges.
       - advance the animation frame every N ticks (use bossframes).
       - increment attack_timer; when it crosses a threshold, do something
         (drop a ball, shoot, change phase) and reset the timer. */
}

void update_upgrades(GameState *gs) {
    /* TODO: for each active upgrade, drift it downward (y += small amount).
             if it goes off the bottom, mark it inactive (player missed it). */
}


/* ---------- spawning ---------- */

void spawn_bullet(GameState *gs, float x, float y) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (gs->bullets[i].active == false) {
            gs->bullets[i].x = x;
            gs->bullets[i].y = y;
            gs->bullets[i].vy = -7.0f;  /* shoot upward */
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
            gs->active_balls++;
            break;
        }
    }
}

/* called when a bullet hits a ball. if the ball wasn't the smallest size,
   replace it with two smaller ones moving in opposite directions. */
void split_ball(GameState *gs, int idx) {
    /* TODO:
       - delete old ball
       - spawn two with opposite velocities IF the ball destroyed was big enough*/
}

void spawn_upgrade(GameState *gs, float x, float y) {
    /* TODO: find a free upgrade slot, set position, pick a random type
             (rand() % UPG_COUNT), mark it active. */
}

void apply_upgrade(Player *p, UpgradeType t) {
    /* TODO:
       - UPG_SPEED  → bump p->speed.
       - UPG_HEALTH → +1 hp, but don't go past max_hp.
       - UPG_BULLET → set double_shot = true. */
}


/* ---------- collisions ---------- */

/* checks every pair that can collide. happens once per tick. */
void check_collisions(GameState *gs) {

    /* bullet vs ball:
       TODO: for each active bullet, check against every active ball.
             a simple test is "distance from bullet to ball centre < ball radius".
             on hit:
               - mark the bullet inactive.
               - small chance to spawn an upgrade where the ball was.
               - add to score (bigger balls = more points).
               - split_ball(gs, j).
               - break out of the inner loop so the bullet doesn't hit twice. */

    /* bullet vs boss:
       TODO: same idea. on hit, deactivate the bullet and decrement boss hp.
             once boss hp <= 0, set boss.active = false. */

    /* player vs ball:
       TODO: only check if player.iframes == 0 (not in i-frames).
             on hit: hp--, set iframes to ~60 ticks so the player isn't hit
             repeatedly on the same touch. break after the first hit. */

    /* player vs upgrade:
       TODO: rectangle overlap or distance check. on pickup:
             apply_upgrade(...) and mark the upgrade inactive. */
}


/* ---------- pause ---------- */

void pause_menu(BITMAP *buf, GameState *gs) {
    gs->paused = true;
    while (key[KEY_ESC]) { }
    while (!key[KEY_ESC] && !exit_flag) {
        textout_centre_ex(buf, font, "Game paused",
                          SCREEN_W / 2, SCREEN_H / 2, makecol(255, 255, 255), -1);
        vsync();
        blit(buf, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
    }
    while (key[KEY_ESC]) { }
    gs->paused = false;
}
