//gestion des fichiers, chargement/sauvegarde du jeu par pseudo du joueur
#include <inclusive.h>
#include <fichiers.h>
#define SAVE_FILE "sauvegardes.txt"
#define MAX_NAME_LEN 20
#define MAX_SAVES 50
// Structure de sauvegarde
typedef struct {
    char pseudo[MAX_NAME_LEN];
    int  niveau;
    int  score;
} SaveEntry;

// Fonctions utilitaires internes (non exposees dans le .h)
// Charge toutes les sauvegardes 
static int charger_toutes_sauvegardes(SaveEntry entries[], int max) {
    FILE *f = fopen(SAVE_FILE, "r");
    if (!f) return 0;   // Pas de fichier = aucune sauvegarde, ce n'est pas une erreur

    int count = 0;
    while (count < max &&
           fscanf(f, "%19s %d %d",
                  entries[count].pseudo,
                  &entries[count].niveau,
                  &entries[count].score) == 3) {
        count++;
    }
    fclose(f);
    return count;
}

// Ecrit toutes les sauvegardes du tableau dans le fichier (ecrasement complet).
// Retourne true en cas de succes
static bool ecrire_toutes_sauvegardes(const SaveEntry entries[], int count) {
    FILE *f = fopen(SAVE_FILE, "w");
    if (!f) return false;

    for (int i = 0; i < count; i++) {
        fprintf(f, "%s %d %d\n", entries[i].pseudo, entries[i].niveau, entries[i].score);
    }
    fclose(f);
    return true;
}
// Sauvegarde une partie (pseudo + dernier niveau atteint + score).
// Si le pseudo existe deja, on met a jour son entree si le niveau est meilleur.
bool sauvegarder_partie(const char *pseudo, int niveau, int score) {
    SaveEntry entries[MAX_SAVES];
    int count = charger_toutes_sauvegardes(entries, MAX_SAVES);
    if (count < 0) count = 0;

    // Cherche si le pseudo existe
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
    // Nouveau joueur
    if (!trouve && count < MAX_SAVES) {
        strncpy(entries[count].pseudo, pseudo, MAX_NAME_LEN - 1);
        entries[count].pseudo[MAX_NAME_LEN - 1] = '\0';
        entries[count].niveau = niveau;
        entries[count].score  = score;
        count++;
    }

    return ecrire_toutes_sauvegardes(entries, count);
}
// Charge la sauvegarde d'un joueur a partir de son pseudo.
// Remplit *niveau_out et *score_out si la sauvegarde existe.
LoadResult charger_partie(const char *pseudo, int *niveau_out, int *score_out) {
    SaveEntry entries[MAX_SAVES];
    int count = charger_toutes_sauvegardes(entries, MAX_SAVES);
    if (count < 0) return LOAD_ERROR;

    for (int i = 0; i < count; i++) {
    for (int i = 0; i < count; i++) {
        if (strcmp(entries[i].pseudo, pseudo) == 0) {
            *niveau_out = entries[i].niveau;
            *score_out  = entries[i].score;
            return LOAD_OK;
        }
    }
    return LOAD_NOT_FOUND;
}
// Supprime la sauvegarde d'un joueur 
bool supprimer_sauvegarde(const char *pseudo) {
    SaveEntry entries[MAX_SAVES];
    int count = charger_toutes_sauvegardes(entries, MAX_SAVES);
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
// Affiche un message d'erreur 
void afficher_erreur_chargement(BITMAP *buf, const char *pseudo) {
    char msg[64];
    snprintf(msg, sizeof(msg), "Aucune sauvegarde trouvee pour : %s", pseudo);

    clear_to_color(buf, makecol(0, 20, 20));
    textout_centre_ex(buf, font, msg,
                      SCREEN_W / 2, SCREEN_H / 2,
                      makecol(255, 80, 80), -1);
    textout_centre_ex(buf, font, "Appuyez sur une touche pour continuer.",
                      SCREEN_W / 2, SCREEN_H / 2 + 20,
                      makecol(200, 200, 200), -1);

    blit(buf, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
    int timer = 0;
    while (timer < 120) {   
        vsync();
        timer++;
        if (keypressed() || (mouse_b & 1)) {
            readkey();      
            break;
        }
    }
}
