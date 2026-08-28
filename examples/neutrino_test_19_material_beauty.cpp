#include "common/Neu_TestCommon.hpp"

using namespace neutrino;
using namespace neutrino_tests;

struct BeautyThemePayload {
    Neu_Window* window{nullptr};
    Neu_Theme theme{};
};

static std::vector<BeautyThemePayload> g_payloads;

static void apply_theme_payload(Neu_Control*, void* user_data)
{
    auto* payload = static_cast<BeautyThemePayload*>(user_data);
    if (payload && payload->window) {
        payload->window->setTheme(payload->theme);
        payload->window->requestRedraw();
    }
}

static void add_theme_button(Neu_Window& win,
                             const std::string& caption,
                             const Neu_Theme& theme,
                             int x,
                             int y,
                             const std::string& hint)
{
    auto button = std::make_shared<Neu_Button>(Neu_Layout{x, y, 240, 48});
    button->setText(caption);
    button->setIconBmp("assets/icons/button_icon.bmp");
    button->setHintText(hint);
    g_payloads.push_back(BeautyThemePayload{&win, theme});
    Neu_Callbacks cb{};
    cb.onClick = apply_theme_payload;
    cb.userData = &g_payloads.back();
    button->setCallbacks(cb);
    win.add(button);
}

int main(int argc, char** argv)
{
    Neu_Application app;
    if (!start_app(app, argc, argv)) {
        return 1;
    }

    Neu_Window win(app, 1180, 760, "Neutrino Test 19 - Material Beauty Theme, Gradients, Corners, AA");
    Neu_Theme material = Neu_Theme::MaterialDark();
    material.gradientControls = true;
    material.highlight = {72, 92, 118, 255};
    material.focus = {28, 96, 168, 255};
    material.controlGradientTop = {68, 72, 84, 255};
    material.controlGradientBottom = {18, 20, 26, 255};
    material.antiAliasMode = Neu_AntiAliasMode::SSAA;
    material.antiAliasSamples = 4;
    material.setDefaultEdgeCorners();
    win.setTheme(material);
    win.setMultiStageDoubleBuffering(true);
    if (!win.create()) {
        return 1;
    }

    auto title = std::make_shared<Neu_Label>(Neu_Layout{24, 20, 1030, 54});
    title->setText("Material-dark is now the default: gradients, theme highlight/focus colors, top-left and bottom-right edge corners, and theme-selected antialiasing.");
    title->setBorderVisible(true);
    title->setWordWrap(true);
    title->setTextOffset(10, 18, 8, 18);
    win.add(title);

    g_payloads.clear();
    g_payloads.reserve(8);

    Neu_Theme rounded = material;
    rounded.setRoundedCorners();
    rounded.focus = {18, 72, 146, 255};

    Neu_Theme sharp = material;
    sharp.setAllCorners(Neu_CornerStyle::EdgeCorner);
    sharp.edgeSize = 12;
    sharp.focus = {80, 52, 12, 255};
    sharp.highlight = {94, 76, 38, 255};
    sharp.controlGradientTop = {94, 72, 40, 255};
    sharp.controlGradientBottom = {28, 22, 16, 255};

    Neu_Theme mixed = material;
    mixed.setCornerStyles(Neu_CornerStyle::RoundedCorner,
                          Neu_CornerStyle::EdgeCorner,
                          Neu_CornerStyle::EdgeCorner,
                          Neu_CornerStyle::RoundedCorner);
    mixed.focus = {88, 34, 126, 255};
    mixed.highlight = {88, 62, 118, 255};
    mixed.antiAliasMode = Neu_AntiAliasMode::MSAA;
    mixed.antiAliasSamples = 3;

    add_theme_button(win, "Default edge material", material, 24, 98,
                     "Default top-left-edge-corner and bottom-right-edge-corner with SSAA.");
    add_theme_button(win, "Rounded-corner material", rounded, 288, 98,
                     "Changes the theme to rounded-corner on every corner.");
    add_theme_button(win, "All edge corners", sharp, 552, 98,
                     "Changes every corner to edge-corner and uses amber focus/highlight colors.");
    add_theme_button(win, "Mixed corner theme", mixed, 816, 98,
                     "Demonstrates top-right-edge-corner and bottom-left-edge-corner with MSAA.");

    auto box = std::make_shared<Neu_Textbox>(Neu_Layout{24, 176, 500, 42});
    box->setText("Textbox inherits the themed surface, focus color, and antialiased edge corners.");
    box->setHintText("The focus color is theme.focus, normally a darker companion of the accent color.");
    win.add(box);

    auto combo = std::make_shared<Neu_ComboBox>(Neu_Layout{552, 176, 260, 42});
    combo->setItems({"DAA default antialiasing", "MSAA multi-sampling", "SSAA super-sampling"});
    combo->setHintText("Theme antiAliasMode may be DAA, MSAA, or SSAA.");
    win.add(combo);

    auto progress = std::make_shared<Neu_ProgressBar>(Neu_Layout{840, 176, 250, 42});
    progress->setProgress(0.72f);
    progress->setText("Gradient Progress 72%");
    progress->setHintText("Progress and value controls retain the same fixed location layout and themed rendering.");
    win.add(progress);

    auto panel = std::make_shared<Neu_ScrollWindow>(Neu_Layout{24, 250, 1066, 360});
    panel->setContentSize(1400, 740);
    panel->setHintText("ScrollWindow clipping remains the same, but child controls draw with the active themed surface.");
    for (int i = 0; i < 16; ++i) {
        auto b = std::make_shared<Neu_Button>(Neu_Layout{24 + (i % 4) * 330, 24 + (i / 4) * 88, 285, 58});
        b->setText("Themed button " + std::to_string(i + 1));
        b->setIconBmp((i % 2) ? "assets/icons/menu_icon.bmp" : "assets/icons/save_icon.bmp");
        b->setHintText("Gradient, highlight, focus, and corner style are coming from the active Neu_Theme.");
        panel->add(b);
    }
    win.add(panel);

    auto footer = std::make_shared<Neu_MultilineLabel>(Neu_Layout{24, 632, 1066, 82});
    footer->setBorderVisible(true);
    footer->setTextOffset(10, 18, 10, 18);
    footer->setText("Theme fields: gradientControls, controlGradientTop, controlGradientBottom, highlight, focus, topLeftCorner/topRightCorner/bottomLeftCorner/bottomRightCorner, edgeSize, antiAliasMode, and antiAliasSamples. No new layouting scheme was introduced.");
    win.add(footer);

    win.show();
    app.run();
    return 0;
}
