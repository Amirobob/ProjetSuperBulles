// la logique du jeu : boucle principale, entités, collisions.
#include <inclusive.h>
#include "logique.h"
#include "affichage.h"
#include "interface.h"

/* timer that ticks GAME_FPS times per second so logic runs at a steady rate
   no matter how fast or slow rendering is. */
static volatile int tick_counter = 0;
static void tick_increment(void) { tick_counter++; }
END_OF_FUNCTION(tick_increment);


/* called by the menu when the player clicks "play".
   sets up assets, then keeps replaying levels until the player quits. */
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

        end_menu(buf, outcome == 1);       /* end_menu shows the win/lose screen */
        if (outcome == 1) level++;         /* won → next level. lost → replay same level. */
        if (exit_flag) break;
    }

    free_assets(&assets);
}


/* the main game loop.
   keeps running until the player dies, beats the level, or quits.
   returns 1 = won, 0 = lost, -1 = quit. */
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
                update_bullets(gs);
                update_balls(gs);
                update_boss(gs);
                update_upgrades(gs);
                check_collisions(gs);
            }
            tick_counter--;
        }

        /* draw once per loop iteration */
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


/* ---------- assets ---------- */

/* load every bitmap once at game start so we don't reload them each level.
   returns false if any bitmap fails to load — caller should bail out. */
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

/* free everything load_assets allocated. safe even if some bitmaps are NULL. */
void free_assets(GameAssets *a) {
    for (int i = 0; i < charframes; i++) if (a->character[i]) destroy_bitmap(a->character[i]);
    for (int i = 0; i < bossframes; i++) if (a->boss[i])      destroy_bitmap(a->boss[i]);
    if (a->map)        destroy_bitmap(a->map);
    if (a->mapalpha)   destroy_bitmap(a->mapalpha);
    if (a->bullet)     destroy_bitmap(a->bullet);
    if (a->boule)      destroy_bitmap(a->boule);
    if (a->upg_speed)  destroy_bitmap(a->upg_speed);
    if (a->upg_health) destroy_bitmap(a->upg_health);
    if (a->upg_bullet) destroy_bitmap(a->upg_bullet);
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

    p->x = map_left + a->map->w / 2.0f;        /* horizontal centre of the map */
    p->y = map_top  + a->map->h - 200.0f;       /* near the bottom — small gap so gravity can pull them onto the floor */

    p->vx = 0;
    p->vy = 0;
    p->on_ground = false;
    p->hp = p->max_hp = 3;
    p->speed = 3;
    p->double_shot = false;
    p->frame = 0;
    p->frame_timer = 0;
    p->facing_right = true;
    p->shoot_held = false;
    p->iframes = 0;
}


/* set up everything for a level: player, bullets all inactive, balls all inactive,
   then spawn the right number of balls and a boss if the level calls for one. */
void init_level(GameState *gs, GameAssets *a, int level_num) {
    init_player(&gs->player, a);

    /* clear every entity slot — the demo doesn't spawn anything else yet. */
    for (int i = 0; i < MAX_BULLETS;  i++) gs->bullets[i].active  = false;
    for (int i = 0; i < MAX_BALLS;    i++) gs->balls[i].active    = false;
    for (int i = 0; i < MAX_UPGRADES; i++) gs->upgrades[i].active = false;
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

    /* jump: only allowed when standing on something. KEY_UP feels natural;
       swap to KEY_SPACE later if you want it as the jump key. */
    if (key[KEY_UP] && p->on_ground) {
        p->vy = -12.0f;
        p->on_ground = false;
    }
}


/* ---------- updates: each runs once per tick ---------- */

/* sample one pixel of mapalpha. the map is centred horizontally at the top of
   the screen, so we translate screen coords back into mapalpha-local coords.
   anything outside the mapalpha bitmap is treated as solid so the player
   can't walk off the level. */
static bool is_solid_pixel(GameAssets *a, int sx, int sy) {
    BITMAP *m  = a->mapalpha;
    /* anchor the alpha to where the visible map is drawn, not to its own size.
       this way even if mapalpha and map differ in width, collisions line up
       with what the player sees. */
    int    ox  = SCREEN_W / 2 - a->map->w / 2;
    int    lx  = sx - ox;
    int    ly  = sy;
    if (lx < 0 || lx >= m->w || ly < 0 || ly >= m->h) return true;
    /* mapalpha convention: black = wall, white = walkable.
       compare against black in the bitmap's own color depth so this works
       whether mapalpha is loaded as 8/16/24/32-bit. */
    return getpixel(m, lx, ly) == makecol_depth(bitmap_color_depth(m), 0, 0, 0);
}

