//ici on gére les menus du jeu, on communiquera ici principalement avec fichiers.c, et peut être avec affichage.c pour le déclenchement du jeu.
#include <inclusive.h>

typedef struct {
    int x1, y1, x2, y2;
}rectangle;



bool mouse_over(int x1, int y1, int x2, int y2) {
    return mouse_x >= x1 && mouse_x <= x2 && mouse_y >= y1 && mouse_y <= y2;
}

//declaration
void enter_name(BITMAP *buf);



void make_button(BITMAP *buf, int x1, int y1, int x2, int y2, const char *label, bool hovered) {
    
int bg = hovered ? makecol(70, 100, 160) : makecol(30, 50, 100);
rectfill(buf, x1, y1, x2, y2, bg);
rect(buf, x1, y1, x2, y2, makecol(180, 200, 255));
int tx = x1 + (x2 - x1) / 2 - text_length(font, label) / 2;
int ty = y1 + (y2 - y1) / 2 - text_height(font) / 2;
textout_ex(buf, font, label, tx, ty, makecol(255, 255, 255), -1);
}





void main_menu(BITMAP *buf){
    while(1){
        clear_to_color(buf, makecol(0, 20, 20));

        rectangle play_b = {SCREEN_W / 2 - SCREEN_W / 3, 2 * SCREEN_H / 8, SCREEN_W / 2 + SCREEN_W / 3, 3 * SCREEN_H / 8};
        rectangle load_b = {SCREEN_W / 2 - SCREEN_W / 3, 4 * SCREEN_H / 8, SCREEN_W / 2 + SCREEN_W / 3, 5 * SCREEN_H / 8};
        rectangle exit_b = {SCREEN_W / 2 - SCREEN_W / 3, 6 * SCREEN_H / 8, SCREEN_W / 2 + SCREEN_W / 3, 7 * SCREEN_H / 8};

        make_button(buf, exit_b.x1, exit_b.y1, exit_b.x2, exit_b.y2, "exit", mouse_over(exit_b.x1, exit_b.y1, exit_b.x2, exit_b.y2));
        make_button(buf, play_b.x1, play_b.y1, play_b.x2, play_b.y2, "play", mouse_over(play_b.x1, play_b.y1, play_b.x2, play_b.y2));
        make_button(buf, load_b.x1, load_b.y1, load_b.x2, load_b.y2, "load game", mouse_over(load_b.x1, load_b.y1, load_b.x2, load_b.y2));

        textout_centre_ex(buf, font, "Super Bulles!", SCREEN_W / 2, SCREEN_H / 8, makecol(255, 255, 255), -1);
        show_mouse(buf);

        //exit button
        if (mouse_b & 1 && mouse_over(exit_b.x1, exit_b.y1, exit_b.x2, exit_b.y2)) break;

        //play button
        if (mouse_b & 1 && mouse_over(play_b.x1, play_b.y1, play_b.x2, play_b.y2)) {
            enter_name(buf);
        }

        //load game button
        if (mouse_b & 1 && mouse_over(load_b.x1, load_b.y1, load_b.x2, load_b.y2)) {
            //charger la partie
        }

        vsync();
        blit(buf, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
        if (exit_flag) break;
    }

}


void enter_name(BITMAP *buf) {
    char name[20] = "", c;
    unsigned int pos = 0;
    while(1) {
        clear_to_color(buf, makecol(0, 20, 20));
        textout_centre_ex(buf, font, "Enter your name:", SCREEN_W / 2, SCREEN_H / 3, makecol(255, 255, 255), -1);
        textout_centre_ex(buf, font, name, SCREEN_W / 2, SCREEN_H / 2, makecol(255, 255, 255), -1);
        show_mouse(buf);

        if (keypressed()) {
            c = readkey();
            if (c == '\n') break; // Enter key
            if (c == '\b' && pos > 0) { // Backspace key
                name[--pos] = '\0';
            } else if (pos < sizeof(name) - 1 && c >= ' ' && c <= '~') { // Printable characters
                name[pos++] = c;
                name[pos] = '\0';
            }
        }

        vsync();
        blit(buf, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
        if (exit_flag) break;
    }
}