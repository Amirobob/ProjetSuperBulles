#include <inclusive.h>


#include <interface.h>

volatile bool exit_flag = false;

void initialisation_allegro();

void time_to_exit(void) {
    exit_flag = true;
}

int main(){
    initialisation_allegro();

    BITMAP *menu_buf = create_bitmap(SCREEN_W, SCREEN_H); 
    main_menu(menu_buf);
    




    destroy_bitmap(menu_buf);
    allegro_exit();
    return 0;
}
END_OF_MAIN();



void initialisation_allegro() {
    allegro_init();
    set_window_title("Super Pang!");
    install_keyboard();
    install_mouse();
    set_color_depth(desktop_color_depth());
    if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, 800, 600, 0, 0) != 0) {
        allegro_message("probleme mode graphique");
        allegro_exit();
        exit(EXIT_FAILURE);
    }
    enable_triple_buffer();
    set_close_button_callback(time_to_exit);
}