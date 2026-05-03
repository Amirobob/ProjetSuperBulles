#include <inclusive.h>


#include <interface.h>

void initialisation_allegro();



int main(){
    initialisation_allegro();
    show_mouse(screen);
    printf("hello world!"); //this is a test here makefile will not work without this otherwise I swear!!!
    BITMAP *menu_buffer = create_bitmap(SCREEN_W, SCREEN_H); 
    main_menu(menu_buffer);
    




    destroy_bitmap(menu_buffer);
    allegro_exit();
    return 0;
}
END_OF_MAIN();



void initialisation_allegro() {
    allegro_init();
    install_keyboard();
    install_mouse();
    set_color_depth(desktop_color_depth());
    if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, 800, 600, 0, 0) != 0) {
        allegro_message("probleme mode graphique");
        allegro_exit();
        exit(EXIT_FAILURE);
    }
}