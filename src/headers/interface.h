#ifndef INTERFACE_H
#define INTERFACE_H

#include <inclusive.h>

void main_menu(BITMAP *buf);
void enter_name(BITMAP *buf);
void find_save_file(BITMAP *buf);
void end_menu(BITMAP *buf, bool won);
void make_button(BITMAP *buf, int x1, int y1, int x2, int y2, const char *label, bool hovered);

#endif
