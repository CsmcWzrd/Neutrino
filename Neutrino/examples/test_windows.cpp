#include "Neutrino/Neutrino.hpp"
#include <iostream>
#include <memory>

using namespace neutrino;

struct WindowBundle {
    Neu_Window* dialog{nullptr};
    Neu_Textbox* log{nullptr};
};

static void on_close(Neu_Window* sender, void*) {
    std::cout << "Closed window xid=" << sender->xid() << std::endl;
}

static void on_open_dialog(Neu_Control*, void* user_data) {
    auto* bundle = static_cast<WindowBundle*>(user_data);
    if (bundle->log) bundle->log->setText("Dialog button clicked. Use the dialog window separately.");
    if (bundle->dialog) bundle->dialog->show();
}

static void on_theme_light(Neu_Control* sender, void*) {
    if (sender->parent()) {
        sender->parent()->setTheme(Neu_Theme::MaterialDark());
        sender->parent()->redraw();
    }
}

static void on_theme_dark(Neu_Control* sender, void*) {
    if (sender->parent()) {
        sender->parent()->setTheme(Neu_Theme::MaterialDark());
        sender->parent()->redraw();
    }
}

int main() {
    Neu_Application app;
    if (!app.open()) {
        std::cerr << "Unable to open X display. Ensure X server and DISPLAY are available.\n";
        return 1;
    }

    Neu_Window main_win(app, 760, 480, "Neutrino Test - Windows and Dialogs");
    main_win.setTheme(Neu_Theme::MaterialDark());
    main_win.setOnClose(on_close, nullptr);
    if (!main_win.create()) return 1;

    Neu_Window dialog(app, 480, 320, "Neutrino Dialog Window");
    dialog.setTheme(Neu_Theme::MaterialDark());
    dialog.setOnClose(on_close, nullptr);
    if (!dialog.create()) return 1;

    auto log = std::make_shared<Neu_Textbox>(Neu_Layout{30, 25, 600, 36, 1.0f, 700, 48});
    log->setText("Main Neu_Window. Press buttons to exercise window callbacks and themes.");
    main_win.add(log);

    WindowBundle bundle{&dialog, log.get()};

    Neu_Callbacks open_cb;
    open_cb.onClick = on_open_dialog;
    open_cb.userData = &bundle;
    auto open_dialog = std::make_shared<Neu_Button>(Neu_Layout{30, 80, 210, 42, 1.0f, 260, 55});
    open_dialog->setText("Show dialog Neu_Window");
    open_dialog->setCallbacks(open_cb);
    main_win.add(open_dialog);

    Neu_Callbacks light_cb;
    light_cb.onClick = on_theme_light;
    auto light = std::make_shared<Neu_FlatButton>(Neu_Layout{260, 80, 170, 42, 1.0f, 220, 55});
    light->setText("Light Theme");
    light->setCallbacks(light_cb);
    main_win.add(light);

    Neu_Callbacks dark_cb;
    dark_cb.onClick = on_theme_dark;
    auto dark = std::make_shared<Neu_FlatButton>(Neu_Layout{450, 80, 170, 42, 1.0f, 220, 55});
    dark->setText("Dark Theme");
    dark->setCallbacks(dark_cb);
    main_win.add(dark);

    auto menu = std::make_shared<Neu_PopWindowMenu>(Neu_Layout{30, 155, 680, 250, 1.0f, 720, 300});
    menu->setCategories({"Window", "Dialog", "Themes"});
    menu->setItems("Window", {"Neu_Window main", "onClose callback", "X11 native event dispatch"});
    menu->setItems("Dialog", {"Dialog-style Neu_Window", "Independent top-level window"});
    menu->setItems("Themes", {"Neu_Theme::Light", "Neu_Theme::Dark", "Neu_Theme::BlueGlass"});
    main_win.add(menu);

    auto dialog_info = std::make_shared<Neu_Multilinetextbox>(Neu_Layout{25, 25, 420, 110, 1.0f, 450, 150});
    dialog_info->setText("This is a dialog-style Neu_Window.\nIt uses the same function pointer callback model.\nClose it through the window manager button.");
    dialog.add(dialog_info);

    auto dialog_button = std::make_shared<Neu_Button>(Neu_Layout{25, 160, 220, 44, 1.0f, 260, 60});
    dialog_button->setText("Dialog Button");
    Neu_Callbacks dialog_cb;
    dialog_cb.onClick = [](Neu_Control* sender, void*) {
        sender->setText("Clicked");
        sender->requestRedraw();
    };
    dialog_button->setCallbacks(dialog_cb);
    dialog.add(dialog_button);

    main_win.show();
    app.run();
    return 0;
}
