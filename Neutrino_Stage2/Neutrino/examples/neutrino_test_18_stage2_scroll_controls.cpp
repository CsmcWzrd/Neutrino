#include "common/Neu_TestCommon.hpp"

using namespace neutrino;
using namespace neutrino_tests;

int main(int argc, char** argv)
{
    Neu_Application app;
    if (!start_app(app, argc, argv)) {
        return 1;
    }

    Neu_Window win(app, 1100, 720, "Neutrino Test 18 - Stage2 Scroll Window with New Controls");
    apply_test_window_defaults(win);
    if (!win.create()) {
        return 1;
    }

    auto status = add_status(win, "ScrollWindow containing Stage2 controls. Contents must be clipped to the viewport.");

    auto scroll = std::make_shared<Neu_ScrollWindow>(Neu_Layout{36, 82, 1010, 560});
    scroll->setContentSize(1500, 980);
    scroll->setHintText("ScrollWindow clips all child controls to the viewport on Linux and Windows.");

    for (int i = 0; i < 8; ++i) {
        auto cb = std::make_shared<Neu_CheckBox>(Neu_Layout{58, 108 + i * 44, 360, 34});
        cb->setText("Scrollable checkbox row " + std::to_string(i + 1));
        cb->setChecked((i % 2) == 0);
        cb->setCallbacks(click_callbacks(status.get()));
        scroll->add(cb);
    }

    auto progress = std::make_shared<Neu_ProgressBar>(Neu_Layout{480, 110, 520, 36});
    progress->setProgress(0.58f);
    progress->setText("Progress inside scroll window 58%");
    scroll->add(progress);

    auto slider = std::make_shared<Neu_Slider>(Neu_Layout{480, 176, 520, 48});
    slider->setRange(0, 500);
    slider->setValue(340);
    scroll->add(slider);

    auto tabs = std::make_shared<Neu_TabView>(Neu_Layout{480, 258, 840, 280});
    auto page = std::make_shared<Neu_Placement>(Neu_Layout{500, 305, 780, 190});
    auto label = std::make_shared<Neu_MultilineLabel>(Neu_Layout{520, 325, 690, 130});
    label->setBorderVisible(true);
    label->setWordWrap(true);
    label->setText("This tabbed content is intentionally inside a scroll window. It should not draw outside the viewport even before scrolling or while scrolled.");
    page->add(label);
    tabs->addTab("Clipping", page);
    auto page2 = std::make_shared<Neu_Placement>(Neu_Layout{500, 305, 780, 190});
    auto innerList = std::make_shared<Neu_Listbox>(Neu_Layout{520, 320, 460, 140});
    innerList->setItems(many_items("Nested selectable item", 25));
    page2->add(innerList);
    tabs->addTab("Nested List", page2);
    scroll->add(tabs);

    auto rich = std::make_shared<Neu_ReadOnlyRichText>(Neu_Layout{58, 490, 780, 350});
    rich->setIconList({"assets/icons/sample_icon.bmp", "assets/icons/menu_icon.bmp", "assets/icons/save_icon.bmp"});
    rich->setLabelSpacing(12);
    rich->setLabelLineSpacing(8);
    for (int i = 0; i < 18; ++i) {
        rich->addMultilineLabel(std::string(i % 3 == 0 ? "## " : "# ") + "ReadOnlyRichText row " + std::to_string(i + 1) + " stays clipped while the outer scroll window moves. Escaped hash: \\# visible hash.");
    }
    scroll->add(rich);

    win.add(scroll);

    auto note = std::make_shared<Neu_MultilineLabel>(Neu_Layout{36, 654, 1010, 42});
    note->setText("Use mouse wheel/scrollbars. This test stresses Stage2 clipping with new controls nested inside a scroll window.");
    note->setTextTruncation(true);
    win.add(note);

    win.show();
    app.run();
    return 0;
}
