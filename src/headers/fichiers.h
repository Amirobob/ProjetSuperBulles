#ifndef FICHIERS_H
#define FICHIERS_H
#include <stdbool.h>
#include <allegro.h>

#define MAX_NAME_LEN 20
#define MAX_SAVES    50

typedef struct {
    char pseudo[MAX_NAME_LEN];
    int  niveau;
    int  score;
} SaveEntry;

typedef enum {
    LOAD_OK        = 0,
    LOAD_NOT_FOUND = 1,
    LOAD_ERROR     = 2
} LoadResult;

int lister_sauvegardes(SaveEntry entries[], int max);
bool sauvegarder_partie(const char *pseudo, int niveau, int score);
LoadResult charger_partie(const char *pseudo, int *niveau_out, int *score_out);
bool supprimer_sauvegarde(const char *pseudo);
void afficher_erreur_chargement(BITMAP *buf, const char *pseudo);
void afficher_chargement_succes(BITMAP *buf, const char *pseudo, int niveau, int score);
void afficher_save_succes(BITMAP *buf, const char *pseudo, int niveau, int score);

#endif
