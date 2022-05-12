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
    const point p( std::max( 0, ( TERMX - width ) / 2 ), std::max( 0, ( TERMY - height ) / 2 ) );
    return catacurses::newwin( height, width, p );
}

generator_ui_settings::generator_ui_settings( bool &enabled, int &sel_load, int &bat_fill ) :
    enabled( enabled ), sel_load( sel_load ), bat_fill( bat_fill ) {}

generator_ui::generator_ui( generator_ui_settings initial_settings ) :
    win( init_window() ), input_ctx( "GENERATORS" ), settings( initial_settings )
{
    input_ctx.register_directions();
    input_ctx.register_action( "QUIT" );
    input_ctx.register_action( "CONFIRM" );
    input_ctx.register_action( "NEXT_TAB" );
}


void generator_ui::refresh()
{
    werase( win );
    draw_border( win );

    const nc_color gray = c_light_gray;
    const nc_color white = c_white;
    const nc_color lgreen = c_light_green;
    const nc_color red = c_red;

    // header
    const std::string title = _( "Generator controls" );
    mvwprintz( win, point( ( WIDTH - title.length() ) / 2, 1 ), white, title );
    mvwhline( win, point( 1, 2 ), LINE_OXOX, WIDTH - 2 );

    // for menu items, y points to the center of the menu item vertical space
    int y = 3 + MENU_ITEM_HEIGHT / 2;

    // enabled flag
    mvwprintz( win, point( LEFT_MARGIN, y ), gray, "[ ]" );
    if( settings.enabled ) {
        mvwprintz( win, point( LEFT_MARGIN + 1, y ), white, "X" );
    }
    mvwprintz( win, point( LEFT_MARGIN + 4, y ), selection == 0 ? hilite( white ) : gray,
               _( "Enabled" ) );

    // load % slider
    y += MENU_ITEM_HEIGHT;
    mvwprintz( win, point( LEFT_MARGIN, y - 1 ), selection == 1 ? white : gray,
               _( "Generator load (%% of Maximum)" ) );

    int load_slider_x = settings.sel_load * SLIDER_W / 100;
    // print selected % numbers
    std::string low_load_text = string_format( "%d%%", settings.sel_load );
    mvwprintz( win, point( LEFT_MARGIN + load_slider_x - low_load_text.length() + 1, y + 2 ),
               selection == 1 ? hilite( white ) : red, low_load_text );
    // draw slider horizontal line
    for( int i = 0; i < SLIDER_W; ++i ) {
        nc_color col = selection == 1 ? white : gray;
        mvwprintz( win, point( LEFT_MARGIN + i, y + 1 ), col, "-" );
    }

    // print bars on the first slider
    mvwprintz( win, point( LEFT_MARGIN + load_slider_x, y + 1 ), selection == 1 ? hilite( white ) : red,
               "|" );

    // Battery fill % slider
    y += MENU_ITEM_HEIGHT;
    mvwprintz( win, point( LEFT_MARGIN, y - 1 ), selection == 2 ? white : gray,
               _( "Fill battery until %%" ) );

    int battery_slider_x = settings.bat_fill * SLIDER_W / 100;
    // print selected % numbers
    std::string battery_fill_text = string_format( "%d%%", settings.bat_fill );
    mvwprintz( win, point( LEFT_MARGIN + battery_slider_x - battery_fill_text.length() + 1, y + 2 ),
               selection == 2 ? hilite( white ) : red, battery_fill_text );
    // draw slider horizontal line
    for( int i = 0; i < SLIDER_W; ++i ) {
        nc_color col = selection == 2 ? white : gray;
        mvwprintz( win, point( LEFT_MARGIN + i, y + 1 ), col, "-" );
    }

    // print bars on the second slider
    mvwprintz( win, point( LEFT_MARGIN + battery_slider_x, y + 1 ),
               selection == 2 ? hilite( white ) : red, "|" );



    // key descriptions
    std::string keys_text = string_format(
                                _( "Use [<color_yellow>%s</color> and <color_yellow>%s</color>] to select option.\n"
                                   "Use [<color_yellow>%s</color>] to change value.\n"
                                   "Use [<color_yellow>%s</color> or <color_yellow>%s</color>] to switch between sliders.\n"
                                   "Use [<color_yellow>%s</color> and <color_yellow>%s</color>] to move sliders."
                                   "Use [<color_yellow>%s</color>] to apply changes and quit." ),
                                input_ctx.get_desc( "UP" ),
                                input_ctx.get_desc( "DOWN" ),
                                input_ctx.get_desc( "CONFIRM" ),
                                input_ctx.get_desc( "NEXT_TAB" ),
                                input_ctx.get_desc( "CONFIRM" ),
                                input_ctx.get_desc( "LEFT" ),
                                input_ctx.get_desc( "RIGHT" ),
                                input_ctx.get_desc( "QUIT" ) );

    int keys_text_w = WIDTH - 2;
    int keys_text_lines_n = foldstring( keys_text, keys_text_w ).size();
    fold_and_print( win, point( 1, HEIGHT - 1 - keys_text_lines_n ), keys_text_w, gray, keys_text );

    wnoutrefresh( win );
}

void generator_ui::control()
{
    ui_adaptor ui;
    ui.on_screen_resize( [this]( ui_adaptor & ui ) {
        win = init_window();
        ui.position_from_window( win );
    } );
    ui.mark_resize();
    ui.on_redraw( [this]( const ui_adaptor & ) {
        refresh();
    } );

    std::string action;

    do {
        ui_manager::redraw();
        action = input_ctx.handle_input();

        if( action == "CONFIRM" && selection == 0 ) {
            settings.enabled = !settings.enabled;
                
        } else if( action == "DOWN" || action == "UP" ) {
            const int dy = action == "DOWN" ? 1 : -1;
            selection = ( selection + MENU_ITEMS_N + dy ) % MENU_ITEMS_N;
        } else if( action == "LEFT" || action == "RIGHT" ) {
            const int dx = action == "RIGHT" ? 1 : -1;
            if( selection == 1 ) {
                settings.sel_load = clamp( ( settings.sel_load / 5 + dx ) * 5, 5, 90 );
            } else if (selection == 2){
                settings.bat_fill = clamp( ( settings.bat_fill / 5 + dx ) * 5, 5, 90 );
            }

        }
    } while( action != "QUIT" );

}
