// la logique du jeu : boucle principale, entités, collisions.
#include <inclusive.h>
#include "logique.h"
#include "affichage.h"
#include "interface.h"

/* Timer de fps qui ne dépend pas de la vitesse de rendu */
static volatile int tick_counter = 0;
static void tick_increment(void) { tick_counter++; }
END_OF_FUNCTION(tick_increment);


/* Ce fonction démarre le jeu et gère la boucle principale.
   les params seedent les globals (username/level/score) que end_menu et le HUD lisent. */
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
        if (outcome < 0) break;            /* user quit */

        /* end_menu mute level/score selon le bouton clique (retry/next/save). */
        end_menu(buf, outcome == 1);
        if (exit_flag) break;
    }

    free_assets(&assets);
}


/* freezes the world and overlays "3 / 2 / 1 / Go!" so the player can scout the level
   before physics starts. called from game_loop before install_int_ex. */
static void countdown_intro(BITMAP *buf, GameAssets *a, GameState *gs) {
    static const char * const steps[] = {"3", "2", "1", "Go!"};

    for (int i = 0; i < 4 && !exit_flag; i++) {
        draw_game(buf, a, gs);
        textout_centre_ex(buf, font, steps[i],
                          SCREEN_W / 2, SCREEN_H / 2,
                          makecol(255, 255, 80), -1);
        blit(buf, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);

        /* hand-roll the wait so we can bail out instantly on close-button. */
        for (int frames = 0; frames < 50 && !exit_flag; frames++) vsync();
    }
}


/* Le loop principal du jeu.
   retourne 1 = won, 0 = lost, -1 = quit. */
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
                update_upgrades(gs);
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
        if (gs->level_timer <= 0)  { outcome = 0; break; }   /* timeout = loss */
        if (is_level_complete(gs)) {
            /* per-level time bonus: faster wins get less, per the spec formula. */
            int seconds_left = gs->level_timer / GAME_FPS;
            if (seconds_left < 0) seconds_left = 0;
            score += 100 - seconds_left;
            outcome = 1;
            break;
        }
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
    p->drop_timer = 0;
}


/* ---------- level framework ----------
   Edit each case to design what level N contains: ball spawns, boss config,
   upgrade placements, etc. Common reset (player position, timers, clearing
   arrays) lives in init_level — only put level-specific stuff here. */
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
            /* boss level — needs update_boss() implemented for the boss to actually move/attack. */
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
            /* boss level — needs update_boss() implemented for the boss to actually move/attack. */
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
            /* fallback for levels beyond what's designed: scale ball count by level. */
            for (int i = 0; i < (level_num < 8 ? level_num : 8); i++) {
                float dir = (i % 2 == 0) ? 2.0f : -2.0f;
                spawn_ball(gs, SCREEN_W / 2.0f + (i - 2) * 100.0f, 150.0f, dir, 0.0f, 1);
            }
            break;
    }
}


/* set up everything for a level: player, bullets all inactive, balls all inactive,
   then delegate to populate_level for the level-specific spawns. */
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

    /* Initialize pipe spawn timers */
    gs->pipe_spawn_timer[0] = 60;   /* tuyau gauche */
    gs->pipe_spawn_timer[1] = 75;   /* tuyau centre */
    gs->pipe_spawn_timer[2] = 90;   /* tuyau droite */

    populate_level(gs, level_num);

    (void)level_num;  /* unused for now */
}


