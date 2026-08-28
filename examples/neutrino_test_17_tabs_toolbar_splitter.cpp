#include "common/Neu_TestCommon.hpp"

using namespace neutrino;
using namespace neutrino_tests;

int main(int argc, char** argv)
{
    Neu_Application app;
    if (!start_app(app, argc, argv)) {
        return 1;
    }

    Neu_Window win(app, 1120, 790, "Neutrino Test 17 - Stage2 Tabs, Toolbar and Splitter");
    apply_test_window_defaults(win);
    if (!win.create()) {
        return 1;
    }

    auto status = add_status(win, "Tabs, toolbar, vertical splitter and horizontal splitter. Drag sashes to verify minimum pane size and clipping.");

    auto toolbar = std::make_shared<Neu_ToolBar>(Neu_Layout{28, 72, 1040, 54});
    toolbar->setHintText("Neu_ToolBar is a fixed-position placement container for menu/flat buttons.");
    const char* names[] = {"New", "Open", "Save", "Build", "Run"};
    for (int i = 0; i < 5; ++i) {
        auto item = std::make_shared<Neu_FlatButton>(Neu_Layout{42 + i * 112, 82, 96, 34});
        item->setText(names[i]);
        item->setIconBmp(i == 2 ? "assets/icons/save_icon.bmp" : "assets/icons/button_icon.bmp");
        item->setCallbacks(click_callbacks(status.get()));
        toolbar->add(item);
    }
    win.add(toolbar);

    auto tabs = std::make_shared<Neu_TabView>(Neu_Layout{28, 148, 1040, 260});
    tabs->setHintText("Neu_TabView mirrors Swing JTabbedPane and SWT TabFolder without adding a new layouting scheme.");

    auto page1 = std::make_shared<Neu_Placement>(Neu_Layout{42, 190, 1000, 190});
    auto p1label = std::make_shared<Neu_MultilineLabel>(Neu_Layout{52, 204, 940, 130});
    p1label->setBorderVisible(true);
    p1label->setWordWrap(true);
    p1label->setTextOffset(10, 14, 10, 14);
    p1label->setText("First tab: a fixed-location placement page with wrapped text and clipped label bounds.");
    page1->add(p1label);

    auto page2 = std::make_shared<Neu_Placement>(Neu_Layout{42, 190, 1000, 190});
    auto pageButton = std::make_shared<Neu_Button>(Neu_Layout{62, 214, 220, 42});
    pageButton->setText("Button on second tab");
    pageButton->setCallbacks(click_callbacks(status.get()));
    page2->add(pageButton);
    auto pageInput = std::make_shared<Neu_Textbox>(Neu_Layout{62, 276, 360, 40});
    pageInput->setText("Tab page textbox");
    pageInput->setCallbacks(text_callbacks(status.get()));
    page2->add(pageInput);

    auto page3 = std::make_shared<Neu_Placement>(Neu_Layout{42, 190, 1000, 190});
    auto pageList = std::make_shared<Neu_Listbox>(Neu_Layout{62, 205, 360, 145});
    pageList->setItems(many_items("Tabbed list row", 30));
    pageList->setCallbacks(selection_callbacks(status.get()));
    page3->add(pageList);

    tabs->addTab("Page One", page1);
    tabs->addTab("Input Page", page2);
    tabs->addTab("List Page", page3);
    win.add(tabs);

    auto splitter = std::make_shared<Neu_Splitter>(Neu_Layout{28, 438, 1040, 150});
    splitter->setMinimumPaneWidth(140);
    splitter->setSashSize(8);
    splitter->setSplitPosition(500);
    splitter->setHintText("Vertical Neu_Splitter: drag the sash across the fixed child starts. The child controls must be clipped, not overdrawn.");
    auto left = std::make_shared<Neu_MultilineLabel>(Neu_Layout{42, 452, 420, 108});
    left->setBorderVisible(true);
    left->setWordWrap(true);
    left->setText("Left fixed pane: drag the splitter bar. This is a visual sash; no new layout scheme is introduced.");
    splitter->add(left);
    auto right = std::make_shared<Neu_MultilineLabel>(Neu_Layout{455, 452, 580, 108});
    right->setBorderVisible(true);
    right->setWordWrap(true);
    right->setText("Right fixed pane: clipping and redraw should work on Windows and Linux.");
    splitter->add(right);
    win.add(splitter);

    auto horizontalSplitter = std::make_shared<Neu_Splitter>(Neu_Layout{28, 610, 1040, 120});
    horizontalSplitter->setVertical(false);
    horizontalSplitter->setMinimumPaneHeight(42);
    horizontalSplitter->setSashSize(8);
    horizontalSplitter->setSplitPosition(78);
    horizontalSplitter->setHintText("Horizontal Neu_Splitter: drag up/down. The top and bottom controls stay clipped when the sash crosses them.");
    auto topPane = std::make_shared<Neu_MultilineLabel>(Neu_Layout{42, 620, 980, 62});
    topPane->setBorderVisible(true);
    topPane->setWordWrap(true);
    topPane->setText("Top fixed pane: horizontal splitter minimum height prevents collapsing below 42 px. Drag the sash through this text to test clipping.");
    horizontalSplitter->add(topPane);
    auto bottomPane = std::make_shared<Neu_MultilineLabel>(Neu_Layout{42, 682, 980, 38});
    bottomPane->setBorderVisible(true);
    bottomPane->setWordWrap(true);
    bottomPane->setText("Bottom fixed pane: remains clipped when the sash moves past its start.");
    horizontalSplitter->add(bottomPane);
    win.add(horizontalSplitter);

    win.show();
    app.run();
    return 0;
}
