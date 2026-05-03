//ici on gére les menus du jeu, on communiquera ici principalement avec fichiers.c, et peut être avec affichage.c pour le déclenchement du jeu.
#include <inclusive.h>

bool mouse_over(int x1, int y1, int x2, int y2) {
    return mouse_x >= x1 && mouse_x <= x2 && mouse_y >= y1 && mouse_y <= y2;
}


const int exit_button = {100, 100, 200, 150};

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
        make_button(buf, 100, 100, 200, 150, "exit", mouse_over(100, 100, 200, 150));
        blit(buf, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
        if (mouse_b & 1 && mouse_over(100, 100, 200, 150)) break;
        rest(5);
    }

}