// la logique du jeu est la partie oú on gère tous qui est la fonctionnalité du jeu.
#include <inclusive.h>
#include "affichage.h"

void startgame(BITMAP *buf){
    char nomfichier[256];
    BITMAP *character[charframes];
    BITMAP *boss[bossframes];
    BITMAP *map = load_bitmap("src/character/map.bmp", NULL);
    BITMAP *bullet = load_bitmap("src/character/bullet.bmp", NULL);
    BITMAP *boule = load_bitmap("src/character/boule.bmp", NULL);
    BITMAP *mapalpha = load_bitmap("src/character/mapalpha.bmp", NULL);
    BITMAP *upg_speed = load_bitmap("src/character/upgrade_speed.bmp", NULL);
    BITMAP *upg_health = load_bitmap("src/character/upgrade_health.bmp", NULL);
    BITMAP *upg_bullet = load_bitmap("src/character/upgrade_doubleshot.bmp", NULL);

    if (!map) { allegro_message("failed to load map.bmp"); exit(EXIT_FAILURE); }
    if (!bullet) { allegro_message("failed to load bullet.bmp"); exit(EXIT_FAILURE); }
    if (!boule) { allegro_message("failed to load boule.bmp"); exit(EXIT_FAILURE); }
    if (!mapalpha) { allegro_message("failed to load mapalpha.bmp"); exit(EXIT_FAILURE); }
    if (!upg_speed) { allegro_message("failed to load upgrade_speed.bmp"); exit(EXIT_FAILURE); }
    if (!upg_health) { allegro_message("failed to load upgrade_health.bmp"); exit(EXIT_FAILURE); }
    if (!upg_bullet) { allegro_message("failed to load upgrade_doubleshot.bmp"); exit(EXIT_FAILURE); }
    for (int i = 0; i < charframes; i++) {
        sprintf(nomfichier, "src/character/character_%d.bmp", i);
        character[i] = load_bitmap(nomfichier, NULL);
        if (!character[i]) { allegro_message("failed to load %s", nomfichier); exit(EXIT_FAILURE); }
    }
    
    for (int i = 0; i < bossframes; i++) {
        sprintf(nomfichier, "src/character/boss_%d.bmp", i);
        boss[i] = load_bitmap(nomfichier, NULL);
        if (!boss[i]) { allegro_message("failed to load %s", nomfichier); exit(EXIT_FAILURE); }
    }

//game section













//game section



//destroy bitmap section
for (int i = 0; i < charframes; i++) {
    sprintf(nomfichier, "character/character_%d.bmp", i);
    destroy_bitmap(character[i]);
}

for (int i = 0; i < bossframes; i++) {
    sprintf(nomfichier, "boss/boss_%d.bmp", i);
    destroy_bitmap(boss[i]);
}

destroy_bitmap(map);
destroy_bitmap(bullet);
destroy_bitmap(boule);
destroy_bitmap(mapalpha);
destroy_bitmap(upg_speed);
destroy_bitmap(upg_health);
destroy_bitmap(upg_bullet);




}