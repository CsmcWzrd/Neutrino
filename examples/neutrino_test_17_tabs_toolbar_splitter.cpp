#include "common/Neu_TestCommon.hpp"

using namespace neutrino;
using namespace neutrino_tests;

namespace {

std::shared_ptr<Neu_Placement> page_with_label(int x, int y, int w, int h, const std::string& text)
{
    auto page = std::make_shared<Neu_Placement>(Neu_Layout{x, y, w, h});
    auto label = std::make_shared<Neu_MultilineLabel>(Neu_Layout{x, y, w, h});
    label->setBorderVisible(true);
    label->setWordWrap(true);
    label->setTextOffset(8, 10, 8, 10);
    label->setText(text);
    page->add(label);
    return page;
}

void add_demo_tabs(Neu_Window& win,
                   int x,
                   int y,
                   int w,
                   int h,
                   Neu_TabPosition position,
                   const std::string& label,
                   int selected)
{
    auto tabs = std::make_shared<Neu_TabView>(Neu_Layout{x, y, w, h});
    tabs->setTabPosition(position);
    tabs->setTabBarThickness(position == Neu_TabPosition::Left || position == Neu_TabPosition::Right ? 116 : 34);
    tabs->setMinimumTabButtonSize(position == Neu_TabPosition::Left || position == Neu_TabPosition::Right ? 46 : 84);
    tabs->setHintText("Neu_TabView tab buttons use the same rounded button shape. Non-selected tabs use a lighter/darker theme highlight variant.");

    int px = x + 14;
    int py = y + 44;
    int pw = w - 28;
    int ph = h - 58;
    if (position == Neu_TabPosition::Bottom) {
        py = y + 12;
    } else if (position == Neu_TabPosition::Left) {
        px = x + 128;
        py = y + 14;
        pw = w - 142;
        ph = h - 28;
    } else if (position == Neu_TabPosition::Right) {
        px = x + 14;
        py = y + 14;
        pw = w - 142;
        ph = h - 28;
    }

    tabs->addTab("Main", page_with_label(px, py, pw, ph, label + ": selected tab button uses theme.highlight and focus border."));
    tabs->addTab("Options", page_with_label(px, py, pw, ph, label + ": inactive tab buttons are tinted from theme.highlight, not flat rectangles."));
    tabs->addTab("Log", page_with_label(px, py, pw, ph, label + ": content is clipped away from the tab bar edge."));
    tabs->setSelectedTab(selected);
    win.add(tabs);
}

} // namespace

int main(int argc, char** argv)
{
    Neu_Application app;
    if (!start_app(app, argc, argv)) {
        return 1;
    }

    Neu_Window win(app, 1160, 820, "Neutrino Test 17 - Tab Positions, Toolbar and Splitter");
    apply_test_window_defaults(win);
    if (!win.create()) {
        return 1;
    }

    auto status = add_status(win, "Tab bar button shape + top/bottom/left/right positions, plus splitter minimum-size/clipping regression checks.");

    auto toolbar = std::make_shared<Neu_ToolBar>(Neu_Layout{28, 72, 1088, 54});
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

    add_demo_tabs(win, 28, 146, 530, 160, Neu_TabPosition::Top, "Top tab bar", 0);
    add_demo_tabs(win, 586, 146, 530, 160, Neu_TabPosition::Bottom, "Bottom tab bar", 1);
    add_demo_tabs(win, 28, 324, 530, 160, Neu_TabPosition::Left, "Left tab bar", 2);
    add_demo_tabs(win, 586, 324, 530, 160, Neu_TabPosition::Right, "Right tab bar", 0);

    auto splitter = std::make_shared<Neu_Splitter>(Neu_Layout{28, 506, 530, 130});
    splitter->setMinimumPaneWidth(120);
    splitter->setSashSize(8);
    splitter->setSplitPosition(250);
    splitter->setHintText("Vertical Neu_Splitter: drag the sash across the fixed child starts. The child controls must be clipped, not overdrawn.");
    auto left = std::make_shared<Neu_MultilineLabel>(Neu_Layout{42, 520, 220, 92});
    left->setBorderVisible(true);
    left->setWordWrap(true);
    left->setText("Left fixed pane: minimum width and clipping are retained after the tab changes.");
    splitter->add(left);
    auto right = std::make_shared<Neu_MultilineLabel>(Neu_Layout{260, 520, 270, 92});
    right->setBorderVisible(true);
    right->setWordWrap(true);
    right->setText("Right fixed pane: drag the sash across this label to verify clipping.");
    splitter->add(right);
    win.add(splitter);

    auto horizontalSplitter = std::make_shared<Neu_Splitter>(Neu_Layout{586, 506, 530, 130});
    horizontalSplitter->setVertical(false);
    horizontalSplitter->setMinimumPaneHeight(42);
    horizontalSplitter->setSashSize(8);
    horizontalSplitter->setSplitPosition(70);
    horizontalSplitter->setHintText("Horizontal Neu_Splitter: drag up/down. The top and bottom controls stay clipped when the sash crosses them.");
    auto topPane = std::make_shared<Neu_MultilineLabel>(Neu_Layout{600, 518, 500, 54});
    topPane->setBorderVisible(true);
    topPane->setWordWrap(true);
    topPane->setText("Top pane: min height keeps it usable while clipping text correctly.");
    horizontalSplitter->add(topPane);
    auto bottomPane = std::make_shared<Neu_MultilineLabel>(Neu_Layout{600, 574, 500, 46});
    bottomPane->setBorderVisible(true);
    bottomPane->setWordWrap(true);
    bottomPane->setText("Bottom pane: remains clipped when sash moves past its start.");
    horizontalSplitter->add(bottomPane);
    win.add(horizontalSplitter);

    auto note = std::make_shared<Neu_MultilineLabel>(Neu_Layout{28, 660, 1088, 88});
    note->setBorderVisible(true);
    note->setWordWrap(true);
    note->setTextOffset(8, 12, 8, 12);
    note->setText("Verification target: all four tab positions should show rounded, button-shaped tabs. Selected tabs use the normal theme highlight/focus colors; inactive tabs use a lighter or darker variant of theme.highlight so they still read as tab buttons. The page body must stay clipped away from the tab strip.");
    win.add(note);

    win.show();
    app.run();
    return 0;
}
