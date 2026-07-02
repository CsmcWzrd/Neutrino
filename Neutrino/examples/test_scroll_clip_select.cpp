#include "Neutrino/Neutrino.hpp"
#include <iostream>
using namespace neutrino;

static Neu_StringTable table;

int main()
{
    for (int r = 0; r < 80; ++r) {
        table.push_back({"Row " + std::to_string(r), "Column with long value that must truncate", std::to_string(r * 17), "0x" + std::to_string(r)});
    }
    Neu_Application app;
    if (!app.open()) {
        std::cerr << "Unable to open display\n";
        return 1;
    }
    Neu_Window window(app, 940, 560, "Neutrino Test - Scroll Clip Select");
    window.create();
    auto scroll = std::make_shared<Neu_ScrollWindow>(Neu_Layout{24, 24, 380, 440});
    scroll->setContentSize(720, 780);
    auto inside = std::make_shared<Neu_MultilineLabel>(Neu_Layout{40, 40, 620, 180});
    inside->setText("This content is inside a scroll window and should not draw outside the window bounds. Drag or wheel the scrollbars to move around the virtual area.");
    scroll->add(inside);
    window.add(scroll);
    auto lv = std::make_shared<Neu_ListView>(Neu_Layout{430, 24, 450, 210});
    lv->bind(&table);
    window.add(lv);
    auto tree = std::make_shared<Neu_TreeView>(Neu_Layout{430, 260, 450, 210});
    tree->bind(&table);
    window.add(tree);
    window.show();
    app.run();
    return 0;
}
