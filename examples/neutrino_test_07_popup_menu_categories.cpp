#include "common/Neu_TestCommon.hpp"

using namespace neutrino;
using namespace neutrino_tests;


struct PopupBundle {
    Neu_PopWindowMenu* menu{nullptr};
    Neu_Textbox* status{nullptr};
};

static void on_popup_show(Neu_Control*, void* user_data)
{
    auto* bundle = static_cast<PopupBundle*>(user_data);
    if (!bundle || !bundle->menu) {
        return;
    }
    bundle->menu->show();
    if (bundle->status) {
        bundle->status->setText("Neu_PopWindowMenu::show() opened the menu.");
    }
}

static void on_popup_hide(Neu_Control*, void* user_data)
{
    auto* bundle = static_cast<PopupBundle*>(user_data);
    if (!bundle || !bundle->menu) {
        return;
    }
    bundle->menu->hide();
    if (bundle->status) {
        bundle->status->setText("Neu_PopWindowMenu::hide() closed the menu.");
    }
}

static void on_popup_toggle(Neu_Control*, void* user_data)
{
    auto* bundle = static_cast<PopupBundle*>(user_data);
    if (!bundle || !bundle->menu) {
        return;
    }
    bundle->menu->toggle();
    if (bundle->status) {
        bundle->status->setText(bundle->menu->isVisible() ? "Neu_PopWindowMenu::toggle() opened the menu." : "Neu_PopWindowMenu::toggle() closed the menu.");
    }
}

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
    menu->setHintText("Neu_PopWindowMenu draws menu categories on the left and category items on the right. Use show(), hide(), toggle(), showAt(), and isVisible() for explicit popup control.");
    menu->hide();
    win.add(menu);

    PopupBundle popupBundle{menu.get(), status.get()};

    auto showButton = std::make_shared<Neu_Button>(Neu_Layout{690, 74, 90, 36, 1.0f, 120, 44});
    showButton->setText("Show");
    Neu_Callbacks showCb;
    showCb.onClick = on_popup_show;
    showCb.userData = &popupBundle;
    showButton->setCallbacks(showCb);
    win.add(showButton);

    auto hideButton = std::make_shared<Neu_Button>(Neu_Layout{790, 74, 90, 36, 1.0f, 120, 44});
    hideButton->setText("Hide");
    Neu_Callbacks hideCb;
    hideCb.onClick = on_popup_hide;
    hideCb.userData = &popupBundle;
    hideButton->setCallbacks(hideCb);
    win.add(hideButton);

    auto toggleButton = std::make_shared<Neu_Button>(Neu_Layout{690, 120, 190, 36, 1.0f, 220, 44});
    toggleButton->setText("Toggle popup");
    Neu_Callbacks toggleCb;
    toggleCb.onClick = on_popup_toggle;
    toggleCb.userData = &popupBundle;
    toggleButton->setCallbacks(toggleCb);
    win.add(toggleButton);

    auto command = std::make_shared<Neu_MenuItem>(Neu_Layout{690, 170, 180, 44, 1.0f, 230, 58});
    command->setText("Command Item");
    command->setIconBmp("assets/icons/menu_icon.bmp");
    command->setHintText("A standalone Neu_MenuItem uses the same implementation as Neu_Button and Neu_FlatButton.");
    command->setCallbacks(click_callbacks(status.get()));
    win.add(command);

    auto label = std::make_shared<Neu_MultilineLabel>(Neu_Layout{690, 240, 190, 210, 1.0f, 240, 260});
    label->setText("Popup-menu feature coverage:\n- categories\n- item list\n- rounded glass drawing\n- menu-item BMP icon\n- hint popup support");
    label->setIconBmp("assets/icons/sample_icon.bmp");
    win.add(label);

    win.show();
    app.run();
    return 0;
}
