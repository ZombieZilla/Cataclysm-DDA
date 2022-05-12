#pragma once
#ifndef CATA_SRC_GENERATOR_UI_H
#define CATA_SRC_GENERATOR_UI_H

#include "cursesdef.h"
#include "input.h"

struct generator_ui_settings {
    bool &enabled;
    int &sel_load;
    int &bat_fill;
    generator_ui_settings( bool &enabled, int &sel_load, int &bat_fill );
};

class generator_ui
{
    public:
        static const int WIDTH = 52;
        static const int HEIGHT = 36;

        explicit generator_ui( generator_ui_settings initial_settings );

        // open UI and allow user to interact with it
        void control();

    private:

        static const int LEFT_MARGIN = 6;

        static const int MENU_ITEM_HEIGHT = 5;
        static const int MENU_ITEMS_N = 3;

        static const int SLIDER_W = 40;

        // Output window. This class assumes win's size does not change.
        catacurses::window win;
        input_context input_ctx;
        // current state of settings
        const generator_ui_settings settings;

        // selected menu row, 0 = button, 1 = load slider, 2 = battery slider
        int selection = 0;
        // draws the window's content
        void refresh();
};

#endif // CATA_SRC_GENERATOR_UI_H
