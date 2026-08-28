#include "common/Neu_TestCommon.hpp"

using namespace neutrino;
using namespace neutrino_tests;

static Neu_Card make_card(int level,
                          const std::wstring& title,
                          const std::wstring& detail,
                          const std::string& icon,
                          Neu_CardIconPosition iconPosition = Neu_CardIconPosition::Beginning)
{
    Neu_Card card;
    card.levelIndex = level;
    card.items.emplace_back(title, icon, iconPosition);
    card.items.emplace_back(detail);
    return card;
}

int main(int argc, char** argv)
{
    Neu_Application app;
    if (!start_app(app, argc, argv)) {
        return 1;
    }

    Neu_Window win(app, 1100, 760, "Neutrino Test 22 - Neu_Cards list control");
    apply_test_window_defaults(win);
    if (!win.create()) {
        return 1;
    }

    auto status = add_status(win, "Neu_Cards: level indentation, card items, begin/end icons, simple rich text and X11 repaint safety.");

    auto title = std::make_shared<Neu_Label>(Neu_Layout{760, 18, 310, 92});
    title->setBorderVisible(true);
    title->setWordWrap(true);
    title->setTextOffset(10, 12, 10, 12);
    title->setText("Each Neu_Card has a levelIndex. The card is pushed right by levelIndex * cardLevelOffset pixels. Item 0 supports icon and simple rich text; remaining items are text rows.");
    win.add(title);

    auto cards = std::make_shared<Neu_Cards>(Neu_Layout{34, 82, 705, 610});
    cards->setCardLevelOffset(34);
    cards->setCardMinHeight(58);
    cards->setCardSpacing(9);
    cards->setIconSize(22);
    cards->setHintText("Neu_Cards works like a list. Scroll with the mouse wheel; click a card to select it. Simple rich text markers: **bold**, //italic//, __underline__, ~~strike~~ and `mono`.");
    Neu_Callbacks cb = selection_callbacks(status.get());
    cards->setCallbacks(cb);

    std::vector<Neu_Card> data;
    data.push_back(make_card(0,
                             L"**Root card** with beginning icon and //simple rich text//",
                             L"This card starts at level 0 and uses asset sample_icon.bmp.",
                             "assets/icons/sample_icon.bmp"));
    data.push_back(make_card(1,
                             L"Child card level 1 uses __underlined__ title text",
                             L"The entire card is shifted by one configured card-level offset.",
                             "assets/icons/menu_icon.bmp"));
    data.push_back(make_card(2,
                             L"Nested child level 2 uses `monospace` inside title",
                             L"Only the first item gets the icon. This second item is plain text.",
                             "assets/icons/save_icon.bmp"));
    data.push_back(make_card(1,
                             L"End icon card keeps the icon at the far right",
                             L"This demonstrates Neu_CardIconPosition::End.",
                             "assets/icons/button_icon.bmp",
                             Neu_CardIconPosition::End));
    data.push_back(make_card(0,
                             L"Another root card with ~~strike~~ and **bold** markers",
                             L"All text is stored in std::wstring and converted for the active backend.",
                             "assets/icons/menu_icon.bmp"));

    for (int i = 0; i < 28; ++i) {
        Neu_Card generated;
        generated.levelIndex = i % 4;
        generated.items.emplace_back(L"Generated card **" + std::to_wstring(i + 1) + L"** at level " + std::to_wstring(generated.levelIndex),
                                     (i % 2) ? "assets/icons/save_icon.bmp" : "assets/icons/sample_icon.bmp",
                                     (i % 3 == 0) ? Neu_CardIconPosition::End : Neu_CardIconPosition::Beginning);
        generated.items.emplace_back(L"Additional item row: text-only index 1, no icon is drawn here.");
        if (i % 5 == 0) {
            generated.items.emplace_back(L"Additional item row: index 2 also stays text-only.");
        }
        data.push_back(generated);
    }

    cards->setCards(data);
    win.add(cards);

    auto knobs = std::make_shared<Neu_GroupBox>(Neu_Layout{760, 128, 310, 220});
    knobs->setText("Neu_Cards configuration");
    auto offsetLabel = std::make_shared<Neu_Label>(Neu_Layout{778, 164, 270, 38});
    offsetLabel->setBorderVisible(true);
    offsetLabel->setText("cardLevelOffset = 34 px");
    auto first = std::make_shared<Neu_Button>(Neu_Layout{778, 216, 132, 34});
    first->setText("Select first");
    auto last = std::make_shared<Neu_Button>(Neu_Layout{920, 216, 132, 34});
    last->setText("Select last");
    auto tight = std::make_shared<Neu_Button>(Neu_Layout{778, 268, 132, 34});
    tight->setText("Offset 18");
    auto wide = std::make_shared<Neu_Button>(Neu_Layout{920, 268, 132, 34});
    wide->setText("Offset 48");

    Neu_Callbacks firstCb;
    firstCb.onClick = [](Neu_Control*, void* userData) {
        auto* c = static_cast<Neu_Cards*>(userData);
        if (c) {
            c->setSelectedIndex(0);
        }
    };
    // Keep callbacks simple and static by capturing through userData.
    firstCb.userData = cards.get();
    first->setCallbacks(firstCb);

    Neu_Callbacks lastCb;
    lastCb.onClick = [](Neu_Control*, void* userData) {
        auto* c = static_cast<Neu_Cards*>(userData);
        if (c && !c->cards().empty()) {
            c->setSelectedIndex(static_cast<int>(c->cards().size() - 1));
        }
    };
    lastCb.userData = cards.get();
    last->setCallbacks(lastCb);

    Neu_Callbacks tightCb;
    tightCb.onClick = [](Neu_Control*, void* userData) {
        auto* c = static_cast<Neu_Cards*>(userData);
        if (c) {
            c->setCardLevelOffset(18);
        }
    };
    tightCb.userData = cards.get();
    tight->setCallbacks(tightCb);

    Neu_Callbacks wideCb;
    wideCb.onClick = [](Neu_Control*, void* userData) {
        auto* c = static_cast<Neu_Cards*>(userData);
        if (c) {
            c->setCardLevelOffset(48);
        }
    };
    wideCb.userData = cards.get();
    wide->setCallbacks(wideCb);

    win.add(knobs);
    win.add(offsetLabel);
    win.add(first);
    win.add(last);
    win.add(tight);
    win.add(wide);

    auto notes = std::make_shared<Neu_MultilineLabel>(Neu_Layout{760, 372, 310, 226});
    notes->setBorderVisible(true);
    notes->setWordWrap(true);
    notes->setTextOffset(10, 12, 10, 12);
    notes->setText("Linux/X11 verification focus:\n- No blank X11 window.\n- No continuous refresh loop.\n- Text is drawn once.\n- WM close still works.\n- Neu_Cards scrollbars remain responsive.");
    win.add(notes);

    win.show();
    app.run();
    return 0;
}
