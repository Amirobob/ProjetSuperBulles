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
/* counts one per render. paired with player y over time it tells you
   whether the timer is ticking AND whether physics is actually advancing. */
static int debug_render_count = 0;

void draw_game(BITMAP *buf, GameAssets *a, GameState *gs) {
    clear_to_color(buf, makecol(0, 0, 0));
    /* paint the map at the same position Startup uses */
    blit(a->map, buf, 0, 0, SCREEN_W/2 - a->map->w/2, 0, a->map->w, a->map->h);
    draw_player(buf, a, &gs->player);
    draw_bullets(buf, a, gs);
    draw_balls(buf, a, gs);

    /* debug overlay — remove once movement works.
       y/vy: physics state. on_ground: has the player landed?
       frames: counts up every redraw, so if it's frozen, the loop is stuck.
       sample: what one pixel of mapalpha looks like at the player's centre —
               tells you if your "empty" colour matches bitmap_mask_color. */
    char line[128];
    Player *p = &gs->player;
    int ox = SCREEN_W/2 - a->map->w/2;
    int sample_lx = (int)p->x - ox;
    int sample_ly = (int)p->y;
    int pix = (sample_lx >= 0 && sample_lx < a->mapalpha->w &&
               sample_ly >= 0 && sample_ly < a->mapalpha->h)
              ? getpixel(a->mapalpha, sample_lx, sample_ly) : -1;
    int mask = bitmap_mask_color(a->mapalpha);

    sprintf(line, "y=%.1f  vy=%.2f  ground=%d  frames=%d",
            p->y, p->vy, p->on_ground, ++debug_render_count);
    textout_ex(buf, font, line, 5, 5, makecol(255, 255, 0), -1);

    sprintf(line, "pixel under player = %d   mask color = %d", pix, mask);
    textout_ex(buf, font, line, 5, 20, makecol(255, 255, 0), -1);
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
            int size = 40;  /* taille simple des balles */
            
            stretch_sprite(buf, a->boule, x - size/2, y - size/2, size, size);
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
    /* TODO: format a string like "HP:3  Score:120  Level:2" with sprintf
             and textout_ex it in a corner. you can also show username
             from the global on the other side. */
}
