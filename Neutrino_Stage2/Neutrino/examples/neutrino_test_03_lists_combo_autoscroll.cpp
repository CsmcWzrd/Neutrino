#include "common/Neu_TestCommon.hpp"

using namespace neutrino;
using namespace neutrino_tests;

int main(int argc, char** argv)
{
    Neu_Application app;
    if (!start_app(app, argc, argv)) {
        return 1;
    }

    Neu_Window win(app, 960, 640, "Neutrino Test 03 - Listbox, ComboBox, Auto Scroll");
    apply_test_window_defaults(win);
    if (!win.create()) {
        return 1;
    }

    auto status = add_status(win, "Select rows or use the mouse wheel over the data-heavy controls.");
    Neu_Callbacks selection = selection_callbacks(status.get());

    auto list = std::make_shared<Neu_Listbox>(Neu_Layout{35, 85, 390, 440, 1.0f, 460, 520});
    list->setAutoScroll(true);
    list->setItems(many_items("Neu_Listbox heavy row", 120));
    list->setHintText("A large Neu_Listbox. Auto-scroll is enabled and the scrollbar appears when virtual content exceeds the control size.");
    list->setCallbacks(selection);
    win.add(list);

    auto combo = std::make_shared<Neu_ComboBox>(Neu_Layout{470, 85, 390, 440, 1.0f, 460, 520});
    combo->setAutoScroll(true);
    combo->setItems(many_items("Neu_ComboBox choice", 100));
    combo->setHintText("Neu_ComboBox shares list selection and scroll behavior with the listbox in this simplified framework.");
    combo->setCallbacks(selection);
    win.add(combo);

    win.show();
    app.run();
    return 0;
}
