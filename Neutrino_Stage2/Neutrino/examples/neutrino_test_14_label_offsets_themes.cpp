#include "common/Neu_TestCommon.hpp"

using namespace neutrino;
using namespace neutrino_tests;

static void apply_theme_by_slot(Neu_Window& win, int slot)
{
    const auto names = Neu_Theme::BuiltInThemeNames();
    if (!names.empty()) {
        win.setTheme(Neu_Theme::BuiltInThemeByName(names[static_cast<size_t>(slot) % names.size()]));
    }
}

int main(int argc, char** argv)
{
    Neu_Application app;
    if (!start_app(app, argc, argv)) {
        return 1;
    }

    Neu_Window win(app, 1180, 760, "Neutrino Test 17 - Stage2 Label Offsets and 24+ Themes");
    apply_test_window_defaults(win);
    if (!win.create()) {
        return 1;
    }

    auto status = add_status(win, "Stage2: label text offsets/insets, optional borders, wrapping/truncation, and theme gallery.");

    const auto names = Neu_Theme::BuiltInThemeNames();
    int x = 24;
    int y = 78;
    int index = 0;
    for (const auto& name : names) {
        auto swatch = std::make_shared<Neu_Button>(Neu_Layout{x, y, 160, 34});
        swatch->setText(name);
        swatch->setTextTruncation(true);
        swatch->setHintText("Click to apply this built-in Neutrino theme. Stage2 includes Windows-inspired themes such as win95, winxp, win10 and win11.");
        const int slot = index;
        Neu_Callbacks cb = click_callbacks(status.get());
        cb.onClick = [](Neu_Control* sender, void* user_data) {
            auto* pair = static_cast<std::pair<Neu_Window*, int>*>(user_data);
            apply_theme_by_slot(*pair->first, pair->second);
            sender->requestRedraw();
        };
        auto* data = new std::pair<Neu_Window*, int>(&win, slot);
        cb.userData = data;
        swatch->setCallbacks(cb);
        win.add(swatch);
        x += 170;
        if (x > 980) {
            x = 24;
            y += 42;
        }
        ++index;
        if (index >= 24) {
            break;
        }
    }

    auto offsetLabel = std::make_shared<Neu_Label>(Neu_Layout{24, 310, 520, 58});
    offsetLabel->setText("Offset label: top=10 right=20 bottom=6 left=30; text must not protrude outside left edge.");
    offsetLabel->setBorderVisible(true);
    offsetLabel->setTextOffset(10, 20, 6, 30);
    offsetLabel->setTextTruncation(true);
    offsetLabel->setHintText("This test is for Windows and Linux clipping. The text begins after the configured left offset.");
    win.add(offsetLabel);

    auto wrappedLabel = std::make_shared<Neu_Label>(Neu_Layout{570, 310, 560, 88});
    wrappedLabel->setText("Word wrapped label: this intentionally long label should wrap inside the rectangular boundary and should never draw outside the control.");
    wrappedLabel->setBorderVisible(true);
    wrappedLabel->setWordWrap(true);
    wrappedLabel->setTextOffset(8, 12, 8, 14);
    win.add(wrappedLabel);

    auto clippedLabel = std::make_shared<Neu_Label>(Neu_Layout{24, 420, 520, 42});
    clippedLabel->setText("Truncated label with a very long left-to-right sentence that must be clipped to the label width.");
    clippedLabel->setBorderVisible(true);
    clippedLabel->setWordWrap(false);
    clippedLabel->setTextTruncation(true);
    clippedLabel->setTextOffset(6, 12, 6, 12);
    win.add(clippedLabel);

    auto richLabel = std::make_shared<Neu_Label>(Neu_Layout{570, 420, 560, 58});
    richLabel->setBorderVisible(true);
    richLabel->setTextOffset(6, 12, 6, 12);
    Neu_TextFragment a; a.text = "Bold "; a.bold = true;
    Neu_TextFragment b; b.text = "Italic "; b.italic = true; b.useFontColor = true; b.fontColor = {180, 60, 60, 255};
    Neu_TextFragment c; c.text = "Underline "; c.underline = true; c.useHighlightColor = true; c.highlightColor = {255, 240, 120, 180};
    Neu_TextFragment d; d.text = "Strike"; d.strikethrough = true;
    richLabel->addRichTextFragment(a);
    richLabel->addRichTextFragment(b);
    richLabel->addRichTextFragment(c);
    richLabel->addRichTextFragment(d);
    richLabel->setHintText("Minimum rich label formatting: bold, italic, underline, strike, font/highlight color.");
    win.add(richLabel);

    auto multi = std::make_shared<Neu_MultilineLabel>(Neu_Layout{24, 500, 1106, 178});
    multi->setText("Multiline label wraps by default.\nThis long second line is meant to verify that word wrapping keeps text inside the control on both Windows GDI and Linux X11/Xft paths.\nBorder is enabled here only through a flag; labels remain borderless by default.");
    multi->setBorderVisible(true);
    multi->setWordWrap(true);
    multi->setTextOffset(10, 16, 10, 16);
    multi->setIconBmp("assets/icons/menu_icon.bmp");
    win.add(multi);

    win.show();
    app.run();
    return 0;
}
