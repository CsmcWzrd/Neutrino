#include "common/Neu_TestCommon.hpp"

using namespace neutrino;
using namespace neutrino_tests;

int main(int argc, char** argv)
{
    Neu_Application app;
    if (!start_app(app, argc, argv)) {
        return 1;
    }

    Neu_Window win(app, 980, 680, "Neutrino Test 14 - Stage2 Selection Controls");
    apply_test_window_defaults(win);
    if (!win.create()) {
        return 1;
    }

    auto status = add_status(win, "Selection controls: checkbox, radio button, toggle button, group box, separator, link label.");

    auto group = std::make_shared<Neu_GroupBox>(Neu_Layout{28, 80, 430, 240});
    group->setText("Selection group");
    group->setHintText("Neu_GroupBox is a fixed-layout container with a titled border.");

    auto checkA = std::make_shared<Neu_CheckBox>(Neu_Layout{56, 122, 350, 32});
    checkA->setText("Enable software double buffering");
    checkA->setChecked(true);
    checkA->setCallbacks(click_callbacks(status.get()));
    checkA->setHintText("Neu_CheckBox toggles a boolean value with function-pointer callbacks.");
    group->add(checkA);

    auto checkB = std::make_shared<Neu_CheckBox>(Neu_Layout{56, 162, 350, 32});
    checkB->setText("Enable high-quality anti-aliased fonts");
    checkB->setChecked(true);
    checkB->setCallbacks(click_callbacks(status.get()));
    group->add(checkB);

    auto radioA = std::make_shared<Neu_RadioButton>(Neu_Layout{56, 212, 170, 32});
    radioA->setText("Radio option A");
    radioA->setChecked(true);
    radioA->setCallbacks(click_callbacks(status.get()));
    group->add(radioA);

    auto radioB = std::make_shared<Neu_RadioButton>(Neu_Layout{235, 212, 170, 32});
    radioB->setText("Radio option B");
    radioB->setCallbacks(click_callbacks(status.get()));
    group->add(radioB);

    auto toggle = std::make_shared<Neu_ToggleButton>(Neu_Layout{56, 262, 350, 36});
    toggle->setText("ToggleButton active state");
    toggle->setCallbacks(click_callbacks(status.get()));
    toggle->setHintText("Neu_ToggleButton uses the same click callback model as Neu_Button.");
    group->add(toggle);
    win.add(group);

    auto sep = std::make_shared<Neu_Separator>(Neu_Layout{492, 92, 2, 520});
    sep->setVertical(true);
    win.add(sep);

    auto list = std::make_shared<Neu_Listbox>(Neu_Layout{530, 95, 370, 210});
    list->setItems(many_items("Ctrl-click and Shift-click selectable list item", 40));
    list->setMultiSelect(true);
    list->setAutoScroll(true);
    list->setCallbacks(selection_callbacks(status.get()));
    list->setHintText("List box supports Ctrl-click multi-select and Shift-click range selection.");
    win.add(list);

    auto combo = std::make_shared<Neu_ComboBox>(Neu_Layout{530, 330, 370, 42});
    combo->setItems({"Combo option 1", "Combo option 2", "Combo option 3", "Combo option 4", "Combo option 5"});
    combo->setMultiSelect(true);
    combo->setCallbacks(selection_callbacks(status.get()));
    combo->setHintText("ComboBox uses the same selectable list model and remains fixed-position.");
    win.add(combo);

    auto link = std::make_shared<Neu_LinkLabel>(Neu_Layout{530, 398, 370, 36});
    link->setText("Neu_LinkLabel clickable hyperlink-style label");
    link->setCallbacks(click_callbacks(status.get()));
    link->setHintText("Inspired by SWT Link and hyperlink-style Swing labels.");
    win.add(link);

    auto note = std::make_shared<Neu_MultilineLabel>(Neu_Layout{28, 350, 430, 250});
    note->setWordWrap(true);
    note->setBorderVisible(true);
    note->setTextOffset(10, 14, 10, 14);
    note->setText("This selection-control test is included in Linux Make/CMake/autoconf builds and in the Visual Studio 2022 solution. It validates the new Stage2 controls without introducing a new layout system.");
    win.add(note);

    win.show();
    app.run();
    return 0;
}