/* level is "done" when there are no balls left and the boss (if any) is dead. */
bool is_level_complete(const GameState *gs) {
    return gs->active_balls == 0 && !gs->boss.active;
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

int ball_radius(GameAssets *a, int size) {
    /* size = denominator: 1 = full boule sprite, 2 = half, 3 = third. */
    return (a->boule->w / 2) / size;
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
    const float floor_pop = -10.0f;   /* upward velocity given on floor bounce */

    for (int i = 0; i < MAX_BALLS; i++) {
        Ball *b = &gs->balls[i];
        if (!b->active) continue;

        b->vy += gravity;

        /* horizontal axis: try to move; if blocked, flip vx instead. */
        float new_x = b->x + b->vx;
        if (ball_hits_map(a, (int)new_x, (int)b->y, ball_radius(a, b->size))) {
            b->vx = -b->vx;
        } else {
            b->x = new_x;
        }

        /* vertical axis: floor pops back up, ceiling just inverts. */
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
        /* changement de direction aléatoire */
        b->attack_timer--;
        if (b->attack_timer <= 0) {
            b->attack_timer = GAME_FPS + rand() % (2 * GAME_FPS);
            float speed = 1.5f + (rand() % 30) / 10.0f;
            b->vx = (rand() % 2 == 0) ? speed : -speed;
        }

        b->x += b->vx;

        /* rebond sur les bords de l'écran */
        if (b->x < 0)        { b->x = 0;        b->vx =  fabsf(b->vx); }
        if (b->x > SCREEN_W) { b->x = SCREEN_W; b->vx = -fabsf(b->vx); }

    } else if(b->phase == 1) {
        b->attack_timer--;
        if (b->attack_timer <= 0) {
            /* change de direction plus souvent, vitesse plus élevée */
            b->attack_timer = GAME_FPS / 2 + rand() % GAME_FPS;
            float speed = 3.0f + (rand() % 30) / 10.0f;
            b->vx = (rand() % 2 == 0) ?  speed : -speed;
            b->vy = (rand() % 2 == 0) ?  speed : -speed;
        }

        b->x += b->vx;
        b->y += b->vy;

        /* rebond sur les 4 bords */
        if (b->x < 0)        { b->x = 0;        b->vx =  fabsf(b->vx); }
        if (b->x > SCREEN_W) { b->x = SCREEN_W; b->vx = -fabsf(b->vx); }
        if (b->y < 0)        { b->y = 0;        b->vy =  fabsf(b->vy); }
        if (b->y > SCREEN_H) { b->y = SCREEN_H; b->vy = -fabsf(b->vy); }

    } else if (b->phase == 2) {
        b->attack_timer--;
        if (b->attack_timer <= 0) {
            b->attack_timer = (int)(7.5f * GAME_FPS);

            float spd = 18.0f;
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
            /* 1 ball toutes les 2s, direction aléatoire horizontale */
            b->spawn_timer = 4 * GAME_FPS;
            float vx = (rand() % 2 == 0) ? 2.0f : -2.0f;
            spawn_ball(gs, b->x, b->y, vx, 0.0f, 2);

        } else if (b->phase == 1) {
            /* 2 balls toutes les 0.75s, directions diagonales opposées */
            b->spawn_timer = 3 * GAME_FPS;
            spawn_ball(gs, b->x, b->y,  2.5f, -1.0f, 2);
            spawn_ball(gs, b->x, b->y, -2.5f, -1.0f, 2);

        } else if (b->phase == 2) {
            /* 4 balls toutes les 2s, directions cardinales, plus rapides que les précédentes. */
            b->spawn_timer = 2* GAME_FPS;
            spawn_ball(gs, b->x, b->y,  3.0f,  0.0f, 3);
            spawn_ball(gs, b->x, b->y, -3.0f,  0.0f, 3);
            spawn_ball(gs, b->x, b->y,  0.0f,  3.0f, 3);
            spawn_ball(gs, b->x, b->y,  0.0f, -3.0f, 3);
        }
    }
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

/* called when a bullet hits a ball. if the ball wasn't the smallest size,
   replace it with two smaller ones moving in opposite directions. */
void split_ball(GameState *gs, int idx) {
    Ball *b = &gs->balls[idx];
    int parent_size = b->size;

    b->active = false;
    gs->active_balls--;

    /* size 3 is smallest — no children, full kill bonus. otherwise spawn two
       halved children moving outward and upward, partial split bonus. */
    if (parent_size < 3) {
        spawn_ball(gs, b->x, b->y,  2.0f, -4.0f, parent_size + 1);
        spawn_ball(gs, b->x, b->y, -2.0f, -4.0f, parent_size + 1);
        score += 25;
    } else {
        score += 100;
    }
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
void check_collisions(GameState *gs, GameAssets *a) {
    const int player_r = a->character[0]->w / 2;

    /* bullet vs ball: split_ball decides whether to spawn children. */
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

    /* bullet vs boss: drains hp, awards 500 on kill. */
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
                    score += 500;
                }
            }
        }
    }

    /* player vs ball: any contact ends the game. */
    for (int j = 0; j < MAX_BALLS; j++) {
        if (!gs->balls[j].active) continue;
        float dx = gs->player.x - gs->balls[j].x;
        float dy = gs->player.y - gs->balls[j].y;
        int   r  = ball_radius(a, gs->balls[j].size) + player_r;
        if (dx*dx + dy*dy < (float)(r * r)) {
            gs->player.hp = 0;
            return;
        }
    }

    /* player vs boss: any contact ends the game. */
    if (gs->boss.active) {
        int   boss_r = a->boss[0]->w / 2;
        float dx = gs->player.x - gs->boss.x;
        float dy = gs->player.y - gs->boss.y;
        int   r  = player_r + boss_r;
        if (dx*dx + dy*dy < (float)(r * r)) {
            gs->player.hp = 0;
        }
    }
}


/* pause_menu now lives in interface.c alongside the other menus. */