/* sample the perimeter of a w×h box at screen-position (x,y).
   step controls the spacing between samples — anything smaller than `step`
   pixels thick can sneak between samples, so set it ≤ the thinnest platform
   you expect. corners-only (the original 4-point check) was missing thin
   platforms when the player walked into them sideways. */
static bool box_hits(GameAssets *a, int x, int y, int w, int h) {
    const int step  = 4;
    const int x_end = x + w - 1;
    const int y_end = y + h - 1;

    /* top + bottom edges */
    for (int sx = x; sx < x_end; sx += step) {
        if (is_solid_pixel(a, sx, y))     return true;
        if (is_solid_pixel(a, sx, y_end)) return true;
    }
    if (is_solid_pixel(a, x_end, y))     return true;
    if (is_solid_pixel(a, x_end, y_end)) return true;

    /* left + right edges (corners already covered above) */
    for (int sy = y + step; sy < y_end; sy += step) {
        if (is_solid_pixel(a, x,     sy)) return true;
        if (is_solid_pixel(a, x_end, sy)) return true;
    }
    return false;
}

void update_player(Player *p, GameAssets *a) {
    BITMAP *spr = a->character[0];
    int hw = spr->w / 2;
    int hh = spr->h / 2;

    /* horizontal: try the move; if the new position would land in a wall,
       reject it. (axis-separated tests stop the player from snagging on
       corners when they're walking and falling at the same time.) */
    {
        float new_x = p->x + p->vx;
        int   left  = (int)new_x - hw;
        int   top   = (int)p->y  - hh;
        if (!box_hits(a, left, top, spr->w, spr->h))
            p->x = new_x;
    }

    /* gravity + cap on fall speed so we don't tunnel through thin floors */
    p->vy += 0.5f;
    if (p->vy > 12.0f) p->vy = 12.0f;

    /* vertical: same idea. if the move is blocked while falling, we landed —
       set on_ground and zero vy. if blocked while rising, we hit a ceiling. */
    {
        float new_y = p->y + p->vy;
        int   left  = (int)p->x  - hw;
        int   top   = (int)new_y - hh;
        if (box_hits(a, left, top, spr->w, spr->h)) {
            if (p->vy > 0) p->on_ground = true;
            p->vy = 0;
        } else {
            p->y = new_y;
            p->on_ground = false;
        }
    }

    /* walk animation: cycle frames while moving */
    if (p->vx != 0 && ++p->frame_timer >= 8) {
        p->frame = (p->frame + 1) % charframes;
        p->frame_timer = 0;
    }
    if (p->iframes > 0) p->iframes--;
}

void update_bullets(GameState *gs) {
    /* TODO: for each active bullet, add vy to y.
             if y < 0 (off the top of the screen), mark it inactive. */
}

void update_balls(GameState *gs) {
    /* TODO: for each active ball:
       - apply gravity to vy (vy += small constant, e.g. 0.15).
       - apply vx to x and vy to y.
       - bounce off the walls: if x is out of bounds, flip vx.
       - bounce off the floor: if y is past the floor, set y to floor and
         give vy a strong upward push. you can keep them bouncing forever
         or dampen slightly each bounce.
       - bounce off the ceiling too if you want. */

    /* later, replace screen-edge bounces with map collision: sample mapalpha
       to see if the ball overlaps a wall pixel, and bounce off that. */
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

/* find the first inactive bullet slot and turn it into a new bullet.
   if none are free, the shot is silently dropped — that's fine. */
void spawn_bullet(GameState *gs, float x, float y) {
    /* TODO: loop through gs->bullets, find one with active == false,
             set its position, give it an upward vy (negative, like -6),
             mark it active, and return. */
}

void spawn_ball(GameState *gs, float x, float y, float vx, float vy, int size) {
    /* TODO: same idea as spawn_bullet but for balls.
             also increment gs->active_balls when you spawn one. */
}

/* called when a bullet hits a ball. if the ball wasn't the smallest size,
   replace it with two smaller ones moving in opposite directions. */
void split_ball(GameState *gs, int idx) {
    /* TODO:
       - if size > 0, spawn two new balls at the same position with
         opposite horizontal velocities and a small upward vy.
       - mark the original ball inactive and decrement gs->active_balls. */
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

/* freeze gameplay until ESC is pressed again.
   the timer keeps ticking but we ignore it because gs->paused is true. */
void pause_menu(BITMAP *buf, GameState *gs) {
    gs->paused = true;
    while (key[KEY_ESC]) { /* wait for release so we don't immediately unpause */ }
    while (!key[KEY_ESC] && !exit_flag) {
        textout_centre_ex(buf, font, "PAUSED - press ESC to resume",
                          SCREEN_W / 2, SCREEN_H / 2, makecol(255, 255, 255), -1);
        vsync();
        blit(buf, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
    }
    while (key[KEY_ESC]) { /* wait for release */ }
    gs->paused = false;
}
