#include "generator_ui.h"

#include <algorithm>
#include <functional>
#include <iosfwd>
#include <string>
#include <vector>

#include "cata_utility.h"
#include "color.h"
#include "cursesdef.h"
#include "input.h"
#include "optional.h"
#include "output.h"
#include "point.h"
#include "string_formatter.h"
#include "text_snippets.h"
#include "translations.h"
#include "ui_manager.h"

static catacurses::window init_window()
{
    const int width = generator_ui::WIDTH;
    const int height = generator_ui::HEIGHT;
    const point p(std::max(0, (TERMX - width) / 2), std::max(0, (TERMY - height) / 2));
    return catacurses::newwin(height, width, p);
}

generator_ui_settings::generator_ui_settings(bool& enabled, int& load_min, int &load_max) :
                                                enabled( enabled ), load_min(load_min), load_max( load_max ) {}

generator_ui::generator_ui( generator_ui_settings initial_settings ) :
    win(init_window()), input_ctx("GENERATORS"), settings(initial_settings)
{
    input_ctx.register_directions();
    input_ctx.register_action("QUIT");
    input_ctx.register_action("CONFIRM");
    input_ctx.register_action("NEXT_TAB");
}


void generator_ui::refresh()
{
    werase(win);
    draw_border(win);

    const nc_color gray = c_light_gray;
    const nc_color white = c_white;
    const nc_color lgreen = c_light_green;
    const nc_color red = c_red;

    // header
    const std::string title = _("Generator controls");
    mvwprintz(win, point((WIDTH - title.length()) / 2, 1), white, title);
    mvwhline(win, point(1, 2), LINE_OXOX, WIDTH - 2);

    // for menu items, y points to the center of the menu item vertical space
    int y = 3 + MENU_ITEM_HEIGHT / 2;

    // enabled flag
    mvwprintz(win, point(LEFT_MARGIN, y), gray, "[ ]");
    if (settings.enabled) {
        mvwprintz(win, point(LEFT_MARGIN + 1, y), white, "X");
    }
    mvwprintz(win, point(LEFT_MARGIN + 4, y), selection == 0 ? hilite(white) : gray,
        _("Enabled"));

    // load % slider
    y += MENU_ITEM_HEIGHT;
    mvwprintz(win, point(LEFT_MARGIN, y - 1), gray,
        _("Generator load (% of Maximum)"));

    int lo_slider_x = settings.load_min * SLIDER_W / 100;
    // print selected % numbers
    std::string low_load_text = string_format("%d%%", settings.load_min);
    mvwprintz(win, point(LEFT_MARGIN + lo_slider_x - low_load_text.length() + 1, y + 2),
        selection == 1 && slider == 0 ? hilite(white) : red, low_load_text);
    // draw slider horizontal line
    for (int i = 0; i < SLIDER_W; ++i) {
        nc_color col = selection == 1 ? white : gray;
        mvwprintz(win, point(LEFT_MARGIN + i, y + 1), col, "-");
    }
    // print on the slider line
    std::string lo_text = _("-");
    mvwprintz(win, point(LEFT_MARGIN + (lo_slider_x - lo_text.length()) / 2, y + 1), red,
        lo_text);
    // print bars on the slider
    mvwprintz(win, point(LEFT_MARGIN + lo_slider_x, y + 1), selection == 1 &&
        slider == 0 ? hilite(white) : red, "|");


    // key descriptions
    std::string keys_text = string_format(
        _("Use [<color_yellow>%s</color> and <color_yellow>%s</color>] to select option.\n"
            "Use [<color_yellow>%s</color>] to change value.\n"
            "Use [<color_yellow>%s</color> or <color_yellow>%s</color>] to switch between sliders.\n"
            "Use [<color_yellow>%s</color> and <color_yellow>%s</color>] to move sliders."
            "Use [<color_yellow>%s</color>] to apply changes and quit."),
        input_ctx.get_desc("UP"),
        input_ctx.get_desc("DOWN"),
        input_ctx.get_desc("CONFIRM"),
        input_ctx.get_desc("NEXT_TAB"),
        input_ctx.get_desc("CONFIRM"),
        input_ctx.get_desc("LEFT"),
        input_ctx.get_desc("RIGHT"),
        input_ctx.get_desc("QUIT"));

    int keys_text_w = WIDTH - 2;
    int keys_text_lines_n = foldstring(keys_text, keys_text_w).size();
    fold_and_print(win, point(1, HEIGHT - 1 - keys_text_lines_n), keys_text_w, gray, keys_text);

    wnoutrefresh(win);
}

void generator_ui::control()
{
    ui_adaptor ui;
    ui.on_screen_resize([this](ui_adaptor& ui) {
        win = init_window();
        ui.position_from_window(win);
        });
    ui.mark_resize();
    ui.on_redraw([this](const ui_adaptor&) {
        refresh();
        });

    std::string action;

    do {
        ui_manager::redraw();
        action = input_ctx.handle_input();

        if (action == "CONFIRM" || (action == "NEXT_TAB" && selection == 1)) {
            settings.enabled = !settings.enabled;

        }
        else if (action == "DOWN" || action == "UP") {
            const int dy = action == "DOWN" ? 1 : -1;
            selection = (selection + MENU_ITEMS_N + dy) % MENU_ITEMS_N;
        }
        else if (selection == 1 && (action == "LEFT" || action == "RIGHT")) {
            const int dx = action == "RIGHT" ? 1 : -1;

            if (slider == 0) {
                settings.load_min = clamp((settings.load_min / 5 + dx) * 5, 5, 90);
                // keep the min space of 5 between this and other slider
                settings.load_max = std::max(settings.load_min + 5, settings.load_max);
            }
        }
    } while (action != "QUIT");

}
