#include "common/Neu_TestCommon.hpp"

using namespace neutrino;
using namespace neutrino_tests;

int main(int argc, char** argv)
{
    Neu_Application app;
    if (!start_app(app, argc, argv)) {
        return 1;
    }

    Neu_Window win(app, 930, 610, "Neutrino Test 07 - Pop Window Menu Categories and Items");
    apply_test_window_defaults(win);
    if (!win.create()) {
        return 1;
    }

    auto status = add_status(win, "The pop-window menu uses a left category sidebar and right item view.");

    auto menu = std::make_shared<Neu_PopWindowMenu>(Neu_Layout{35, 88, 620, 400, 1.0f, 700, 480});
    menu->setCategories({"File", "Edit", "View", "Build", "Help"});
    menu->setItems("File", {"New Project", "Open", "Save", "Save As", "Exit"});
    menu->setItems("Edit", {"Undo", "Redo", "Cut", "Copy", "Paste"});
    menu->setItems("View", {"Zoom In", "Zoom Out", "Toggle Status", "Theme"});
    menu->setItems("Build", {"Configure", "Compile", "Run Tests", "Package"});
    menu->setItems("Help", {"Documentation", "About Neutrino"});
    menu->setHintText("Neu_PopWindowMenu draws menu categories on the left and category items on the right.");
    win.add(menu);

    auto command = std::make_shared<Neu_MenuItem>(Neu_Layout{690, 120, 180, 44, 1.0f, 230, 58});
    command->setText("Command Item");
    command->setIconBmp("assets/icons/menu_icon.bmp");
    command->setHintText("A standalone Neu_MenuItem uses the same implementation as Neu_Button and Neu_FlatButton.");
    command->setCallbacks(click_callbacks(status.get()));
    win.add(command);

    auto label = std::make_shared<Neu_MultilineLabel>(Neu_Layout{690, 190, 190, 210, 1.0f, 240, 260});
    label->setText("Popup-menu feature coverage:\n- categories\n- item list\n- rounded glass drawing\n- menu-item BMP icon\n- hint popup support");
    label->setIconBmp("assets/icons/sample_icon.bmp");
    win.add(label);

    win.show();
    app.run();
    return 0;
}
