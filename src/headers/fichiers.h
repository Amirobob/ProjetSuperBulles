#ifndef FICHIERS_H
#define FICHIERS_H
#include <stdbool.h>
#include <allegro.h>
// Codes de retour pour charger_partie()
typedef enum {
    LOAD_OK        = 0,  
    LOAD_NOT_FOUND = 1,   
    LOAD_ERROR     = 2   
} LoadResult;
// Sauvegarde ou met a jour la partie d'un joueur.
// Seule la meilleure progression (niveau le plus eleve) est conservee.
// Retourne true en cas de succes, false si le fichier est plein ou inaccessible.
bool sauvegarder_partie(const char *pseudo, int niveau, int score);
// Charge la sauvegarde du joueur identifie par pseudo.
LoadResult charger_partie(const char *pseudo, int *niveau_out, int *score_out);
// Supprime la sauvegarde d'un joueur.
// Retourne true si une entree a effectivement ete supprimee.
bool supprimer_sauvegarde(const char *pseudo);
// Affiche un message d'erreur a l'ecran quand aucune sauvegarde n'est trouvee.
// A appeler depuis interface.c apres un LOAD_NOT_FOUND.
void afficher_erreur_chargement(BITMAP *buf, const char *pseudo);

// Affiche un ecran de confirmation apres un chargement reussi, avant de lancer la partie.
// Attend une touche / un clic.
void afficher_chargement_succes(BITMAP *buf, const char *pseudo, int niveau, int score);

// Affiche un ecran de confirmation apres une sauvegarde reussie. Attend une touche / un clic.
void afficher_save_succes(BITMAP *buf, const char *pseudo, int niveau, int score);
#endif // FICHIERS_H
