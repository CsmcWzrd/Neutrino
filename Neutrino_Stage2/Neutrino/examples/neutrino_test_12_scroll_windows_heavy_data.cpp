#include "common/Neu_TestCommon.hpp"

using namespace neutrino;
using namespace neutrino_tests;

int main(int argc, char** argv)
{
    Neu_Application app;
    if (!start_app(app, argc, argv)) {
        return 1;
    }

    Neu_Window win(app, 1120, 740, "Neutrino Test 12 - ScrollWindow, ScrollBars, Heavy Data Auto Scroll");
    apply_test_window_defaults(win);
    if (!win.create()) {
        return 1;
    }

    auto status = add_status(win, "Heavy data test: scroll windows, listbox, listview, and explicit scrollbars.");
    Neu_Callbacks selection = selection_callbacks(status.get());

    static Neu_StringTable table;
    if (table.empty()) {
        table.push_back({"Row", "Name", "Integer", "Boolean", "Hex", "Double"});
        for (int i = 0; i < 220; ++i) {
            std::ostringstream row;
            row << i;
            std::ostringstream name;
            name << "Large table item " << i;
            std::ostringstream integer;
            integer << (i * 17);
            std::ostringstream hex;
            hex << "0x" << std::hex << (i * 31 + 255);
            std::ostringstream dbl;
            dbl << std::dec << (i * 0.125);
            table.push_back({row.str(), name.str(), integer.str(), (i % 2) ? "1:true" : "0:false", hex.str(), dbl.str()});
        }
    }

    auto scroll = std::make_shared<Neu_ScrollWindow>(Neu_Layout{35, 85, 500, 500, 1.0f, 560, 600});
    scroll->setContentSize(760, 1050);
    scroll->setHintText("Neu_ScrollWindow is a scrollable container. Use mouse wheel to adjust its scroll offsets.");

    for (int i = 0; i < 18; ++i) {
        auto item = std::make_shared<Neu_Label>(Neu_Layout{60, 125 + i * 50, 360, 34, 1.0f, 420, 44});
        std::ostringstream text;
        text << "ScrollWindow child label " << (i + 1);
        item->setText(text.str());
        item->setIconBmp((i % 2) ? "assets/icons/menu_icon.bmp" : "assets/icons/sample_icon.bmp");
        scroll->add(item);
    }
    win.add(scroll);

    auto list_view = std::make_shared<Neu_ListView>(Neu_Layout{570, 85, 500, 310, 1.0f, 560, 380});
    list_view->bind(&table);
    list_view->setAutoScroll(true);
    list_view->setCallbacks(selection);
    list_view->setHintText("Large Neu_ListView with hundreds of rows and horizontal/vertical virtual size.");
    win.add(list_view);

    auto list = std::make_shared<Neu_Listbox>(Neu_Layout{570, 425, 360, 160, 1.0f, 420, 220});
    list->setAutoScroll(true);
    list->setItems(many_items("Large list item", 180));
    list->setCallbacks(selection);
    win.add(list);

    auto vbar = std::make_shared<Neu_ScrollBar>(Neu_Layout{950, 425, 28, 160, 1.0f, 40, 220});
    vbar->setVertical(true);
    vbar->setRange(180, 16, 45);
    vbar->setHintText("Standalone vertical Neu_ScrollBar.");
    win.add(vbar);

    auto hbar = std::make_shared<Neu_ScrollBar>(Neu_Layout{570, 605, 360, 28, 1.0f, 420, 40});
    hbar->setVertical(false);
    hbar->setRange(1000, 320, 150);
    hbar->setHintText("Standalone horizontal Neu_ScrollBar.");
    win.add(hbar);

    win.show();
    app.run();
    return 0;
}
