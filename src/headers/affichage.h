#ifndef AFFICHAGE_H
#define AFFICHAGE_H

#include <inclusive.h>
#include "logique.h"

void Startup(BITMAP *buf, BITMAP *map);

void draw_game(BITMAP *buf, GameAssets *a, GameState *gs);
void draw_player(BITMAP *buf, GameAssets *a, Player *p);
void draw_bullets(BITMAP *buf, GameAssets *a, GameState *gs);
void draw_balls(BITMAP *buf, GameAssets *a, GameState *gs);
void draw_boss(BITMAP *buf, GameAssets *a, Boss *b);
void draw_upgrades(BITMAP *buf, GameAssets *a, GameState *gs);
void draw_ball_attacks(BITMAP *buf, GameAssets *a, GameState *gs);
void draw_hud(BITMAP *buf, GameState *gs);

#endif
