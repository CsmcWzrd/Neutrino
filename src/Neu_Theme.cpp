#include "Neutrino/Neutrino.hpp"
#include <algorithm>

namespace neutrino {

namespace {

static Neu_Color localLighten(Neu_Color color, int amount)
{
    auto add = [amount](uint8_t v) -> uint8_t { return static_cast<uint8_t>(std::min(255, static_cast<int>(v) + amount)); };
    return {add(color.r), add(color.g), add(color.b), color.a};
}

static Neu_Color localDarken(Neu_Color color, int amount)
{
    auto sub = [amount](uint8_t v) -> uint8_t { return static_cast<uint8_t>(std::max(0, static_cast<int>(v) - amount)); };
    return {sub(color.r), sub(color.g), sub(color.b), color.a};
}

static Neu_Theme makeTheme(Neu_Color background,
                           Neu_Color glass,
                           Neu_Color border,
                           Neu_Color text,
                           Neu_Color accent,
                           Neu_Color hover,
                           Neu_Color pressed,
                           Neu_Color shadow,
                           Neu_Color hintBackground,
                           Neu_Color hintBorder,
                           int radius = 12,
                           const std::string& font = "DejaVu Sans:size=10:antialias=true:hinting=true:hintstyle=hintfull:rgba=rgb:lcdfilter=lcddefault")
{
    Neu_Theme theme;
    theme.background = background;
    theme.glass = glass;
    theme.border = border;
    theme.text = text;
    theme.accent = accent;
    theme.hover = hover;
    theme.pressed = pressed;
    theme.highlight = hover;
    theme.focus = localDarken(accent, 56);
    theme.controlGradientTop = localLighten(glass, 24);
    theme.controlGradientBottom = localDarken(glass, 20);
    theme.shadow = shadow;
    theme.hintBackground = hintBackground;
    theme.hintBorder = hintBorder;
    theme.radius = radius;
    theme.edgeSize = std::max(4, std::min(14, radius + 4));
    theme.gradientControls = true;
    theme.setDefaultEdgeCorners();
    theme.antiAliasMode = Neu_AntiAliasMode::DAA;
    theme.antiAliasSamples = 3;
    theme.fontName = font;
    return theme;
}

static std::string lowerName(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    value.erase(std::remove(value.begin(), value.end(), ' '), value.end());
    value.erase(std::remove(value.begin(), value.end(), '_'), value.end());
    value.erase(std::remove(value.begin(), value.end(), '-'), value.end());
    return value;
}

} // namespace

Neu_Theme Neu_Theme::Light()
{
    return makeTheme({245, 248, 252, 255}, {238, 246, 255, 235}, {150, 175, 205, 255}, {20, 28, 38, 255}, {70, 135, 220, 255}, {225, 238, 255, 255}, {190, 215, 250, 255}, {36, 52, 70, 85}, {255, 255, 232, 245}, {118, 132, 72, 255}, 12);
}

Neu_Theme Neu_Theme::Dark()
{
    return makeTheme({24, 28, 34, 255}, {42, 48, 58, 230}, {90, 105, 125, 255}, {235, 240, 248, 255}, {105, 160, 245, 255}, {55, 64, 78, 255}, {70, 85, 105, 255}, {0, 0, 0, 125}, {52, 58, 68, 250}, {145, 165, 200, 255}, 12);
}

Neu_Theme Neu_Theme::BlueGlass()
{
    return makeTheme({222, 237, 255, 255}, {240, 248, 255, 230}, {100, 150, 210, 255}, {10, 35, 70, 255}, {20, 100, 210, 255}, {224, 240, 255, 255}, {184, 215, 250, 255}, {30, 64, 110, 95}, {246, 251, 255, 250}, {95, 135, 190, 255}, 14);
}

Neu_Theme Neu_Theme::Win95()
{
    Neu_Theme t = makeTheme({0, 128, 128, 255}, {192, 192, 192, 255}, {128, 128, 128, 255}, {0, 0, 0, 255}, {0, 0, 128, 255}, {224, 224, 224, 255}, {160, 160, 160, 255}, {70, 70, 70, 85}, {255, 255, 225, 255}, {128, 128, 0, 255}, 0, "DejaVu Sans:size=10:antialias=true");
    t.shadowSize = 3;
    t.shadowOffsetX = 2;
    t.shadowOffsetY = 2;
    return t;
}

Neu_Theme Neu_Theme::WinXP()
{
    return makeTheme({236, 243, 252, 255}, {222, 234, 255, 255}, {59, 97, 156, 255}, {0, 0, 0, 255}, {49, 106, 197, 255}, {255, 238, 194, 255}, {251, 194, 94, 255}, {40, 62, 110, 95}, {255, 255, 225, 255}, {160, 140, 80, 255}, 8, "Tahoma:size=10:antialias=true");
}

Neu_Theme Neu_Theme::Win10()
{
    return makeTheme({243, 243, 243, 255}, {255, 255, 255, 240}, {210, 210, 210, 255}, {32, 32, 32, 255}, {0, 120, 215, 255}, {229, 241, 251, 255}, {204, 228, 247, 255}, {0, 0, 0, 85}, {255, 255, 225, 255}, {120, 120, 80, 255}, 2, "Segoe UI:size=10:antialias=true");
}

Neu_Theme Neu_Theme::Win11()
{
    return makeTheme({249, 249, 249, 255}, {255, 255, 255, 235}, {225, 225, 225, 255}, {32, 32, 32, 255}, {0, 103, 192, 255}, {238, 246, 255, 255}, {213, 234, 255, 255}, {0, 0, 0, 70}, {255, 255, 240, 250}, {120, 120, 90, 255}, 10, "Segoe UI Variable:size=10:antialias=true");
}

Neu_Theme Neu_Theme::ClassicMotif()
{
    return makeTheme({174, 178, 195, 255}, {210, 213, 224, 255}, {93, 98, 114, 255}, {24, 24, 24, 255}, {55, 85, 150, 255}, {226, 228, 236, 255}, {185, 190, 204, 255}, {70, 70, 90, 80}, {255, 255, 220, 255}, {100, 100, 80, 255}, 2);
}

Neu_Theme Neu_Theme::SolarizedLight()
{
    return makeTheme({253, 246, 227, 255}, {238, 232, 213, 235}, {147, 161, 161, 255}, {101, 123, 131, 255}, {38, 139, 210, 255}, {232, 225, 202, 255}, {220, 210, 185, 255}, {88, 110, 117, 70}, {255, 252, 230, 250}, {181, 137, 0, 255}, 10);
}

Neu_Theme Neu_Theme::SolarizedDark()
{
    return makeTheme({0, 43, 54, 255}, {7, 54, 66, 235}, {88, 110, 117, 255}, {131, 148, 150, 255}, {38, 139, 210, 255}, {12, 69, 83, 255}, {20, 83, 98, 255}, {0, 0, 0, 130}, {7, 54, 66, 250}, {181, 137, 0, 255}, 10);
}

Neu_Theme Neu_Theme::Nord()
{
    return makeTheme({46, 52, 64, 255}, {59, 66, 82, 235}, {76, 86, 106, 255}, {236, 239, 244, 255}, {136, 192, 208, 255}, {67, 76, 94, 255}, {94, 129, 172, 255}, {0, 0, 0, 110}, {59, 66, 82, 250}, {129, 161, 193, 255}, 12);
}

Neu_Theme Neu_Theme::Dracula()
{
    return makeTheme({40, 42, 54, 255}, {68, 71, 90, 235}, {98, 114, 164, 255}, {248, 248, 242, 255}, {189, 147, 249, 255}, {80, 82, 105, 255}, {139, 233, 253, 255}, {0, 0, 0, 120}, {68, 71, 90, 250}, {255, 121, 198, 255}, 12);
}

Neu_Theme Neu_Theme::GruvboxLight()
{
    return makeTheme({251, 241, 199, 255}, {235, 219, 178, 235}, {168, 153, 132, 255}, {60, 56, 54, 255}, {69, 133, 136, 255}, {242, 229, 188, 255}, {213, 196, 161, 255}, {80, 73, 69, 80}, {251, 241, 199, 250}, {181, 118, 20, 255}, 8);
}

Neu_Theme Neu_Theme::GruvboxDark()
{
    return makeTheme({40, 40, 40, 255}, {60, 56, 54, 235}, {102, 92, 84, 255}, {235, 219, 178, 255}, {250, 189, 47, 255}, {80, 73, 69, 255}, {104, 157, 106, 255}, {0, 0, 0, 130}, {60, 56, 54, 250}, {214, 93, 14, 255}, 8);
}

Neu_Theme Neu_Theme::HighContrastLight()
{
    return makeTheme({255, 255, 255, 255}, {255, 255, 255, 255}, {0, 0, 0, 255}, {0, 0, 0, 255}, {0, 0, 255, 255}, {220, 235, 255, 255}, {190, 215, 255, 255}, {0, 0, 0, 90}, {255, 255, 210, 255}, {0, 0, 0, 255}, 0);
}

Neu_Theme Neu_Theme::HighContrastDark()
{
    return makeTheme({0, 0, 0, 255}, {0, 0, 0, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 0, 255}, {40, 40, 40, 255}, {70, 70, 70, 255}, {0, 0, 0, 0}, {0, 0, 0, 255}, {255, 255, 0, 255}, 0);
}

Neu_Theme Neu_Theme::UbuntuAubergine()
{
    return makeTheme({48, 10, 36, 255}, {246, 244, 241, 240}, {174, 167, 159, 255}, {45, 45, 45, 255}, {233, 84, 32, 255}, {255, 240, 230, 255}, {238, 177, 151, 255}, {0, 0, 0, 95}, {255, 247, 230, 250}, {150, 80, 40, 255}, 9, "Ubuntu:size=10:antialias=true");
}

Neu_Theme Neu_Theme::KDEBreeze()
{
    return makeTheme({239, 240, 241, 255}, {252, 252, 252, 240}, {189, 195, 199, 255}, {35, 38, 41, 255}, {61, 174, 233, 255}, {227, 244, 255, 255}, {199, 228, 247, 255}, {0, 0, 0, 70}, {255, 255, 230, 250}, {110, 125, 130, 255}, 5, "Noto Sans:size=10:antialias=true");
}

Neu_Theme Neu_Theme::MacAqua()
{
    return makeTheme({236, 238, 241, 255}, {255, 255, 255, 238}, {180, 185, 194, 255}, {30, 30, 30, 255}, {0, 122, 255, 255}, {232, 243, 255, 255}, {207, 229, 255, 255}, {0, 0, 0, 75}, {255, 255, 230, 250}, {130, 130, 90, 255}, 11, "San Francisco:size=10:antialias=true");
}

Neu_Theme Neu_Theme::MaterialLight()
{
    return makeTheme({250, 250, 250, 255}, {255, 255, 255, 240}, {224, 224, 224, 255}, {33, 33, 33, 255}, {33, 150, 243, 255}, {227, 242, 253, 255}, {187, 222, 251, 255}, {0, 0, 0, 80}, {255, 253, 231, 250}, {158, 158, 158, 255}, 6, "Roboto:size=10:antialias=true");
}

Neu_Theme Neu_Theme::MaterialDark()
{
    Neu_Theme t = makeTheme({18, 18, 18, 255}, {30, 30, 34, 240}, {62, 66, 74, 255}, {232, 234, 237, 255}, {144, 202, 249, 255}, {64, 76, 92, 255}, {56, 60, 68, 255}, {0, 0, 0, 140}, {34, 34, 38, 250}, {144, 202, 249, 255}, 10, "Roboto:size=10:antialias=true:hinting=true:hintstyle=hintfull:rgba=rgb:lcdfilter=lcddefault");
    t.focus = {42, 112, 178, 255};
    t.controlGradientTop = {58, 62, 70, 255};
    t.controlGradientBottom = {22, 24, 30, 255};
    t.edgeSize = 8;
    t.antiAliasMode = Neu_AntiAliasMode::SSAA;
    t.antiAliasSamples = 4;
    t.setDefaultEdgeCorners();
    return t;
}

Neu_Theme Neu_Theme::Ocean()
{
    return makeTheme({8, 54, 78, 255}, {221, 245, 252, 230}, {64, 164, 196, 255}, {4, 40, 56, 255}, {0, 157, 196, 255}, {198, 235, 247, 255}, {158, 215, 235, 255}, {0, 30, 50, 95}, {232, 251, 255, 250}, {0, 120, 160, 255}, 13);
}

Neu_Theme Neu_Theme::Forest()
{
    return makeTheme({30, 64, 42, 255}, {235, 248, 235, 230}, {91, 140, 92, 255}, {28, 50, 30, 255}, {54, 136, 74, 255}, {219, 242, 220, 255}, {184, 224, 188, 255}, {0, 40, 0, 90}, {250, 255, 232, 250}, {91, 120, 60, 255}, 12);
}

Neu_Theme Neu_Theme::Rose()
{
    return makeTheme({255, 242, 246, 255}, {255, 250, 252, 235}, {224, 143, 166, 255}, {82, 29, 45, 255}, {214, 65, 112, 255}, {255, 232, 239, 255}, {246, 199, 214, 255}, {80, 0, 30, 70}, {255, 250, 230, 250}, {190, 90, 120, 255}, 14);
}

Neu_Theme Neu_Theme::Amber()
{
    return makeTheme({255, 248, 225, 255}, {255, 253, 240, 235}, {212, 163, 60, 255}, {65, 48, 20, 255}, {255, 152, 0, 255}, {255, 239, 190, 255}, {255, 213, 128, 255}, {80, 55, 0, 70}, {255, 255, 232, 250}, {180, 120, 40, 255}, 10);
}

Neu_Theme Neu_Theme::Slate()
{
    return makeTheme({226, 232, 240, 255}, {248, 250, 252, 235}, {148, 163, 184, 255}, {30, 41, 59, 255}, {59, 130, 246, 255}, {241, 245, 249, 255}, {203, 213, 225, 255}, {15, 23, 42, 70}, {255, 255, 230, 250}, {100, 116, 139, 255}, 10);
}

Neu_Theme Neu_Theme::Candy()
{
    return makeTheme({255, 245, 252, 255}, {255, 255, 255, 238}, {190, 160, 230, 255}, {76, 42, 100, 255}, {236, 72, 153, 255}, {245, 230, 255, 255}, {244, 194, 230, 255}, {90, 40, 120, 70}, {255, 250, 240, 250}, {190, 120, 200, 255}, 16);
}

Neu_Theme Neu_Theme::TerminalGreen()
{
    return makeTheme({0, 20, 0, 255}, {0, 35, 0, 240}, {0, 128, 0, 255}, {160, 255, 160, 255}, {0, 255, 65, 255}, {0, 55, 0, 255}, {0, 85, 0, 255}, {0, 0, 0, 130}, {0, 40, 0, 250}, {0, 255, 65, 255}, 4, "DejaVu Sans Mono:size=10:antialias=true");
}

Neu_Theme Neu_Theme::CorporateBlue()
{
    return makeTheme({237, 242, 248, 255}, {255, 255, 255, 238}, {166, 184, 204, 255}, {25, 42, 63, 255}, {31, 85, 158, 255}, {225, 237, 252, 255}, {198, 219, 245, 255}, {20, 40, 80, 65}, {255, 255, 232, 250}, {114, 132, 155, 255}, 8);
}

std::vector<std::string> Neu_Theme::BuiltInThemeNames()
{
    return {"MaterialDark", "Light", "Dark", "BlueGlass", "Win95", "WinXP", "Win10", "Win11", "ClassicMotif", "SolarizedLight", "SolarizedDark", "Nord", "Dracula", "GruvboxLight", "GruvboxDark", "HighContrastLight", "HighContrastDark", "UbuntuAubergine", "KDEBreeze", "MacAqua", "MaterialLight", "Ocean", "Forest", "Rose", "Amber", "Slate", "Candy", "TerminalGreen", "CorporateBlue"};
}

Neu_Theme Neu_Theme::BuiltInThemeByName(const std::string& name)
{
    const std::string key = lowerName(name);
    if (key == "materialdark") return MaterialDark();
    if (key == "light") return Light();
    if (key == "dark") return Dark();
    if (key == "blueglass") return BlueGlass();
    if (key == "win95") return Win95();
    if (key == "winxp") return WinXP();
    if (key == "win10") return Win10();
    if (key == "win11") return Win11();
    if (key == "classicmotif") return ClassicMotif();
    if (key == "solarizedlight") return SolarizedLight();
    if (key == "solarizeddark") return SolarizedDark();
    if (key == "nord") return Nord();
    if (key == "dracula") return Dracula();
    if (key == "gruvboxlight") return GruvboxLight();
    if (key == "gruvboxdark") return GruvboxDark();
    if (key == "highcontrastlight") return HighContrastLight();
    if (key == "highcontrastdark") return HighContrastDark();
    if (key == "ubuntuaubergine") return UbuntuAubergine();
    if (key == "kdebreeze") return KDEBreeze();
    if (key == "macaqua") return MacAqua();
    if (key == "materiallight") return MaterialLight();
    if (key == "ocean") return Ocean();
    if (key == "forest") return Forest();
    if (key == "rose") return Rose();
    if (key == "amber") return Amber();
    if (key == "slate") return Slate();
    if (key == "candy") return Candy();
    if (key == "terminalgreen") return TerminalGreen();
    if (key == "corporateblue") return CorporateBlue();
    return Light();
}

} // namespace neutrino
