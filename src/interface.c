#include <inclusive.h>
#include "logique.h"
#include "fichiers.h"

typedef struct {
    int x1, y1, x2, y2;
}rectangle;



bool mouse_over(int x1, int y1, int x2, int y2) {
    return mouse_x >= x1 && mouse_x <= x2 && mouse_y >= y1 && mouse_y <= y2;
}

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

        make_button(buf, play_b.x1, play_b.y1, play_b.x2, play_b.y2, "play", mouse_over(play_b.x1, play_b.y1, play_b.x2, play_b.y2));
        make_button(buf, load_b.x1, load_b.y1, load_b.x2, load_b.y2, "load game", mouse_over(load_b.x1, load_b.y1, load_b.x2, load_b.y2));
        make_button(buf, exit_b.x1, exit_b.y1, exit_b.x2, exit_b.y2, "exit", mouse_over(exit_b.x1, exit_b.y1, exit_b.x2, exit_b.y2));

        textout_centre_ex(buf, font, "Super Bulles!", SCREEN_W / 2, SCREEN_H / 8, makecol(255, 255, 255), -1);
        show_mouse(buf);

        if (mouse_b & 1 && mouse_over(exit_b.x1, exit_b.y1, exit_b.x2, exit_b.y2)) {
            exit_flag = true;
            return;
        }

        if (mouse_b & 1 && mouse_over(play_b.x1, play_b.y1, play_b.x2, play_b.y2)) {
            enter_name(buf);
        }

        if (mouse_b & 1 && mouse_over(load_b.x1, load_b.y1, load_b.x2, load_b.y2)) {
            find_save_file(buf);
        }

        vsync();
        blit(buf, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
        if (exit_flag) return;
    }

}


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
    const char *err_msg = "";

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
            } else if (c == '\b' && pos > 0) {
                name[--pos] = '\0';
            } else if (pos < sizeof(name) - 1 && c >= ' ' && c <= '~') {
                name[pos++] = c;
                name[pos] = '\0';
            }

            if (pos == 0) {
                valid = false;
                err_msg = "";
            } else if (!has_letter(name)) {
                valid = false;
                err_msg = "Name must contain at least one letter.";
            } else if (has_spaces(name)) {
                valid = false;
                err_msg = "Name must not contain spaces.";
            } else {
                int dummy_lvl, dummy_score;
                if (charger_partie(name, &dummy_lvl, &dummy_score) == LOAD_OK) {
                    valid = false;
                    err_msg = "This name is already taken.";
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
    SaveEntry entries[MAX_SAVES];
    int count = lister_sauvegardes(entries, MAX_SAVES);

    while ((mouse_b & 1) && !exit_flag) vsync();

    const int row_w  = 500;
    const int row_h  = 38;
    const int gap    = 6;
    const int top    = 90;
    const int row_x1 = SCREEN_W / 2 - row_w / 2;
    const int row_x2 = SCREEN_W / 2 + row_w / 2;
    const int max_visible = (SCREEN_H - 80 - top) / (row_h + gap);

    rectangle back_b = {SCREEN_W / 2 - 80, SCREEN_H - 60,
                        SCREEN_W / 2 + 80, SCREEN_H - 20};

    while (!exit_flag) {
        clear_to_color(buf, makecol(0, 20, 20));
        textout_centre_ex(buf, font, "Choose a save",
                          SCREEN_W / 2, 40, makecol(255, 255, 255), -1);

        if (count == 0) {
            textout_centre_ex(buf, font, "No saves available.",
                              SCREEN_W / 2, SCREEN_H / 2,
                              makecol(255, 200, 80), -1);
        } else if (count < 0) {
            textout_centre_ex(buf, font, "File read error.",
                              SCREEN_W / 2, SCREEN_H / 2,
                              makecol(255, 80, 80), -1);
        }

        int chosen = -1;
        int visible = (count > max_visible) ? max_visible : count;
        for (int i = 0; i < visible; i++) {
            int y1 = top + i * (row_h + gap);
            int y2 = y1 + row_h;
            char label[80];
            snprintf(label, sizeof(label), "%s   Level %d   Score %d",
                     entries[i].pseudo, entries[i].niveau, entries[i].score);
            bool hover = mouse_over(row_x1, y1, row_x2, y2);
            make_button(buf, row_x1, y1, row_x2, y2, label, hover);
            if ((mouse_b & 1) && hover) chosen = i;
        }

        if (count > max_visible) {
            char overflow[64];
            snprintf(overflow, sizeof(overflow),
                     "(+%d saves not shown)", count - max_visible);
            textout_centre_ex(buf, font, overflow,
                              SCREEN_W / 2, SCREEN_H - 80,
                              makecol(180, 180, 180), -1);
        }

        make_button(buf, back_b.x1, back_b.y1, back_b.x2, back_b.y2,
                    "back", mouse_over(back_b.x1, back_b.y1, back_b.x2, back_b.y2));
        show_mouse(buf);

        vsync();
        blit(buf, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);

        if (chosen >= 0) {
            while ((mouse_b & 1) && !exit_flag) vsync();
            afficher_chargement_succes(buf, entries[chosen].pseudo,
                                       entries[chosen].niveau, entries[chosen].score);
            startgame(buf, entries[chosen].pseudo,
                      entries[chosen].niveau, entries[chosen].score);
            return;
        }
        if ((mouse_b & 1) && mouse_over(back_b.x1, back_b.y1, back_b.x2, back_b.y2)) {
            while ((mouse_b & 1) && !exit_flag) vsync();
            return;
        }
        if (key[KEY_ESC]) {
            while (key[KEY_ESC]) {}
            return;
        }
    }
}


bool pause_menu(BITMAP *buf, GameState *gs) {
    gs->paused = true;

    rectangle continue_b  = {SCREEN_W / 2 - SCREEN_W / 3, 2 * SCREEN_H / 8, SCREEN_W / 2 + SCREEN_W / 3, 3 * SCREEN_H / 8};
    rectangle quit_game_b = {SCREEN_W / 2 - SCREEN_W / 3, 4 * SCREEN_H / 8, SCREEN_W / 2 + SCREEN_W / 3, 5 * SCREEN_H / 8};
    rectangle quit_app_b  = {SCREEN_W / 2 - SCREEN_W / 3, 6 * SCREEN_H / 8, SCREEN_W / 2 + SCREEN_W / 3, 7 * SCREEN_H / 8};

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

    while (key[KEY_ESC])     { }
    while (mouse_b & 1)      vsync();

    gs->paused = false;
    return quit_game;
}


bool end_menu(BITMAP *buf, bool won) {

    rectangle retry_b = {SCREEN_W / 2 - SCREEN_W / 3, SCREEN_H / 8, SCREEN_W / 2 + SCREEN_W / 3, 2 * SCREEN_H / 8};
    rectangle save_game_b = {SCREEN_W / 2 - SCREEN_W / 3, 3 * SCREEN_H / 8, SCREEN_W / 2 + SCREEN_W / 3, 4 * SCREEN_H / 8};
    rectangle main_menu_b = {SCREEN_W / 2 - SCREEN_W / 3, 5 * SCREEN_H / 8, SCREEN_W / 2 + SCREEN_W / 3, 6 * SCREEN_H / 8};
    rectangle quit_b = {SCREEN_W / 2 - SCREEN_W / 3, 7 * SCREEN_H / 8, SCREEN_W / 2 + SCREEN_W / 3, 8 * SCREEN_H / 8  - 20};

    bool saved = false;
    bool retry = false;

    while (!exit_flag) {
        clear_to_color(buf, won ? makecol(0, 20, 20) : makecol(20, 0, 0));

        make_button(buf, quit_b.x1, quit_b.y1, quit_b.x2, quit_b.y2, "quit", mouse_over(quit_b.x1, quit_b.y1, quit_b.x2, quit_b.y2));
        make_button(buf, save_game_b.x1, save_game_b.y1, save_game_b.x2, save_game_b.y2, "save game", mouse_over(save_game_b.x1, save_game_b.y1, save_game_b.x2, save_game_b.y2));
        make_button(buf, retry_b.x1, retry_b.y1, retry_b.x2, retry_b.y2, won ? "next level" : "retry", mouse_over(retry_b.x1, retry_b.y1, retry_b.x2, retry_b.y2));
        make_button(buf, main_menu_b.x1, main_menu_b.y1, main_menu_b.x2, main_menu_b.y2, "main menu", mouse_over(main_menu_b.x1, main_menu_b.y1, main_menu_b.x2, main_menu_b.y2));

        textout_centre_ex(buf, font, won ? "Victory!" : "Defeat",
                          SCREEN_W / 2, SCREEN_H / 16,
                          won ? makecol(120, 220, 120) : makecol(220, 80, 80), -1);
        show_mouse(buf);

        if (mouse_b & 1) {
            if (mouse_over(main_menu_b.x1, main_menu_b.y1, main_menu_b.x2, main_menu_b.y2)) break;
            if (mouse_over(quit_b.x1, quit_b.y1, quit_b.x2, quit_b.y2))     { exit_flag = true; break; }
            if (mouse_over(retry_b.x1, retry_b.y1, retry_b.x2, retry_b.y2)) {
                if (won) { level++; score += 1000; }
                else     { score = 0; }
                retry = true;
                break;
            }
            if (!saved && mouse_over(save_game_b.x1, save_game_b.y1, save_game_b.x2, save_game_b.y2)) {
                sauvegarder_partie((const char *)username, level, score);
                afficher_save_succes(buf, (const char *)username, level, score);
                saved = true;
            }
        } else {
            saved = false;
        }

        vsync();
        blit(buf, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
    }
    return retry;
}


bool final_victory_menu(BITMAP *buf, BITMAP *bg) {
    rectangle inf_b  = {SCREEN_W / 2 - 200, 3 * SCREEN_H / 8,
                        SCREEN_W / 2 + 200, 3 * SCREEN_H / 8 + 55};
    rectangle save_b = {SCREEN_W / 2 - 200, 3 * SCREEN_H / 8 + 70,
                        SCREEN_W / 2 + 200, 3 * SCREEN_H / 8 + 125};
    rectangle menu_b = {SCREEN_W / 2 - 200, 3 * SCREEN_H / 8 + 140,
                        SCREEN_W / 2 + 200, 3 * SCREEN_H / 8 + 195};
    rectangle quit_b = {SCREEN_W / 2 - 200, 3 * SCREEN_H / 8 + 210,
                        SCREEN_W / 2 + 200, 3 * SCREEN_H / 8 + 265};

    bool saved = false;
    bool save_armed = true;

    while ((mouse_b & 1) && !exit_flag) vsync();

    while (!exit_flag) {
        if (bg) blit(bg, buf, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
        else    clear_to_color(buf, makecol(20, 30, 60));

        int cx = SCREEN_W / 2, cy = SCREEN_H / 8;
        int gold = makecol(255, 220, 80);
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++)
                textout_centre_ex(buf, font, "FINAL VICTORY!",
                                  cx + dx, cy + dy, gold, -1);

        textout_centre_ex(buf, font, "You beat the final boss!",
                          cx, cy + 30, makecol(220, 220, 220), -1);

        char line[80];
        snprintf(line, sizeof(line), "Player: %s    Final Score: %d",
                 (const char *)username, score);
        textout_centre_ex(buf, font, line, cx, cy + 55,
                          makecol(220, 220, 220), -1);

        if (saved) {
            textout_centre_ex(buf, font, "Score saved.",
                              cx, cy + 80, makecol(120, 220, 120), -1);
        }

        make_button(buf, inf_b.x1, inf_b.y1, inf_b.x2, inf_b.y2,
                    "infinite mode",
                    mouse_over(inf_b.x1, inf_b.y1, inf_b.x2, inf_b.y2));
        make_button(buf, save_b.x1, save_b.y1, save_b.x2, save_b.y2,
                    "save score",
                    mouse_over(save_b.x1, save_b.y1, save_b.x2, save_b.y2));
        make_button(buf, menu_b.x1, menu_b.y1, menu_b.x2, menu_b.y2,
                    "main menu",
                    mouse_over(menu_b.x1, menu_b.y1, menu_b.x2, menu_b.y2));
        make_button(buf, quit_b.x1, quit_b.y1, quit_b.x2, quit_b.y2,
                    "quit game",
                    mouse_over(quit_b.x1, quit_b.y1, quit_b.x2, quit_b.y2));

        show_mouse(buf);

        if (mouse_b & 1) {
            if (mouse_over(inf_b.x1, inf_b.y1, inf_b.x2, inf_b.y2)) {
                while ((mouse_b & 1) && !exit_flag) vsync();
                return true;
            }
            if (mouse_over(menu_b.x1, menu_b.y1, menu_b.x2, menu_b.y2)) {
                while ((mouse_b & 1) && !exit_flag) vsync();
                return false;
            }
            if (mouse_over(quit_b.x1, quit_b.y1, quit_b.x2, quit_b.y2)) {
                exit_flag = true;
                return false;
            }
            if (save_armed && mouse_over(save_b.x1, save_b.y1, save_b.x2, save_b.y2)) {
                sauvegarder_partie((const char *)username, level, score);
                saved = true;
                save_armed = false;
            }
        } else {
            save_armed = true;
        }

        vsync();
        blit(buf, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
    }
    return false;
}
