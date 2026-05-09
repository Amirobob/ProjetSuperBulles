//ici on gére les menus du jeu, on communiquera ici principalement avec fichiers.c, et peut être avec affichage.c pour le déclenchement du jeu.
#include <inclusive.h>
#include "logique.h"
#include "fichiers.h"

typedef struct {
    int x1, y1, x2, y2;
}rectangle;



bool mouse_over(int x1, int y1, int x2, int y2) {
    return mouse_x >= x1 && mouse_x <= x2 && mouse_y >= y1 && mouse_y <= y2;
}

//declaration
void enter_name(BITMAP *buf);

void find_save_file(BITMAP *buf);



void make_button(BITMAP *buf, int x1, int y1, int x2, int y2, const char *label, bool hovered) {
    
int bg = hovered ? makecol(70, 100, 160) : makecol(30, 50, 100);
rectfill(buf, x1, y1, x2, y2, bg);
rect(buf, x1, y1, x2, y2, makecol(180, 200, 255));
int tx = x1 + (x2 - x1) / 2 - text_length(font, label) / 2;
int ty = y1 + (y2 - y1) / 2 - text_height(font) / 2;
textout_ex(buf, font, label, tx, ty, makecol(255, 255, 255), -1);
}





void main_menu(BITMAP *buf){

    rectangle play_b = {SCREEN_W / 2 - SCREEN_W / 3, 2 * SCREEN_H / 8, SCREEN_W / 2 + SCREEN_W / 3, 3 * SCREEN_H / 8};
    rectangle load_b = {SCREEN_W / 2 - SCREEN_W / 3, 4 * SCREEN_H / 8, SCREEN_W / 2 + SCREEN_W / 3, 5 * SCREEN_H / 8};
    rectangle exit_b = {SCREEN_W / 2 - SCREEN_W / 3, 6 * SCREEN_H / 8, SCREEN_W / 2 + SCREEN_W / 3, 7 * SCREEN_H / 8};

    while(1){
        clear_to_color(buf, makecol(0, 20, 20));
        
        make_button(buf, exit_b.x1, exit_b.y1, exit_b.x2, exit_b.y2, "exit", mouse_over(exit_b.x1, exit_b.y1, exit_b.x2, exit_b.y2));
        make_button(buf, play_b.x1, play_b.y1, play_b.x2, play_b.y2, "play", mouse_over(play_b.x1, play_b.y1, play_b.x2, play_b.y2));
        make_button(buf, load_b.x1, load_b.y1, load_b.x2, load_b.y2, "load game", mouse_over(load_b.x1, load_b.y1, load_b.x2, load_b.y2));

        textout_centre_ex(buf, font, "Super Bulles!", SCREEN_W / 2, SCREEN_H / 8, makecol(255, 255, 255), -1);
        show_mouse(buf);

        //exit button
        if (mouse_b & 1 && mouse_over(exit_b.x1, exit_b.y1, exit_b.x2, exit_b.y2)) {
            exit_flag = true;   /* propagates through end_menu/startgame/enter_name back to main() */
            return;
        }

        //play button
        if (mouse_b & 1 && mouse_over(play_b.x1, play_b.y1, play_b.x2, play_b.y2)) {
            enter_name(buf);
        }

        //load game button
        if (mouse_b & 1 && mouse_over(load_b.x1, load_b.y1, load_b.x2, load_b.y2)) {
            find_save_file(buf);
        }

        vsync();
        blit(buf, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
        if (exit_flag) return;
    }

}


/* "name has at least one alphabetic character." used to reject pure-digit names. */
static bool has_letter(const char *s) {
    for (; *s; s++) {
        if ((*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z')) return true;
    }
    return false;
}

static bool has_spaces(const char *s) {
    for (; *s; s++) {
        if (*s == ' ') return true;
    }
    return false;
}

void enter_name(BITMAP *buf) {
    char name[20] = "", c;
    unsigned int pos = 0;
    bool valid = false;
    const char *err_msg = "";   /* shown at bottom in red when non-empty */

    while(1) {
        clear_to_color(buf, makecol(0, 20, 20));
        rect(buf, SCREEN_W / 2 - 100, SCREEN_H / 2 - 10, SCREEN_W / 2 + 100, SCREEN_H / 2 + 19, makecol(255, 255, 255));
        textout_centre_ex(buf, font, "Enter your name:", SCREEN_W / 2, SCREEN_H / 3, makecol(255, 255, 255), -1);
        textout_centre_ex(buf, font, name, SCREEN_W / 2, SCREEN_H / 2, makecol(255, 255, 255), -1);
        if (err_msg[0]) {
            textout_centre_ex(buf, font, err_msg,
                              SCREEN_W / 2, SCREEN_H / 2 - 30,
                              makecol(255, 80, 80), -1);
        }
        show_mouse(buf);

        if (keypressed()) {
            c = readkey();

            if (key[KEY_ENTER]) {
                if (valid) {
                    strcpy((char *)username, name);
                    level = 1;
                    score = 0;
                    startgame(buf, username, level, score);
                    return;
                }
                /* else: silently ignore Enter while the name is invalid. */
            } else if (c == '\b' && pos > 0) {
                name[--pos] = '\0';
            } else if (pos < sizeof(name) - 1 && c >= ' ' && c <= '~') {
                name[pos++] = c;
                name[pos] = '\0';
            }

            /* re-validate after every keystroke. */
            if (pos == 0) {
                valid = false;
                err_msg = "";   /* nothing typed yet, no need to nag */
            } else if (!has_letter(name)) {
                valid = false;
                err_msg = "Le nom doit contenir au moins une lettre.";
            } else if (has_spaces(name)) {
                valid = false;
                err_msg = "Le nom ne doit pas contenir d'espaces.";
            } else {
                int dummy_lvl, dummy_score;
                if (charger_partie(name, &dummy_lvl, &dummy_score) == LOAD_OK) {
                    valid = false;
                    err_msg = "Ce nom est deja pris.";
                } else {
                    valid = true;
                    err_msg = "";
                }
            }
        }

        vsync();
        blit(buf, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
        if (exit_flag) break;
    }
}

void find_save_file(BITMAP *buf) {
    char name[20] = "", c;
    unsigned int pos = 0;
    while(1) {
        clear_to_color(buf, makecol(0, 20, 20));
        rect(buf, SCREEN_W / 2 - 100, SCREEN_H / 2 - 10, SCREEN_W / 2 + 100, SCREEN_H / 2 + 20, makecol(255, 255, 255));
        textout_centre_ex(buf, font, "Enter the name of your player:", SCREEN_W / 2, SCREEN_H / 3, makecol(255, 255, 255), -1);
        textout_centre_ex(buf, font, name, SCREEN_W / 2, SCREEN_H / 2, makecol(255, 255, 255), -1);
        show_mouse(buf);

        if (keypressed()) {
            c = readkey();
            if (key[KEY_ENTER]) {
                int loaded_level = 1, loaded_score = 0;
                LoadResult r = charger_partie(name, &loaded_level, &loaded_score);
                if (r == LOAD_OK) {
                    afficher_chargement_succes(buf, name, loaded_level, loaded_score);
                    startgame(buf, name, loaded_level, loaded_score);
                } else {
                    afficher_erreur_chargement(buf, name);
                }
                return;
            }
            if (c == '\b' && pos > 0) {
                name[--pos] = '\0';
            } else if (pos < sizeof(name) - 1 && c >= ' ' && c <= '~') {
                name[pos++] = c;
                name[pos] = '\0';
            }
        }

        vsync();
        blit(buf, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
        if (exit_flag) break;
    }
}



/* shown when the player presses ESC mid-game.
   returns true if they want to quit the current game (caller treats as outcome -1).
   sets exit_flag if they want to quit the entire app. */
bool pause_menu(BITMAP *buf, GameState *gs) {
    gs->paused = true;

    rectangle continue_b  = {SCREEN_W / 2 - SCREEN_W / 3, 2 * SCREEN_H / 8, SCREEN_W / 2 + SCREEN_W / 3, 3 * SCREEN_H / 8};
    rectangle quit_game_b = {SCREEN_W / 2 - SCREEN_W / 3, 4 * SCREEN_H / 8, SCREEN_W / 2 + SCREEN_W / 3, 5 * SCREEN_H / 8};
    rectangle quit_app_b  = {SCREEN_W / 2 - SCREEN_W / 3, 6 * SCREEN_H / 8, SCREEN_W / 2 + SCREEN_W / 3, 7 * SCREEN_H / 8};

    /* drain the ESC press that opened us, so it doesn't immediately resume us. */
    while (key[KEY_ESC]) { }

    bool resume    = false;
    bool quit_game = false;

    while (!resume && !quit_game && !exit_flag) {
        clear_to_color(buf, makecol(0, 0, 30));
        textout_centre_ex(buf, font, "Game paused",
                          SCREEN_W / 2, SCREEN_H / 8,
                          makecol(255, 255, 255), -1);

        make_button(buf, continue_b.x1, continue_b.y1, continue_b.x2, continue_b.y2,
                    "continue",  mouse_over(continue_b.x1, continue_b.y1, continue_b.x2, continue_b.y2));
        make_button(buf, quit_game_b.x1, quit_game_b.y1, quit_game_b.x2, quit_game_b.y2,
                    "quit game", mouse_over(quit_game_b.x1, quit_game_b.y1, quit_game_b.x2, quit_game_b.y2));
        make_button(buf, quit_app_b.x1, quit_app_b.y1, quit_app_b.x2, quit_app_b.y2,
                    "quit app",  mouse_over(quit_app_b.x1, quit_app_b.y1, quit_app_b.x2, quit_app_b.y2));

        show_mouse(buf);

        if (key[KEY_ESC]) resume = true;

        if (mouse_b & 1) {
            if      (mouse_over(continue_b.x1,  continue_b.y1,  continue_b.x2,  continue_b.y2))  resume    = true;
            else if (mouse_over(quit_game_b.x1, quit_game_b.y1, quit_game_b.x2, quit_game_b.y2)) quit_game = true;
            else if (mouse_over(quit_app_b.x1,  quit_app_b.y1,  quit_app_b.x2,  quit_app_b.y2))  {
                exit_flag = true;
                quit_game = true;
            }
        }

        vsync();
        blit(buf, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
    }

    /* don't carry the dismiss input into the resumed game (or the next menu). */
    while (key[KEY_ESC])     { }
    while (mouse_b & 1)      vsync();

    gs->paused = false;
    return quit_game;
}


void end_menu(BITMAP *buf, bool won) {

    rectangle quit_b = {SCREEN_W / 2 - SCREEN_W / 3, SCREEN_H / 8, SCREEN_W / 2 + SCREEN_W / 3, 2 * SCREEN_H / 8};
    rectangle save_game_b = {SCREEN_W / 2 - SCREEN_W / 3, 3 * SCREEN_H / 8, SCREEN_W / 2 + SCREEN_W / 3, 4 * SCREEN_H / 8};
    rectangle retry_b = {SCREEN_W / 2 - SCREEN_W / 3, 5 * SCREEN_H / 8, SCREEN_W / 2 + SCREEN_W / 3, 6 * SCREEN_H / 8};
    rectangle main_menu_b = {SCREEN_W / 2 - SCREEN_W / 3, 7 * SCREEN_H / 8, SCREEN_W / 2 + SCREEN_W / 3, 8 * SCREEN_H / 8  - 20};

    bool saved = false;   /* edge-detect: prevents holding the mouse from spamming saves */

    while(1){
        won?clear_to_color(buf, makecol(0, 20, 20)):clear_to_color(buf, makecol(20, 0, 0));

        make_button(buf, quit_b.x1, quit_b.y1, quit_b.x2, quit_b.y2, "quit", mouse_over(quit_b.x1, quit_b.y1, quit_b.x2, quit_b.y2));
        make_button(buf, save_game_b.x1, save_game_b.y1, save_game_b.x2, save_game_b.y2, "save game", mouse_over(save_game_b.x1, save_game_b.y1, save_game_b.x2, save_game_b.y2));
        make_button(buf, retry_b.x1, retry_b.y1, retry_b.x2, retry_b.y2, won ? "next level" : "retry", mouse_over(retry_b.x1, retry_b.y1, retry_b.x2, retry_b.y2));
        make_button(buf, main_menu_b.x1, main_menu_b.y1, main_menu_b.x2, main_menu_b.y2, "main menu", mouse_over(main_menu_b.x1, main_menu_b.y1, main_menu_b.x2, main_menu_b.y2));

        won?textout_centre_ex(buf, font, "Super Bulles!", SCREEN_W / 2, SCREEN_H / 8, makecol(255, 255, 255), -1):textout_centre_ex(buf, font, "Game Over", SCREEN_W / 2, SCREEN_H / 8, makecol(255, 255, 255), -1);
        show_mouse(buf);

        //main menu button
        if (mouse_b & 1 && mouse_over(main_menu_b.x1, main_menu_b.y1, main_menu_b.x2, main_menu_b.y2)) main_menu(buf);

        //quit button
        if (mouse_b & 1 && mouse_over(quit_b.x1, quit_b.y1, quit_b.x2, quit_b.y2)) {
            exit_flag = true;
            break;
        }

        //save game button
        if (mouse_b & 1 && mouse_over(save_game_b.x1, save_game_b.y1, save_game_b.x2, save_game_b.y2)) {
            if (!saved) {
                sauvegarder_partie((const char *)username, level, score);
                afficher_save_succes(buf, (const char *)username, level, score);
                saved = true;
            }
        }
        if (!(mouse_b & 1)) saved = false;   /* re-arm once mouse is released */

        //retry game button
        if (mouse_b & 1 && mouse_over(retry_b.x1, retry_b.y1, retry_b.x2, retry_b.y2)) {
            if (won) {
                level++;
                score += 1000; // exemple de score pour le niveau gagné
            }
            else {
                score = 0; // reset du score en cas de défaite
            }
            break;  /* retour a startgame, la boucle relance init_level avec le nouveau level. */
        }

        vsync();
        blit(buf, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
        if (exit_flag) break;
    }
}