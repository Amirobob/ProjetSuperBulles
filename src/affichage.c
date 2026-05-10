#include <inclusive.h>
#include "interface.h"
#include "logique.h"
#include "affichage.h"

void draw_boss_hud(BITMAP *buf, Boss *b, int max_hp);

void Startup(BITMAP *buf, BITMAP *map) {
    blit(map, buf, 0, 0, 0, 0, SCREEN_W/2 - map->w/2, 0);
}


void draw_game(BITMAP *buf, GameAssets *a, GameState *gs) {
    clear_to_color(buf, makecol(0, 0, 0));
    blit(a->map, buf, 0, 0, SCREEN_W/2 - a->map->w/2, 0, a->map->w, a->map->h);
    draw_player(buf, a, &gs->player);
    draw_bullets(buf, a, gs);
    draw_balls(buf, a, gs);
    draw_ball_attacks(buf, a, gs);
    draw_upgrades(buf, a, gs);
    if (gs->boss.active){
        draw_boss(buf, a, &gs->boss);
        draw_boss_hud(buf, &gs->boss, gs->boss.hp_max);
    }
    draw_hud(buf, gs);
}


void draw_player(BITMAP *buf, GameAssets *a, Player *p) {
    BITMAP *spr = p->has_second_life ? a->character_blue[p->frame]
                                     : a->character[p->frame];
    int x = (int)p->x - spr->w / 2;
    int y = (int)p->y - spr->h / 2;
    if (p->facing_right) draw_sprite(buf, spr, x, y);
    else                 draw_sprite_h_flip(buf, spr, x, y);
}


void draw_bullets(BITMAP *buf, GameAssets *a, GameState *gs) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (gs->bullets[i].active) {
            int x = (int)gs->bullets[i].x - a->bullet->w / 2;
            int y = (int)gs->bullets[i].y - a->bullet->h / 2;
            draw_sprite(buf, a->bullet, x, y);
        }
    }
}


void draw_balls(BITMAP *buf, GameAssets *a, GameState *gs) {
    for (int i = 0; i < MAX_BALLS; i++) {
        if (gs->balls[i].active) {
            int x = (int)gs->balls[i].x;
            int y = (int)gs->balls[i].y;
            int r = ball_radius(a, gs->balls[i].size);
            stretch_sprite(buf, a->boule, x - r, y - r, 2 * r, 2 * r);
        }
    }
}


void draw_ball_attacks(BITMAP *buf, GameAssets *a, GameState *gs) {
    for (int i = 0; i < MAX_BALLS; i++) {
        Ball *b = &gs->balls[i];
        if (!b->active) continue;
        int r = ball_radius(a, b->size);

        if (b->attack_state == BALL_WARNING) {
            BITMAP *spr = a->exclamation;
            int sx = (int)b->x - spr->w / 2;
            int sy = (int)b->y - r - spr->h - 4;
            draw_sprite(buf, spr, sx, sy);
        } else if (b->attack_state == BALL_FIRING) {
            BITMAP *spr = a->beam;
            int half_w = (spr->w / 2) / b->size;
            int top    = (int)b->y + r;
            int bottom = SCREEN_H;
            stretch_sprite(buf, spr,
                           (int)b->x - half_w, top,
                           half_w * 2, bottom - top);
        }
    }
}


void draw_boss(BITMAP *buf, GameAssets *a, Boss *b) {
    if (!b->active) return;
    int frame = (b->phase == 2) ? 1 : 0;
    if (frame >= bossframes) frame = 0;
    BITMAP *spr = a->boss[frame];
    int x = (int)b->x - spr->w / 2;
    int y = (int)b->y - spr->h / 2;
    draw_sprite(buf, spr, x, y);
}

void draw_boss_hud(BITMAP *buf, Boss *b, int max_hp) {
    if (!b->active) return;

    const int bar_w = 400;
    const int bar_h = 18;
    const int bar_x = SCREEN_W / 2 - bar_w / 2;
    const int bar_y = 12;

    rectfill(buf, bar_x, bar_y, bar_x + bar_w, bar_y + bar_h, makecol(40, 40, 40));

    int fill = (b->hp * bar_w) / max_hp;
    if (fill < 0) fill = 0;

    int col;
    if      (b->phase == 0) col = makecol(80, 200, 80);
    else if (b->phase == 1) col = makecol(220, 140, 0);
    else                    col = makecol(200, 40, 40);

    rectfill(buf, bar_x, bar_y, bar_x + fill, bar_y + bar_h, col);

    rect(buf, bar_x, bar_y, bar_x + bar_w, bar_y + bar_h, makecol(200, 200, 200));

    textout_centre_ex(buf, font, "BOSS", SCREEN_W / 2, bar_y + bar_h + 4,
                      makecol(255, 255, 255), -1);
}

void draw_upgrades(BITMAP *buf, GameAssets *a, GameState *gs) {
    for (int i = 0; i < MAX_UPGRADES; i++) {
        Upgrade *u = &gs->upgrades[i];
        if (!u->active) continue;
        BITMAP *spr = a->upg_speed;
        if      (u->type == UPG_HEALTH) spr = a->upg_health;
        else if (u->type == UPG_BULLET) spr = a->upg_bullet;
        draw_sprite(buf, spr, (int)u->x - spr->w / 2, (int)u->y - spr->h / 2);
    }
}


void draw_hud(BITMAP *buf, GameState *gs) {
    rectfill(buf, 0, SCREEN_H - 30, SCREEN_W, SCREEN_H, makecol(0, 0, 0));

    int time_left = gs->level_timer / GAME_FPS;
    if (time_left < 0) time_left = 0;

    char line[160];
    sprintf(line, "Player: %s    Level: %d    Score: %d    Time: %d",
            (const char *)username, level, score, time_left);
    textout_centre_ex(buf, font, line, SCREEN_W / 2, SCREEN_H - 20,
                      makecol(255, 255, 255), -1);
}
