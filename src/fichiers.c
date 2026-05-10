#include <inclusive.h>
#include "fichiers.h"
#define SAVE_FILE "sauvegardes.txt"

int lister_sauvegardes(SaveEntry entries[], int max) {
    FILE *f = fopen(SAVE_FILE, "r");
    if (!f) return 0;

    int count = 0;
    while (count < max &&
           fscanf(f, "%19s %d %d",
                  entries[count].pseudo,
                  &entries[count].niveau,
                  &entries[count].score) == 3) {
        count++;
    }
    char dummy;
    bool overflow = (count == max && fscanf(f, " %c", &dummy) == 1);
    fclose(f);
    return overflow ? -1 : count;
}

static bool ecrire_toutes_sauvegardes(const SaveEntry entries[], int count) {
    FILE *f = fopen(SAVE_FILE, "w");
    if (!f) return false;

    for (int i = 0; i < count; i++) {
        fprintf(f, "%s %d %d\n", entries[i].pseudo, entries[i].niveau, entries[i].score);
    }
    fclose(f);
    return true;
}

bool sauvegarder_partie(const char *pseudo, int niveau, int score) {
    SaveEntry entries[MAX_SAVES];
    int count = lister_sauvegardes(entries, MAX_SAVES);
    if (count < 0) return false;

    bool trouve = false;
    for (int i = 0; i < count; i++) {
        if (strcmp(entries[i].pseudo, pseudo) == 0) {
            if (niveau > entries[i].niveau ||
               (niveau == entries[i].niveau && score > entries[i].score)) {
                entries[i].niveau = niveau;
                entries[i].score  = score;
            }
            trouve = true;
            break;
        }
    }
    if (!trouve && count < MAX_SAVES) {
        strncpy(entries[count].pseudo, pseudo, MAX_NAME_LEN - 1);
        entries[count].pseudo[MAX_NAME_LEN - 1] = '\0';
        entries[count].niveau = niveau;
        entries[count].score  = score;
        count++;
    }

    return ecrire_toutes_sauvegardes(entries, count);
}

LoadResult charger_partie(const char *pseudo, int *niveau_out, int *score_out) {
    SaveEntry entries[MAX_SAVES];
    int count = lister_sauvegardes(entries, MAX_SAVES);
    if (count < 0) return LOAD_ERROR;

    for (int i = 0; i < count; i++) {
        if (strcmp(entries[i].pseudo, pseudo) == 0) {
            *niveau_out = entries[i].niveau;
            *score_out  = entries[i].score;
            return LOAD_OK;
        }
    }
    return LOAD_NOT_FOUND;
}

bool supprimer_sauvegarde(const char *pseudo) {
    SaveEntry entries[MAX_SAVES];
    int count = lister_sauvegardes(entries, MAX_SAVES);
    if (count <= 0) return false;

    int idx = -1;
    for (int i = 0; i < count; i++) {
        if (strcmp(entries[i].pseudo, pseudo) == 0) {
            idx = i;
            break;
        }
    }
    if (idx < 0) return false;
    for (int i = idx; i < count - 1; i++) {
        entries[i] = entries[i + 1];
    }
    count--;
    return ecrire_toutes_sauvegardes(entries, count);
}

static void wait_for_dismiss(void) {
    while ((mouse_b & 1) && !exit_flag) vsync();
    clear_keybuf();
    while (!exit_flag) {
        vsync();
        if (keypressed())  { readkey(); break; }
        if (mouse_b & 1)   { break; }
    }
    while ((mouse_b & 1) && !exit_flag) vsync();
}

static void afficher_message(BITMAP *buf, const char *titre, const char *detail, int couleur_titre) {
    clear_to_color(buf, makecol(0, 20, 20));
    textout_centre_ex(buf, font, titre,
                      SCREEN_W / 2, SCREEN_H / 2 - 20, couleur_titre, -1);
    if (detail) {
        textout_centre_ex(buf, font, detail,
                          SCREEN_W / 2, SCREEN_H / 2, makecol(220, 220, 220), -1);
    }
    textout_centre_ex(buf, font, "Press any key to continue.",
                      SCREEN_W / 2, SCREEN_H / 2 + 30, makecol(180, 180, 180), -1);
    blit(buf, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
    wait_for_dismiss();
}

void afficher_erreur_chargement(BITMAP *buf, const char *pseudo) {
    char msg[64];
    snprintf(msg, sizeof(msg), "No save found for: %s", pseudo);
    afficher_message(buf, msg, NULL, makecol(255, 80, 80));
}

void afficher_chargement_succes(BITMAP *buf, const char *pseudo, int niveau, int score) {
    char titre[64], details[64];
    snprintf(titre,   sizeof(titre),   "Save loaded: %s", pseudo);
    snprintf(details, sizeof(details), "Level %d   Score %d", niveau, score);
    afficher_message(buf, titre, details, makecol(120, 220, 120));
}

void afficher_save_succes(BITMAP *buf, const char *pseudo, int niveau, int score) {
    char titre[64], details[64];
    snprintf(titre,   sizeof(titre),   "Game saved: %s", pseudo);
    snprintf(details, sizeof(details), "Level %d   Score %d", niveau, score);
    afficher_message(buf, titre, details, makecol(120, 220, 120));
}
