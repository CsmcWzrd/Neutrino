#include "common/Neu_TestCommon.hpp"

using namespace neutrino;
using namespace neutrino_tests;

int main(int argc, char** argv)
{
    Neu_Application app;
    if (!start_app(app, argc, argv)) {
        return 1;
    }

    Neu_Window win(app, 900, 560, "Neutrino Test 01 - Buttons, Flat Buttons, Menu Items, Icons");
    apply_test_window_defaults(win);
    if (!win.create()) {
        return 1;
    }

    auto status = add_status(win, "Click any button-style control. All callbacks are function pointers.");
    Neu_Callbacks click = click_callbacks(status.get());

    auto button = std::make_shared<Neu_Button>(Neu_Layout{35, 85, 230, 46, 1.0f, 280, 60});
    button->setText("Neu_Button + BMP");
    button->setIconBmp("assets/icons/button_icon.bmp");
    button->setHintText("Neu_Button uses rounded drawing, BMP-only icon loading, hover highlighting, shadows, and function-pointer click callbacks.");
    button->setCallbacks(click);
    win.add(button);

    auto flat = std::make_shared<Neu_FlatButton>(Neu_Layout{300, 85, 230, 46, 1.0f, 280, 60});
    flat->setText("Neu_FlatButton + BMP");
    flat->setIconBmp("assets/icons/save_icon.bmp");
    flat->setHintText("Neu_FlatButton is the same button family, useful for toolbar-like UI.");
    flat->setCallbacks(click);
    win.add(flat);

    auto menu = std::make_shared<Neu_MenuItem>(Neu_Layout{565, 85, 230, 46, 1.0f, 280, 60});
    menu->setText("Neu_MenuItem + BMP");
    menu->setIconBmp("assets/icons/menu_icon.bmp");
    menu->setHintText("Neu_MenuItem is equivalent to the button/flat button callback path and supports BMP icons.");
    menu->setCallbacks(click);
    win.add(menu);

    auto placement = std::make_shared<Neu_Placement>(Neu_Layout{35, 170, 760, 290, 1.0f, 820, 340});
    placement->setText("Nested Neu_Placement with button controls");
    placement->setHintText("The placement container performs sublayouting and forwards parent window/redraw state to nested controls.");

    auto nested1 = std::make_shared<Neu_Button>(Neu_Layout{55, 225, 210, 42, 1.0f, 240, 55});
    nested1->setText("Nested Button");
    nested1->setIconBmp("assets/icons/sample_icon.bmp");
    nested1->setCallbacks(click);
    placement->add(nested1);

    auto nested2 = std::make_shared<Neu_FlatButton>(Neu_Layout{300, 225, 210, 42, 1.0f, 240, 55});
    nested2->setText("Nested Flat");
    nested2->setIconBmp("assets/icons/save_icon.bmp");
    nested2->setCallbacks(click);
    placement->add(nested2);

    auto nested3 = std::make_shared<Neu_MenuItem>(Neu_Layout{545, 225, 210, 42, 1.0f, 240, 55});
    nested3->setText("Nested Menu Item");
    nested3->setIconBmp("assets/icons/menu_icon.bmp");
    nested3->setCallbacks(click);
    placement->add(nested3);

    win.add(placement);

    win.show();
    app.run();
    return 0;
}
