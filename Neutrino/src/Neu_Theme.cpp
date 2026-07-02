#include "Neutrino/Neutrino.hpp"

namespace neutrino {

Neu_Theme Neu_Theme::Light()
{
    return Neu_Theme{};
}

Neu_Theme Neu_Theme::Dark()
{
    Neu_Theme theme;
    theme.background = {24, 28, 34, 255};
    theme.glass = {42, 48, 58, 230};
    theme.border = {90, 105, 125, 255};
    theme.text = {235, 240, 248, 255};
    theme.accent = {105, 160, 245, 255};
    theme.hover = {55, 64, 78, 255};
    theme.pressed = {70, 85, 105, 255};
    theme.shadow = {0, 0, 0, 125};
    theme.hintBackground = {52, 58, 68, 250};
    theme.hintBorder = {145, 165, 200, 255};
    return theme;
}

Neu_Theme Neu_Theme::BlueGlass()
{
    Neu_Theme theme;
    theme.background = {222, 237, 255, 255};
    theme.glass = {240, 248, 255, 230};
    theme.border = {100, 150, 210, 255};
    theme.text = {10, 35, 70, 255};
    theme.accent = {20, 100, 210, 255};
    theme.shadow = {30, 64, 110, 95};
    return theme;
}

} // namespace neutrino
