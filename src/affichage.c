// l'affichage du jeu, ceci est different de l'interface, car l'interface en gros c'est que les menus
#include <inclusive.h>
#include "interface.h"
#include "logique.h"
#include "affichage.h"


void Startup(BITMAP *buf, BITMAP *map) {
    blit(map, buf, 0, 0, 0, 0, SCREEN_W/2 - map->w/2, 0);
}


/* the master draw call: clears the buffer, paints the background, then
   layers each kind of entity on top, then the HUD. order matters — things
   drawn later sit on top. */
void draw_game(BITMAP *buf, GameAssets *a, GameState *gs) {
    clear_to_color(buf, makecol(0, 0, 0));
    /* paint the map at the same position Startup uses */
    blit(a->map, buf, 0, 0, SCREEN_W/2 - a->map->w/2, 0, a->map->w, a->map->h);
    draw_player(buf, a, &gs->player);
    draw_bullets(buf, a, gs);
    draw_balls(buf, a, gs);
    draw_hud(buf, gs);
}


void draw_player(BITMAP *buf, GameAssets *a, Player *p) {
    BITMAP *spr = a->character[p->frame];
    int x = (int)p->x - spr->w / 2;   /* p->x/p->y is the centre, draw_sprite wants top-left */
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


void draw_boss(BITMAP *buf, GameAssets *a, Boss *b) {
    /* TODO: if !b->active, return.
             pick a->boss[b->frame] and draw it centred on b->x,b->y. */
}


void draw_upgrades(BITMAP *buf, GameAssets *a, GameState *gs) {
    /* TODO: for each active upgrade, pick the right bitmap based on type:
               UPG_SPEED  → a->upg_speed
               UPG_HEALTH → a->upg_health
               UPG_BULLET → a->upg_bullet
             then draw_sprite centred on the upgrade's position. */
}


void draw_hud(BITMAP *buf, GameState *gs) {
    /* black bar across the bottom so the text reads cleanly over any background. */
    rectfill(buf, 0, SCREEN_H - 30, SCREEN_W, SCREEN_H, makecol(0, 0, 0));

    int time_left = gs->level_timer / GAME_FPS;
    if (time_left < 0) time_left = 0;

    char line[160];
    sprintf(line, "Player: %s    Level: %d    Score: %d    Time: %d",
            (const char *)username, level, score, time_left);
    textout_centre_ex(buf, font, line, SCREEN_W / 2, SCREEN_H - 20,
                      makecol(255, 255, 255), -1);
}
