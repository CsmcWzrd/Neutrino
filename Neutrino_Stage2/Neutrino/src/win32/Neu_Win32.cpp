#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Neutrino/Neutrino.hpp"
#include <windowsx.h>
#include <commctrl.h>
#include <algorithm>
#include <cmath>
#include <cwchar>
#include <sstream>
#include <cctype>

namespace neutrino {

static Neu_SmoothGraphicsOptions g_options{};
static Neu_Theme g_currentDrawingTheme{};
static const wchar_t* kNeuWindowClass = L"Neutrino_Neu_Window";
static HINSTANCE g_instance = nullptr;
static bool g_windowClassRegistered = false;
static HFONT g_uiFont = nullptr;

static constexpr int kListMinColumnWidthWin32 = 48;
static constexpr int kListDefaultColumnWidthWin32 = 160;
static constexpr int kListRowHeightWin32 = 28;
static constexpr int kTreeHeaderHeightWin32 = 26;
static constexpr int kTreeRowHeightWin32 = 28;

static std::wstring toWide(const std::string& s)
{
    if (s.empty()) {
        return L"";
    }

    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    UINT codePage = CP_UTF8;
    if (n <= 0) {
        codePage = CP_ACP;
        n = MultiByteToWideChar(codePage, 0, s.c_str(), -1, nullptr, 0);
    }

    if (n <= 0) {
        return L"";
    }

    std::wstring w(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(codePage, 0, s.c_str(), -1, &w[0], n);
    if (!w.empty() && w.back() == L'\0') {
        w.pop_back();
    }
    return w;
}

static std::string utf8FromWide(const wchar_t* text, int count)
{
    if (!text || count <= 0) {
        return {};
    }

    int n = WideCharToMultiByte(CP_UTF8, 0, text, count, nullptr, 0, nullptr, nullptr);
    if (n <= 0) {
        return {};
    }

    std::string out(static_cast<size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, count, &out[0], n, nullptr, nullptr);
    return out;
}

static std::string unescapeHashesWin32(const std::string& text)
{
    std::string output;
    output.reserve(text.size());
    bool escaped = false;
    for (char ch : text) {
        if (escaped) {
            output.push_back(ch);
            escaped = false;
            continue;
        }
        if (ch == '\\') {
            escaped = true;
            continue;
        }
        output.push_back(ch);
    }
    if (escaped) {
        output.push_back('\\');
    }
    return output;
}


static COLORREF rgb(const Neu_Color& c)
{
    return RGB(c.r, c.g, c.b);
}

static Neu_Color darkerWin32(const Neu_Color& c, int amount = 24)
{
    const int r = static_cast<int>(c.r) - amount;
    const int g = static_cast<int>(c.g) - amount;
    const int b = static_cast<int>(c.b) - amount;
    return {
        static_cast<uint8_t>(r < 0 ? 0 : r),
        static_cast<uint8_t>(g < 0 ? 0 : g),
        static_cast<uint8_t>(b < 0 ? 0 : b),
        c.a
    };
}

Neu_Color Neu_LightenColor(const Neu_Color& color, int amount)
{
    auto add = [amount](uint8_t value) -> uint8_t {
        return static_cast<uint8_t>(((static_cast<int>(value) + amount) > 255 ? 255 : (static_cast<int>(value) + amount)));
    };
    return {add(color.r), add(color.g), add(color.b), color.a};
}

Neu_Color Neu_DarkenColor(const Neu_Color& color, int amount)
{
    auto sub = [amount](uint8_t value) -> uint8_t {
        return static_cast<uint8_t>(((static_cast<int>(value) - amount) < 0 ? 0 : (static_cast<int>(value) - amount)));
    };
    return {sub(color.r), sub(color.g), sub(color.b), color.a};
}

Neu_Color Neu_MixColor(const Neu_Color& a, const Neu_Color& b, double t)
{
    t = std::max(0.0, std::min(1.0, t));
    auto mix = [t](uint8_t av, uint8_t bv) -> uint8_t {
        return static_cast<uint8_t>(std::round(static_cast<double>(av) * (1.0 - t) + static_cast<double>(bv) * t));
    };
    return {mix(a.r, b.r), mix(a.g, b.g), mix(a.b, b.b), mix(a.a, b.a)};
}


static Neu_Theme makeThemeWin32(Neu_Color background,
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
                                const std::string& font = "Segoe UI")
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
    theme.focus = Neu_DarkenColor(accent, 56);
    theme.controlGradientTop = Neu_LightenColor(glass, 24);
    theme.controlGradientBottom = Neu_DarkenColor(glass, 20);
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

static std::string lowerNameWin32(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    value.erase(std::remove(value.begin(), value.end(), ' '), value.end());
    value.erase(std::remove(value.begin(), value.end(), '_'), value.end());
    value.erase(std::remove(value.begin(), value.end(), '-'), value.end());
    return value;
}

static HFONT uiFont()
{
    if (!g_uiFont) {
        g_uiFont = CreateFontW(-16,
                               0,
                               0,
                               0,
                               FW_NORMAL,
                               FALSE,
                               FALSE,
                               FALSE,
                               DEFAULT_CHARSET,
                               OUT_OUTLINE_PRECIS,
                               CLIP_DEFAULT_PRECIS,
                               CLEARTYPE_QUALITY,
                               VARIABLE_PITCH | FF_DONTCARE,
                               L"Segoe UI");
    }
    return g_uiFont;
}

static HFONT createTextFont(bool bold, bool italic, bool underline, bool strikeOut, bool monospace, int headingLevel)
{
    const int height = headingLevel > 0 ? -(34 - headingLevel * 2) : -16;
    return CreateFontW(height,
                       0,
                       0,
                       0,
                       bold ? FW_BOLD : FW_NORMAL,
                       italic ? TRUE : FALSE,
                       underline ? TRUE : FALSE,
                       strikeOut ? TRUE : FALSE,
                       DEFAULT_CHARSET,
                       OUT_OUTLINE_PRECIS,
                       CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY,
                       (monospace ? FIXED_PITCH : VARIABLE_PITCH) | FF_DONTCARE,
                       monospace ? L"Consolas" : L"Segoe UI");
}


static int textWidthWin32(HDC hdc,
                          const std::string& text,
                          bool monospace = false,
                          bool bold = false,
                          bool italic = false,
                          int headingLevel = 0)
{
    if (!hdc || text.empty()) {
        const int fallbackAdvance = monospace ? 8 : 7;
        return static_cast<int>(text.size()) * fallbackAdvance;
    }

    HFONT font = createTextFont(bold, italic, false, false, monospace, headingLevel);
    HGDIOBJ oldFont = SelectObject(hdc, font ? font : uiFont());
    std::wstring wide = toWide(text);
    SIZE size{};
    GetTextExtentPoint32W(hdc, wide.c_str(), static_cast<int>(wide.size()), &size);
    SelectObject(hdc, oldFont);
    if (font) {
        DeleteObject(font);
    }
    return size.cx < 0 ? 0 : static_cast<int>(size.cx);
}

static std::vector<std::string> logicalLinesWin32(const std::string& text)
{
    std::vector<std::string> out;
    std::string current;
    current.reserve(text.size());

    for (size_t i = 0; i < text.size(); ++i) {
        const char ch = text[i];
        if (ch == '\r') {
            if (i + 1 < text.size() && text[i + 1] == '\n') {
                ++i;
            }
            out.push_back(current);
            current.clear();
        } else if (ch == '\n') {
            out.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }

    out.push_back(current);
    if (out.empty()) {
        out.push_back({});
    }
    return out;
}

static std::vector<std::string> splitPreserveLinesWin32(const std::string& text)
{
    return logicalLinesWin32(text);
}

static size_t lineStartOffsetWin32(const std::string& text, int targetLine)
{
    int line = 0;
    size_t start = 0;
    for (size_t i = 0; i < text.size() && line < targetLine; ++i) {
        if (text[i] == '\r') {
            if (i + 1 < text.size() && text[i + 1] == '\n') {
                ++i;
            }
            start = i + 1;
            ++line;
        } else if (text[i] == '\n') {
            start = i + 1;
            ++line;
        }
    }
    return std::min(start, text.size());
}

static size_t lineEndOffsetWin32(const std::string& text, size_t start)
{
    size_t end = start;
    while (end < text.size() && text[end] != '\n' && text[end] != '\r') {
        ++end;
    }
    return end;
}

static void caretLinePrefixWin32(const std::string& text, size_t caret, size_t& lineIndex, size_t& lineStart)
{
    caret = std::min(caret, text.size());
    lineIndex = 0;
    lineStart = 0;
    size_t i = 0;
    while (i < caret && i < text.size()) {
        if (text[i] == '\r') {
            if (i + 1 < caret && i + 1 < text.size() && text[i + 1] == '\n') {
                i += 2;
            } else {
                ++i;
            }
            ++lineIndex;
            lineStart = i;
            continue;
        }
        if (text[i] == '\n') {
            ++i;
            ++lineIndex;
            lineStart = i;
            continue;
        }
        ++i;
    }
    if (lineStart > caret) {
        lineStart = caret;
    }
}

static int byteOffsetFromXWin32(HDC hdc, const std::string& text, size_t start, size_t end, int x, bool monospace)
{
    size_t cursor = start;
    for (size_t i = start + 1; i <= end; ++i) {
        const std::string prefix = text.substr(start, i - start);
        int width = static_cast<int>(prefix.size()) * (monospace ? 8 : 7);
        if (hdc) {
            HFONT font = createTextFont(false, false, false, false, monospace, 0);
            HGDIOBJ oldFont = SelectObject(hdc, font ? font : uiFont());
            std::wstring wide = toWide(prefix);
            SIZE sz{};
            GetTextExtentPoint32W(hdc, wide.c_str(), static_cast<int>(wide.size()), &sz);
            width = sz.cx;
            SelectObject(hdc, oldFont);
            if (font) {
                DeleteObject(font);
            }
        }
        if (width <= x) {
            cursor = i;
        } else {
            break;
        }
    }
    return static_cast<int>(cursor);
}

static int lineHeightForHeadingWin32(int headingLevel)
{
    if (headingLevel > 0) {
        return std::max(22, 38 - headingLevel * 2);
    }
    return 20;
}


static int uiFontPixelHeightWin32(HDC hdc, bool bold = false, bool italic = false, bool monospace = false, int headingLevel = 0)
{
    if (!hdc) {
        return headingLevel > 0 ? lineHeightForHeadingWin32(headingLevel) : 18;
    }
    HFONT font = createTextFont(bold, italic, false, false, monospace, headingLevel);
    HGDIOBJ old = SelectObject(hdc, font ? font : uiFont());
    TEXTMETRICW tm{};
    GetTextMetricsW(hdc, &tm);
    SelectObject(hdc, old);
    if (font) {
        DeleteObject(font);
    }
    return std::max(1, static_cast<int>(tm.tmHeight));
}

static int centeredTextTopWin32(HDC hdc, int controlTop, int controlHeight)
{
    const int fh = uiFontPixelHeightWin32(hdc);
    return controlTop + std::max(2, (controlHeight - fh) / 2);
}

static bool ctrlDownWin32()
{
    return (GetKeyState(VK_CONTROL) & 0x8000) != 0;
}

static bool shiftDownWin32()
{
    return (GetKeyState(VK_SHIFT) & 0x8000) != 0;
}

static int neuMaxIntWin32(int a, int b)
{
    return a > b ? a : b;
}

static int neuMinIntWin32(int a, int b)
{
    return a < b ? a : b;
}

static RECT safeRectWin32(int left, int top, int right, int bottom)
{
    if (right < left) {
        right = left;
    }
    if (bottom < top) {
        bottom = top;
    }
    return RECT{left, top, right, bottom};
}

static void drawArrowWin32(HDC hdc, int centerX, int centerY, bool up, COLORREF color)
{
    POINT points[3]{};
    if (up) {
        points[0] = POINT{centerX, centerY - 4};
        points[1] = POINT{centerX - 5, centerY + 3};
        points[2] = POINT{centerX + 5, centerY + 3};
    } else {
        points[0] = POINT{centerX - 5, centerY - 3};
        points[1] = POINT{centerX + 5, centerY - 3};
        points[2] = POINT{centerX, centerY + 4};
    }
    HBRUSH brush = CreateSolidBrush(color);
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HGDIOBJ oldBrush = SelectObject(hdc, brush);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    Polygon(hdc, points, 3);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(pen);
    DeleteObject(brush);
}

static void drawSupersampledEllipseWin32(HDC hdc, const RECT& dest, COLORREF background, COLORREF fill, COLORREF outline, bool filled)
{
    const int w = neuMaxIntWin32(1, static_cast<int>(dest.right - dest.left));
    const int h = neuMaxIntWin32(1, static_cast<int>(dest.bottom - dest.top));
    constexpr int scale = 3;
    HDC mem = CreateCompatibleDC(hdc);
    HBITMAP bmp = CreateCompatibleBitmap(hdc, w * scale, h * scale);
    HGDIOBJ oldBmp = SelectObject(mem, bmp);
    RECT mr{0, 0, w * scale, h * scale};
    HBRUSH bg = CreateSolidBrush(background);
    FillRect(mem, &mr, bg);
    DeleteObject(bg);
    HPEN pen = CreatePen(PS_SOLID, scale, outline);
    HBRUSH brush = filled ? CreateSolidBrush(fill) : reinterpret_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
    HGDIOBJ oldPen = SelectObject(mem, pen);
    HGDIOBJ oldBrush = SelectObject(mem, brush);
    Ellipse(mem, scale, scale, w * scale - scale, h * scale - scale);
    SelectObject(mem, oldBrush);
    SelectObject(mem, oldPen);
    DeleteObject(pen);
    if (filled) {
        DeleteObject(brush);
    }
    int oldMode = SetStretchBltMode(hdc, HALFTONE);
    SetBrushOrgEx(hdc, 0, 0, nullptr);
    StretchBlt(hdc, dest.left, dest.top, w, h, mem, 0, 0, w * scale, h * scale, SRCCOPY);
    SetStretchBltMode(hdc, oldMode);
    SelectObject(mem, oldBmp);
    DeleteObject(bmp);
    DeleteDC(mem);
}

static void drawSupersampledRoundRectWin32(HDC hdc, const RECT& dest, int radius, COLORREF background, COLORREF fill, COLORREF outline, bool filled)
{
    const int w = neuMaxIntWin32(1, static_cast<int>(dest.right - dest.left));
    const int h = neuMaxIntWin32(1, static_cast<int>(dest.bottom - dest.top));
    constexpr int scale = 3;
    HDC mem = CreateCompatibleDC(hdc);
    HBITMAP bmp = CreateCompatibleBitmap(hdc, w * scale, h * scale);
    HGDIOBJ oldBmp = SelectObject(mem, bmp);
    RECT mr{0, 0, w * scale, h * scale};
    HBRUSH bg = CreateSolidBrush(background);
    FillRect(mem, &mr, bg);
    DeleteObject(bg);
    HPEN pen = CreatePen(PS_SOLID, scale, outline);
    HBRUSH brush = filled ? CreateSolidBrush(fill) : reinterpret_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
    HGDIOBJ oldPen = SelectObject(mem, pen);
    HGDIOBJ oldBrush = SelectObject(mem, brush);
    RoundRect(mem, scale, scale, w * scale - scale, h * scale - scale, radius * scale * 2, radius * scale * 2);
    SelectObject(mem, oldBrush);
    SelectObject(mem, oldPen);
    DeleteObject(pen);
    if (filled) {
        DeleteObject(brush);
    }
    int oldMode = SetStretchBltMode(hdc, HALFTONE);
    SetBrushOrgEx(hdc, 0, 0, nullptr);
    StretchBlt(hdc, dest.left, dest.top, w, h, mem, 0, 0, w * scale, h * scale, SRCCOPY);
    SetStretchBltMode(hdc, oldMode);
    SelectObject(mem, oldBmp);
    DeleteObject(bmp);
    DeleteDC(mem);
}

static RECT insetRectWin32(const Neu_Rect& r, int left, int top, int right, int bottom)
{
    return safeRectWin32(r.x + left, r.y + top, r.x + r.width - right, r.y + r.height - bottom);
}

static void drawCenteredTriangleWin32(HDC hdc, COLORREF color, int cx, int cy, bool up)
{
    POINT pts[3]{};
    const int halfWidth = 4;
    const int halfHeight = 3;
    if (up) {
        pts[0] = POINT{cx, cy - halfHeight};
        pts[1] = POINT{cx - halfWidth, cy + halfHeight};
        pts[2] = POINT{cx + halfWidth, cy + halfHeight};
    } else {
        pts[0] = POINT{cx - halfWidth, cy - halfHeight};
        pts[1] = POINT{cx + halfWidth, cy - halfHeight};
        pts[2] = POINT{cx, cy + halfHeight};
    }
    HBRUSH brush = CreateSolidBrush(color);
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HGDIOBJ oldBrush = SelectObject(hdc, brush);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    Polygon(hdc, pts, 3);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(pen);
    DeleteObject(brush);
}

static COLORREF mixColorRefWin32(COLORREF a, COLORREF b, double t)
{
    t = std::max(0.0, std::min(1.0, t));
    auto mix = [t](int av, int bv) -> BYTE {
        return static_cast<BYTE>(std::round(static_cast<double>(av) * (1.0 - t) + static_cast<double>(bv) * t));
    };
    return RGB(mix(GetRValue(a), GetRValue(b)), mix(GetGValue(a), GetGValue(b)), mix(GetBValue(a), GetBValue(b)));
}

static bool hasEdgeCornersWin32(const Neu_Theme& theme)
{
    return theme.topLeftCorner == Neu_CornerStyle::EdgeCorner
           || theme.topRightCorner == Neu_CornerStyle::EdgeCorner
           || theme.bottomLeftCorner == Neu_CornerStyle::EdgeCorner
           || theme.bottomRightCorner == Neu_CornerStyle::EdgeCorner;
}

static void beginThemedRectPathWin32(HDC hdc, const Neu_Rect& r, int radius)
{
    BeginPath(hdc);
    if (!hasEdgeCornersWin32(g_currentDrawingTheme)) {
        RoundRect(hdc, r.x, r.y, r.x + r.width, r.y + r.height, radius * 2, radius * 2);
    } else {
        const int e = neuMaxIntWin32(0, neuMinIntWin32(g_currentDrawingTheme.edgeSize, neuMinIntWin32(r.width, r.height) / 2));
        const int tl = g_currentDrawingTheme.topLeftCorner == Neu_CornerStyle::EdgeCorner ? e : 0;
        const int tr = g_currentDrawingTheme.topRightCorner == Neu_CornerStyle::EdgeCorner ? e : 0;
        const int bl = g_currentDrawingTheme.bottomLeftCorner == Neu_CornerStyle::EdgeCorner ? e : 0;
        const int br = g_currentDrawingTheme.bottomRightCorner == Neu_CornerStyle::EdgeCorner ? e : 0;
        POINT pts[8]{
            POINT{r.x + tl, r.y},
            POINT{r.x + r.width - tr, r.y},
            POINT{r.x + r.width, r.y + tr},
            POINT{r.x + r.width, r.y + r.height - br},
            POINT{r.x + r.width - br, r.y + r.height},
            POINT{r.x + bl, r.y + r.height},
            POINT{r.x, r.y + r.height - bl},
            POINT{r.x, r.y + tl}
        };
        Polygon(hdc, pts, 8);
    }
    EndPath(hdc);
}

static void strokeThemedRectWin32(HDC hdc, const Neu_Rect& r, int radius, COLORREF border)
{
    HPEN pen = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
    if (!hasEdgeCornersWin32(g_currentDrawingTheme)) {
        RoundRect(hdc, r.x, r.y, r.x + r.width, r.y + r.height, radius * 2, radius * 2);
    } else {
        const int e = neuMaxIntWin32(0, neuMinIntWin32(g_currentDrawingTheme.edgeSize, neuMinIntWin32(r.width, r.height) / 2));
        const int tl = g_currentDrawingTheme.topLeftCorner == Neu_CornerStyle::EdgeCorner ? e : 0;
        const int tr = g_currentDrawingTheme.topRightCorner == Neu_CornerStyle::EdgeCorner ? e : 0;
        const int bl = g_currentDrawingTheme.bottomLeftCorner == Neu_CornerStyle::EdgeCorner ? e : 0;
        const int br = g_currentDrawingTheme.bottomRightCorner == Neu_CornerStyle::EdgeCorner ? e : 0;
        POINT pts[9]{
            POINT{r.x + tl, r.y},
            POINT{r.x + r.width - tr, r.y},
            POINT{r.x + r.width, r.y + tr},
            POINT{r.x + r.width, r.y + r.height - br},
            POINT{r.x + r.width - br, r.y + r.height},
            POINT{r.x + bl, r.y + r.height},
            POINT{r.x, r.y + r.height - bl},
            POINT{r.x, r.y + tl},
            POINT{r.x + tl, r.y}
        };
        Polyline(hdc, pts, 9);
    }
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

static void fillRound(HDC hdc, const Neu_Rect& r, int radius, COLORREF fill, COLORREF border)
{
    if (!hdc || r.width <= 0 || r.height <= 0) {
        return;
    }

    int saved = SaveDC(hdc);
    beginThemedRectPathWin32(hdc, r, radius);
    SelectClipPath(hdc, RGN_AND);

    if (g_currentDrawingTheme.gradientControls) {
        const COLORREF top = mixColorRefWin32(fill, rgb(g_currentDrawingTheme.controlGradientTop), 0.42);
        const COLORREF bottom = mixColorRefWin32(fill, rgb(g_currentDrawingTheme.controlGradientBottom), 0.42);
        for (int yy = 0; yy < r.height; ++yy) {
            const double t = static_cast<double>(yy) / static_cast<double>(neuMaxIntWin32(1, r.height - 1));
            const COLORREF lineColor = mixColorRefWin32(top, bottom, t);
            HBRUSH brush = CreateSolidBrush(lineColor);
            RECT line{r.x, r.y + yy, r.x + r.width, r.y + yy + 1};
            FillRect(hdc, &line, brush);
            DeleteObject(brush);
        }
    } else {
        HBRUSH brush = CreateSolidBrush(fill);
        RECT rect{r.x, r.y, r.x + r.width, r.y + r.height};
        FillRect(hdc, &rect, brush);
        DeleteObject(brush);
    }

    RestoreDC(hdc, saved);
    strokeThemedRectWin32(hdc, r, radius, border);
}


static uint16_t read16(const std::vector<uint8_t>& bytes, size_t offset)
{
    if (offset + 1 >= bytes.size()) {
        return 0;
    }
    return static_cast<uint16_t>(bytes[offset] | (bytes[offset + 1] << 8));
}

static uint32_t read32(const std::vector<uint8_t>& bytes, size_t offset)
{
    if (offset + 3 >= bytes.size()) {
        return 0;
    }
    return static_cast<uint32_t>(bytes[offset])
           | (static_cast<uint32_t>(bytes[offset + 1]) << 8)
           | (static_cast<uint32_t>(bytes[offset + 2]) << 16)
           | (static_cast<uint32_t>(bytes[offset + 3]) << 24);
}

static void clientPointFromLParam(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, int& x, int& y)
{
    if (msg == WM_MOUSEWHEEL || msg == WM_MOUSEHWHEEL) {
        POINT pt{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        ScreenToClient(hwnd, &pt);
        x = pt.x;
        y = pt.y;
    } else if (msg == WM_KEYDOWN || msg == WM_CHAR) {
        (void)wp;
        POINT pt{};
        GetCursorPos(&pt);
        ScreenToClient(hwnd, &pt);
        x = pt.x;
        y = pt.y;
    } else if (msg == WM_TIMER) {
        (void)wp;
        (void)lp;
        x = 0;
        y = 0;
    } else {
        x = GET_X_LPARAM(lp);
        y = GET_Y_LPARAM(lp);
    }
}

static LRESULT CALLBACK Neu_WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    Neu_Window* win = reinterpret_cast<Neu_Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        win = reinterpret_cast<Neu_Window*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(win));
    }

    if (!win) {
        return DefWindowProcW(hwnd, msg, wp, lp);
    }

    switch (msg) {
    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT: {
        PAINTSTRUCT ps{};
        BeginPaint(hwnd, &ps);
        win->paint(ps.hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_KEYDOWN:
    case WM_CHAR:
    case WM_MOUSELEAVE:
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
    case WM_TIMER:
    case WM_SIZE:
    case WM_CLOSE: {
        XEvent ev{};
        ev.message = msg;
        ev.wParam = wp;
        ev.lParam = lp;
        clientPointFromLParam(hwnd, msg, wp, lp, ev.x, ev.y);
        win->handleXEvent(ev);
        return 0;
    }

    case WM_NCDESTROY:
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        return DefWindowProcW(hwnd, msg, wp, lp);

    default:
        return DefWindowProcW(hwnd, msg, wp, lp);
    }
}

Neu_Application* Neu_Application::current_ = nullptr;

Neu_Application::Neu_Application()
{
    current_ = this;
}

Neu_Application::~Neu_Application()
{
    if (g_uiFont) {
        DeleteObject(g_uiFont);
        g_uiFont = nullptr;
    }
    current_ = nullptr;
}

bool Neu_Application::open()
{
    g_instance = GetModuleHandleW(nullptr);
    SetProcessDPIAware();

    INITCOMMONCONTROLSEX icc{};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);

    if (!g_windowClassRegistered) {
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.style = CS_DBLCLKS;
        wc.lpfnWndProc = Neu_WindowProc;
        wc.hInstance = g_instance;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr;
        wc.lpszClassName = kNeuWindowClass;

        if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            return false;
        }
        g_windowClassRegistered = true;
    }

    const char* vmMode = std::getenv("NEUTRINO_VM_MODE");
    const char* fastMode = std::getenv("NEUTRINO_FAST_RENDER");
    if ((vmMode && std::strcmp(vmMode, "0") != 0) || (fastMode && std::strcmp(fastMode, "0") != 0)) {
        Neu_UseVirtualMachineFriendlyDefaults(true);
    }

    return true;
}

void Neu_Application::run()
{
    running_ = true;
    MSG msg{};
    while (running_ && GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void Neu_Application::quit()
{
    running_ = false;
    PostQuitMessage(0);
}

void Neu_Application::registerWindow(Neu_Window* win)
{
    if (win && std::find(windows_.begin(), windows_.end(), win) == windows_.end()) {
        windows_.push_back(win);
    }
}

void Neu_Application::unregisterWindow(Neu_Window* win)
{
    windows_.erase(std::remove(windows_.begin(), windows_.end(), win), windows_.end());
}

Neu_Application* Neu_Application::current()
{
    return current_;
}

bool Neu_Application::detectXRender()
{
    return false;
}

unsigned long Neu_Pixel(Display*, const Neu_Color& color)
{
    return static_cast<unsigned long>(rgb(color));
}

Neu_Color Neu_PixelToColor(Display*, unsigned long pixel)
{
    return Neu_Color{static_cast<uint8_t>(GetRValue(pixel)),
                     static_cast<uint8_t>(GetGValue(pixel)),
                     static_cast<uint8_t>(GetBValue(pixel)),
                     255};
}

void Neu_SetSmoothGraphicsOptions(const Neu_SmoothGraphicsOptions& options)
{
    g_options = options;
    g_options.supersample = std::max(1, std::min(8, g_options.supersample));
    g_options.bufferStages = std::max(1, std::min(3, g_options.bufferStages));
}

Neu_SmoothGraphicsOptions Neu_GetSmoothGraphicsOptions()
{
    return g_options;
}

void Neu_EnableAntialiasing(bool enabled)
{
    g_options.enabled = enabled;
}

void Neu_UseVirtualMachineFriendlyDefaults(bool enabled)
{
    g_options.vmFriendly = enabled;
    if (enabled) {
        g_options.drawShadows = false;
        g_options.supersample = 1;
        g_options.repaintOnMouseMove = false;
        g_options.bufferStages = 2;
    }
}

void Neu_EnableMultiStageDoubleBuffering(bool enabled)
{
    g_options.multiStageDoubleBuffering = enabled;
}

void Neu_ApplyThemeRenderingOptions(const Neu_Theme& theme)
{
    if (g_options.vmFriendly) {
        return;
    }
    g_options.enabled = true;
    if (theme.antiAliasMode == Neu_AntiAliasMode::SSAA) {
        g_options.backend = Neu_GraphicsBackend::SoftwareAntialias;
        g_options.supersample = std::max(4, std::min(8, theme.antiAliasSamples));
    } else if (theme.antiAliasMode == Neu_AntiAliasMode::MSAA) {
        g_options.backend = Neu_GraphicsBackend::SoftwareAntialias;
        g_options.supersample = std::max(3, std::min(6, theme.antiAliasSamples));
    } else {
        g_options.backend = Neu_GraphicsBackend::XRenderAntialias;
        g_options.supersample = std::max(2, std::min(4, theme.antiAliasSamples));
    }
}

void Neu_SetCurrentDrawingTheme(const Neu_Theme& theme)
{
    g_currentDrawingTheme = theme;
    Neu_ApplyThemeRenderingOptions(theme);
}

void Neu_DrawRoundedRect(Display*, Drawable drawable, GC, int x, int y, int w, int h, int radius, bool fill)
{
    Neu_Rect r{x, y, w, h};
    fillRound(drawable, r, radius, rgb(fill ? g_currentDrawingTheme.glass : g_currentDrawingTheme.border), rgb(g_currentDrawingTheme.border));
}

void Neu_DrawSmoothRoundedRect(Display*, Drawable drawable, GC, const Neu_Color& color, const Neu_Color&, int x, int y, int w, int h, int radius, bool, int)
{
    fillRound(drawable, Neu_Rect{x, y, w, h}, radius, rgb(color), rgb(g_currentDrawingTheme.border));
}

void Neu_DrawSmoothDropShadow(Display*, Drawable drawable, GC, const Neu_Color& shadow, const Neu_Color&, int x, int y, int w, int h, int radius, int, int offsetX, int offsetY)
{
    if (!g_options.drawShadows) {
        return;
    }

    const Neu_Rect sr{x + offsetX, y + offsetY, w, h};
    int saved = SaveDC(drawable);
    beginThemedRectPathWin32(drawable, sr, radius);
    HBRUSH brush = CreateSolidBrush(RGB(shadow.r, shadow.g, shadow.b));
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(shadow.r, shadow.g, shadow.b));
    HGDIOBJ oldBrush = SelectObject(drawable, brush);
    HGDIOBJ oldPen = SelectObject(drawable, pen);
    FillPath(drawable);
    RestoreDC(drawable, saved);
    DeleteObject(pen);
    DeleteObject(brush);
}

namespace {

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
    theme.focus = Neu_DarkenColor(accent, 56);
    theme.controlGradientTop = Neu_LightenColor(glass, 24);
    theme.controlGradientBottom = Neu_DarkenColor(glass, 20);
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
    Neu_Theme t = makeTheme({18, 18, 18, 255}, {30, 30, 34, 240}, {62, 66, 74, 255}, {232, 234, 237, 255}, {144, 202, 249, 255}, {64, 76, 92, 255}, {56, 60, 68, 255}, {0, 0, 0, 140}, {34, 34, 38, 250}, {144, 202, 249, 255}, 10, "Segoe UI");
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

bool Neu_IconBmp::load(const std::string& path)
{
    HANDLE file = CreateFileW(toWide(path).c_str(),
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              nullptr,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD size = GetFileSize(file, nullptr);
    if (size == INVALID_FILE_SIZE || size < 54) {
        CloseHandle(file);
        return false;
    }

    std::vector<uint8_t> bytes(size);
    DWORD read = 0;
    const BOOL ok = ReadFile(file, bytes.data(), size, &read, nullptr);
    CloseHandle(file);

    if (!ok || read < 54 || bytes[0] != 'B' || bytes[1] != 'M') {
        return false;
    }

    const uint32_t off = read32(bytes, 10);
    const int32_t bw = static_cast<int32_t>(read32(bytes, 18));
    const int32_t bh = static_cast<int32_t>(read32(bytes, 22));
    const uint16_t bpp = read16(bytes, 28);
    if (bw <= 0 || bh == 0 || (bpp != 24 && bpp != 32)) {
        return false;
    }

    w_ = bw;
    h_ = std::abs(bh);
    pixels_.assign(static_cast<size_t>(w_ * h_), 0xff000000u);
    const int bytesPerPixel = bpp / 8;
    const int stride = ((w_ * bpp + 31) / 32) * 4;
    const bool bottomUp = bh > 0;

    if (off + static_cast<uint32_t>(stride * h_) > read) {
        return false;
    }

    for (int y = 0; y < h_; ++y) {
        const int sy = bottomUp ? (h_ - 1 - y) : y;
        const uint8_t* row = bytes.data() + off + static_cast<size_t>(sy * stride);
        for (int x = 0; x < w_; ++x) {
            const uint8_t b = row[x * bytesPerPixel + 0];
            const uint8_t g = row[x * bytesPerPixel + 1];
            const uint8_t r = row[x * bytesPerPixel + 2];
            const uint8_t a = bpp == 32 ? row[x * 4 + 3] : 255;
            pixels_[static_cast<size_t>(y * w_ + x)] = (static_cast<uint32_t>(a) << 24)
                                                       | (static_cast<uint32_t>(r) << 16)
                                                       | (static_cast<uint32_t>(g) << 8)
                                                       | b;
        }
    }

    return true;
}

Neu_TypedValue Neu_TypeInterpreter::interpret(const std::string& text)
{
    Neu_TypedValue v{};
    v.original = text;
    if (text == "true" || text == "1:true" || text == "checkbox:1") {
        v.type = Neu_CellType::Boolean;
        v.value = true;
    } else if (text == "false" || text == "0:false" || text == "checkbox:0") {
        v.type = Neu_CellType::Boolean;
        v.value = false;
    } else if (text.rfind("0x", 0) == 0 || text.rfind("hex:", 0) == 0) {
        v.type = Neu_CellType::Hex;
        v.value = text;
    } else if (text.rfind("image:", 0) == 0) {
        v.type = Neu_CellType::ImageBmp;
        v.value = text.substr(6);
    } else {
        char* end = nullptr;
        const double d = std::strtod(text.c_str(), &end);
        if (end && *end == '\0' && text.find('.') != std::string::npos) {
            v.type = Neu_CellType::Double;
            v.value = d;
        } else {
            v.type = Neu_CellType::String;
            v.value = text;
        }
    }
    return v;
}

Neu_Control::Neu_Control(Neu_Layout layout)
    : layout_(layout)
{
}

bool Neu_Control::contains(int x, int y) const
{
    Neu_Rect r = bounds();
    return x >= r.x && y >= r.y && x < r.x + r.width && y < r.y + r.height;
}

void Neu_Control::setText(const std::string& text)
{
    text_ = text;
    invokeTextChanged();
    requestRedraw();
}

void Neu_Control::setScrollOffset(int x, int y)
{
    const auto r = bounds();
    const int maxX = std::max(0, virtualSize_.width - r.width);
    const int maxY = std::max(0, virtualSize_.height - r.height);
    scrollX_ = std::max(0, std::min(x, maxX));
    scrollY_ = std::max(0, std::min(y, maxY));
    if (callbacks_.onScroll) {
        callbacks_.onScroll(this, scrollX_, scrollY_, callbacks_.userData);
    }
    requestRedraw();
}

void Neu_Control::setVirtualSize(int width, int height)
{
    virtualSize_ = {std::max(0, width), std::max(0, height)};
    const auto r = bounds();
    scrollX_ = std::max(0, std::min(scrollX_, std::max(0, virtualSize_.width - r.width)));
    scrollY_ = std::max(0, std::min(scrollY_, std::max(0, virtualSize_.height - r.height)));
}

void Neu_Control::requestRedraw()
{
    if (parent_) {
        parent_->requestRedraw();
    }
}

void Neu_Control::drawText(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme, const std::string& s, int x, int y)
{
    drawTextColored(d, drawable, gc, theme, s, x, y, theme.text);
}

void Neu_Control::drawTextColored(Display*,
                                  Drawable drawable,
                                  GC,
                                  const Neu_Theme&,
                                  const std::string& s,
                                  int x,
                                  int y,
                                  const Neu_Color& color,
                                  bool bold,
                                  bool italic,
                                  bool underline,
                                  bool strikethrough,
                                  bool doubleStrikethrough,
                                  bool monospace,
                                  int headingLevel)
{
    SetBkMode(drawable, TRANSPARENT);
    SetTextColor(drawable, rgb(color));
    HFONT font = createTextFont(bold, italic, underline, strikethrough || doubleStrikethrough, monospace, headingLevel);
    HGDIOBJ old = SelectObject(drawable, font ? font : uiFont());
    std::wstring w = toWide(s);
    TextOutW(drawable, x, y, w.c_str(), static_cast<int>(w.size()));

    SIZE sz{};
    GetTextExtentPoint32W(drawable, w.c_str(), static_cast<int>(w.size()), &sz);
    if (doubleStrikethrough) {
        HPEN pen = CreatePen(PS_SOLID, 1, rgb(color));
        HGDIOBJ oldPen = SelectObject(drawable, pen);
        MoveToEx(drawable, x, y + 7, nullptr);
        LineTo(drawable, x + sz.cx, y + 7);
        MoveToEx(drawable, x, y + 10, nullptr);
        LineTo(drawable, x + sz.cx, y + 10);
        SelectObject(drawable, oldPen);
        DeleteObject(pen);
    }
    SelectObject(drawable, old);
    if (font) {
        DeleteObject(font);
    }
}

int Neu_Control::measureTextWidth(Display*,
                                  Drawable drawable,
                                  GC,
                                  const Neu_Theme&,
                                  const std::string& s,
                                  bool bold,
                                  bool italic,
                                  bool monospace,
                                  int headingLevel) const
{
    HDC hdc = drawable;
    if (!hdc) {
        return static_cast<int>(s.size()) * (monospace ? 8 : 7);
    }
    HFONT font = createTextFont(bold, italic, false, false, monospace, headingLevel);
    HGDIOBJ old = SelectObject(hdc, font ? font : uiFont());
    std::wstring w = toWide(s);
    SIZE sz{};
    GetTextExtentPoint32W(hdc, w.c_str(), static_cast<int>(w.size()), &sz);
    SelectObject(hdc, old);
    if (font) {
        DeleteObject(font);
    }
    return std::max(0, static_cast<int>(sz.cx));
}

std::vector<std::string> Neu_Control::wrapTextToWidth(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme, const std::string& text, int maxWidth) const
{
    std::vector<std::string> wrapped;
    if (maxWidth <= 0) {
        wrapped.push_back({});
        return wrapped;
    }
    for (const auto& logical : logicalLinesWin32(text)) {
        std::string current;
        std::string word;
        std::istringstream input(logical);
        while (input >> word) {
            const std::string candidate = current.empty() ? word : current + " " + word;
            if (!current.empty() && measureTextWidth(d, drawable, gc, theme, candidate) > maxWidth) {
                wrapped.push_back(current);
                current.clear();
            }
            if (measureTextWidth(d, drawable, gc, theme, word) > maxWidth) {
                std::string part;
                for (char ch : word) {
                    std::string candidatePart = part + ch;
                    if (!part.empty() && measureTextWidth(d, drawable, gc, theme, candidatePart) > maxWidth) {
                        wrapped.push_back(part);
                        part.clear();
                    }
                    part.push_back(ch);
                }
                if (!part.empty()) {
                    if (!current.empty()) {
                        wrapped.push_back(current);
                    }
                    current = part;
                }
            } else {
                current = current.empty() ? word : current + " " + word;
            }
        }
        wrapped.push_back(current);
    }
    if (wrapped.empty()) {
        wrapped.push_back({});
    }
    return wrapped;
}

std::string Neu_Control::truncateTextToWidth(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme, const std::string& text, int maxWidth) const
{
    if (maxWidth <= 0 || text.empty()) {
        return {};
    }
    if (measureTextWidth(d, drawable, gc, theme, text) <= maxWidth) {
        return text;
    }
    const std::string ellipsis = "...";
    std::string out;
    for (char ch : text) {
        std::string candidate = out + ch + ellipsis;
        if (measureTextWidth(d, drawable, gc, theme, candidate) > maxWidth) {
            break;
        }
        out.push_back(ch);
    }
    return out + ellipsis;
}

int Neu_Control::alignedTextX(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme, const std::string& text, int left, int width) const
{
    const int textWidth = measureTextWidth(d, drawable, gc, theme, text);
    if (textAlignment_ == Neu_TextAlignment::Center) {
        return left + std::max(0, (width - textWidth) / 2);
    }
    if (textAlignment_ == Neu_TextAlignment::Right) {
        return left + std::max(0, width - textWidth);
    }
    return left;
}

void Neu_Control::drawIconBmp(Display*, Drawable drawable, GC, int x, int y, int maxSize)
{
    if (icon_.width() <= 0 || icon_.height() <= 0 || maxSize <= 0) {
        return;
    }

    const int sz = std::min(maxSize, std::min(icon_.width(), icon_.height()));
    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = icon_.width();
    bmi.bmiHeader.biHeight = -icon_.height();
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    StretchDIBits(drawable,
                  x,
                  y,
                  sz,
                  sz,
                  0,
                  0,
                  icon_.width(),
                  icon_.height(),
                  icon_.pixels().data(),
                  &bmi,
                  DIB_RGB_COLORS,
                  SRCCOPY);
}

void Neu_Control::drawShadow(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    if (!g_options.drawShadows) {
        return;
    }
    Neu_Rect r = bounds();
    Neu_DrawSmoothDropShadow(d,
                             drawable,
                             gc,
                             theme.shadow,
                             theme.background,
                             r.x,
                             r.y,
                             r.width,
                             r.height,
                             theme.radius,
                             theme.shadowSize,
                             theme.shadowOffsetX,
                             theme.shadowOffsetY);
}

void Neu_Control::drawHintPopup(Display*, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    if (!g_options.drawHints || !hover_ || !hoverHintArmed_ || hintText_.empty()) {
        return;
    }

    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - hoverStartTime_).count();
    if (elapsed < 5000) {
        return;
    }

    Neu_Rect r = bounds();
    constexpr int padding = 12;
    constexpr int lineHeight = 20;
    int boxWidth = 400;
    if (parent_) {
        boxWidth = std::min(boxWidth, std::max(140, parent_->width() - 12));
    }
    const int contentWidth = std::max(40, boxWidth - 2 * padding - 18);
    const auto lines = wrapTextToWidth(nullptr, drawable, gc, theme, hintText_, contentWidth);
    const int naturalHeight = static_cast<int>(lines.size()) * lineHeight + 2 * padding;
    const int visibleHeight = std::max(44, std::min(500, naturalHeight));
    const int visibleLines = std::max(1, (visibleHeight - 2 * padding) / lineHeight);
    const bool needsScrollbar = naturalHeight > visibleHeight;
    const bool needsDropDown = lines.size() > 3U;
    int boxX = r.x + r.width + 12;
    int boxY = r.y;
    if (parent_) {
        if (boxX + boxWidth > parent_->width()) {
            boxX = std::max(6, r.x - boxWidth - 12);
        }
        if (boxY + visibleHeight > parent_->height()) {
            boxY = std::max(6, parent_->height() - visibleHeight - 6);
        }
    }

    RECT box{boxX, boxY, boxX + boxWidth, boxY + visibleHeight};
    HBRUSH b = CreateSolidBrush(rgb(theme.hintBackground));
    FillRect(drawable, &box, b);
    DeleteObject(b);
    FrameRect(drawable, &box, reinterpret_cast<HBRUSH>(GetStockObject(GRAY_BRUSH)));

    const int clipLeft = box.left + padding;
    const int clipTop = box.top + padding;
    const int clipRight = box.right - padding - (needsScrollbar ? 12 : 2);
    const int clipBottom = box.bottom - padding;
    int saved = SaveDC(drawable);
    IntersectClipRect(drawable, clipLeft, clipTop, clipRight, clipBottom);
    const int count = hintExpanded_ ? std::min(static_cast<int>(lines.size()), visibleLines) : std::min(3, visibleLines);
    const int drawLeft = clipLeft + 4;
    const int drawWidth = std::max(1, clipRight - drawLeft - 2);
    for (int i = 0; i < count; ++i) {
        const std::string line = truncateTextToWidth(nullptr, drawable, gc, theme, lines[static_cast<size_t>(i)], drawWidth);
        drawText(nullptr, drawable, gc, theme, line, drawLeft, clipTop + i * lineHeight + 14);
    }
    RestoreDC(drawable, saved);

    if (needsDropDown) {
        drawText(nullptr, drawable, gc, theme, hintExpanded_ ? "^" : "v", box.right - 24, box.bottom - 24);
    }
    if (needsScrollbar) {
        RECT track{box.right - 10, box.top + 12, box.right - 6, box.bottom - 12};
        HBRUSH tr = CreateSolidBrush(RGB(205, 210, 170));
        FillRect(drawable, &track, tr);
        DeleteObject(tr);
        RECT thumb{box.right - 11, box.top + 12, box.right - 5, box.top + 12 + std::max(20, visibleHeight / 4)};
        HBRUSH th = CreateSolidBrush(rgb(theme.hintBorder));
        FillRect(drawable, &thumb, th);
        DeleteObject(th);
    }
}

void Neu_Control::drawScrollbars(Display*, Drawable drawable, GC, const Neu_Theme& theme)
{
    if (!autoScroll_) {
        return;
    }

    Neu_Rect r = bounds();
    const bool needVertical = virtualSize_.height > r.height;
    const bool needHorizontal = virtualSize_.width > r.width;
    HBRUSH track = CreateSolidBrush(RGB(205, 215, 226));
    HBRUSH thumb = CreateSolidBrush(rgb(theme.focus));

    if (needVertical) {
        const int trackX = r.x + r.width - 10;
        const int trackY = r.y + 8;
        const int trackH = std::max(1, r.height - 16);
        const int thumbH = std::max(20, trackH * r.height / std::max(1, virtualSize_.height));
        const int maxY = std::max(1, virtualSize_.height - r.height);
        const int thumbY = trackY + (trackH - thumbH) * scrollY_ / maxY;
        RECT tr{trackX, trackY, trackX + 5, trackY + trackH};
        RECT th{trackX - 1, thumbY, trackX + 6, thumbY + thumbH};
        FillRect(drawable, &tr, track);
        FillRect(drawable, &th, thumb);
    }

    if (needHorizontal) {
        const int trackX = r.x + 8;
        const int trackY = r.y + r.height - 10;
        const int trackW = std::max(1, r.width - 16);
        const int thumbW = std::max(20, trackW * r.width / std::max(1, virtualSize_.width));
        const int maxX = std::max(1, virtualSize_.width - r.width);
        const int thumbX = trackX + (trackW - thumbW) * scrollX_ / maxX;
        RECT tr{trackX, trackY, trackX + trackW, trackY + 5};
        RECT th{thumbX, trackY - 1, thumbX + thumbW, trackY + 6};
        FillRect(drawable, &tr, track);
        FillRect(drawable, &th, thumb);
    }

    DeleteObject(track);
    DeleteObject(thumb);
}

void Neu_Control::invokeClick()
{
    if (callbacks_.onClick) {
        callbacks_.onClick(this, callbacks_.userData);
    }
}

void Neu_Control::invokeTextChanged()
{
    if (callbacks_.onTextChanged) {
        callbacks_.onTextChanged(this, text_.c_str(), callbacks_.userData);
    }
}

void Neu_Control::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Rect r = bounds();
    const std::string cls = className();
    const bool suppressHoverFill = cls == "Neu_RichTextCode"
                                   || cls == "Neu_ReadOnlyRichText"
                                   || cls == "Neu_Placement"
                                   || cls == "Neu_ScrollWindow"
                                   || cls == "Neu_ListView"
                                   || cls == "Neu_TreeView"
                                   || cls == "Neu_Multilinetextbox";
    const bool visualHover = hover_ && !suppressHoverFill;
    fillRound(drawable, r, theme.radius, rgb(visualHover ? theme.highlight : theme.glass), rgb(focused_ ? theme.focus : theme.border));
    if (focused_ && !suppressHoverFill) {
        strokeThemedRectWin32(drawable, Neu_Rect{r.x + 2, r.y + 2, r.width - 4, r.height - 4}, std::max(1, theme.radius - 2), rgb(theme.focus));
    }
    drawIconBmp(d, drawable, gc, r.x + 6, r.y + 6, r.height - 12);

    const bool baseText = cls == "Neu_Button" || cls == "Neu_Control" || cls == "Neu_MenuItem" || cls == "Neu_FlatButton";
    if (baseText && !text_.empty()) {
        const int textLeft = r.x + (icon_.width() > 0 ? r.height : 10);
        const int textWidth = std::max(1, r.width - (textLeft - r.x) - 12);
        int saved = SaveDC(drawable);
        IntersectClipRect(drawable, textLeft, r.y + 4, r.x + r.width - 8, r.y + r.height - 4);
        const std::string visible = truncateTextToWidth(d, drawable, gc, theme, text_, textWidth);
        drawText(d,
                 drawable,
                 gc,
                 theme,
                 visible,
                 alignedTextX(d, drawable, gc, theme, visible, textLeft, textWidth),
                 r.y + std::max(4, (r.height - 16) / 2));
        RestoreDC(drawable, saved);
    }
    drawScrollbars(d, drawable, gc, theme);
}

void Neu_Control::handleXEvent(XEvent& ev)
{
    const bool inside = contains(ev.x, ev.y);

    auto resetHintAnchor = [&]() {
        hoverAnchorX_ = ev.x;
        hoverAnchorY_ = ev.y;
        hoverStartTime_ = std::chrono::steady_clock::now();
        hoverHintArmed_ = true;
    };

    auto updateScrollbarDrag = [&]() -> bool {
        if (!autoScroll_ || activeScrollDrag_ == 0) {
            return false;
        }
        Neu_Rect r = bounds();
        if (activeScrollDrag_ == 1) {
            const int trackY = r.y + 8;
            const int trackH = std::max(1, r.height - 16);
            const int maxY = std::max(1, virtualSize_.height - r.height);
            setScrollOffset(scrollX_, (ev.y - trackY) * maxY / trackH);
        } else if (activeScrollDrag_ == 2) {
            const int trackX = r.x + 8;
            const int trackW = std::max(1, r.width - 16);
            const int maxX = std::max(1, virtualSize_.width - r.width);
            setScrollOffset((ev.x - trackX) * maxX / trackW, scrollY_);
        }
        return true;
    };

    if (ev.message == WM_MOUSEMOVE) {
        if (updateScrollbarDrag()) {
            return;
        }
        if (inside) {
            if (!hover_) {
                hover_ = true;
                resetHintAnchor();
                if (callbacks_.onFocus) {
                    callbacks_.onFocus(this, callbacks_.userData);
                }
                requestRedraw();
            } else if (std::abs(ev.x - hoverAnchorX_) > 4 || std::abs(ev.y - hoverAnchorY_) > 4) {
                resetHintAnchor();
                requestRedraw();
            }
        } else if (hover_) {
            hover_ = false;
            hoverHintArmed_ = false;
            if (callbacks_.onBlur) {
                callbacks_.onBlur(this, callbacks_.userData);
            }
            requestRedraw();
        }
        return;
    }

    if (ev.message == WM_MOUSELEAVE) {
        if (hover_) {
            hover_ = false;
            hoverHintArmed_ = false;
            if (callbacks_.onBlur) {
                callbacks_.onBlur(this, callbacks_.userData);
            }
            requestRedraw();
        }
        activeScrollDrag_ = 0;
        return;
    }

    if (ev.message == WM_LBUTTONDOWN && inside && autoScroll_) {
        Neu_Rect r = bounds();
        const bool needVertical = virtualSize_.height > r.height;
        const bool needHorizontal = virtualSize_.width > r.width;
        if (needVertical && ev.x >= r.x + r.width - 14) {
            activeScrollDrag_ = 1;
            updateScrollbarDrag();
            return;
        }
        if (needHorizontal && ev.y >= r.y + r.height - 14) {
            activeScrollDrag_ = 2;
            updateScrollbarDrag();
            return;
        }
    }

    if (ev.message == WM_LBUTTONUP && activeScrollDrag_ != 0) {
        updateScrollbarDrag();
        activeScrollDrag_ = 0;
        return;
    }

    if (ev.message == WM_LBUTTONDOWN && inside) {
        pressed_ = true;
        requestRedraw();
    } else if (ev.message == WM_LBUTTONUP) {
        const bool wasPressed = pressed_;
        pressed_ = false;
        if (inside && wasPressed) {
            invokeClick();
        }
        if (wasPressed) {
            requestRedraw();
        }
    } else if (ev.message == WM_MOUSEWHEEL && autoScroll_) {
        const int delta = GET_WHEEL_DELTA_WPARAM(ev.wParam) / WHEEL_DELTA;
        if (inside || hover_ || focused_) {
            if (GET_KEYSTATE_WPARAM(ev.wParam) & MK_SHIFT) {
                setScrollOffset(scrollX_ - delta * 48, scrollY_);
            } else {
                setScrollOffset(scrollX_, scrollY_ - delta * 48);
            }
        }
    } else if (ev.message == WM_MOUSEHWHEEL && autoScroll_) {
        const int delta = GET_WHEEL_DELTA_WPARAM(ev.wParam) / WHEEL_DELTA;
        if (inside || hover_ || focused_) {
            setScrollOffset(scrollX_ + delta * 48, scrollY_);
        }
    } else if (ev.message == WM_KEYDOWN && focused_ && callbacks_.onKeyDown) {
        callbacks_.onKeyDown(this, static_cast<KeySym>(ev.wParam), 0, callbacks_.userData);
    }
}

void Neu_Button::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
}

void Neu_Button::handleXEvent(XEvent& ev)
{
    Neu_Control::handleXEvent(ev);
}

void Neu_Textbox::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    Neu_Rect r = bounds();
    const int textLeft = r.x + 10;
    const int textTop = centeredTextTopWin32(drawable, r.y, r.height);
    const int textWidth = std::max(1, r.width - 22);

    const size_t caretBytes = std::min(cursor_, text_.size());
    const std::string prefix = text_.substr(0, caretBytes);
    const int prefixWidth = measureTextWidth(d, drawable, gc, theme, prefix);
    const int fullWidth = measureTextWidth(d, drawable, gc, theme, text_);
    const int maxScroll = std::max(0, fullWidth - textWidth + 4);
    if (focused_) {
        if (prefixWidth - textScrollX_ > textWidth - 2) {
            textScrollX_ = prefixWidth - textWidth + 2;
        }
        if (prefixWidth - textScrollX_ < 0) {
            textScrollX_ = prefixWidth;
        }
    }
    textScrollX_ = std::max(0, std::min(textScrollX_, maxScroll));

    int saved = SaveDC(drawable);
    IntersectClipRect(drawable, r.x + 6, r.y + 4, r.x + r.width - 6, r.y + r.height - 4);
    drawText(d,
             drawable,
             gc,
             theme,
             text_,
             textLeft - textScrollX_,
             textTop);
    if (focused_) {
        const int caretX = textLeft + prefixWidth - textScrollX_;
        if (caretX >= r.x + 6 && caretX <= r.x + r.width - 6) {
            HPEN pen = CreatePen(PS_SOLID, 1, rgb(theme.focus));
            HGDIOBJ old = SelectObject(drawable, pen);
            const int caretTop = textTop;
            const int caretBottom = std::min(r.y + r.height - 5, textTop + uiFontPixelHeightWin32(drawable));
            MoveToEx(drawable, caretX, caretTop, nullptr);
            LineTo(drawable, caretX, caretBottom);
            SelectObject(drawable, old);
            DeleteObject(pen);
        }
    }
    RestoreDC(drawable, saved);
    drawHintPopup(d, drawable, gc, theme);
}

void Neu_Textbox::handleXEvent(XEvent& ev)
{
    Neu_Control::handleXEvent(ev);

    if (ev.message == WM_LBUTTONDOWN && contains(ev.x, ev.y)) {
        Neu_Rect r = bounds();
        const int textLeft = r.x + 10;
        const int localX = ev.x - textLeft + textScrollX_;
        cursor_ = 0;
        HDC hdc = parent_ && parent_->xid() ? GetDC(parent_->xid()) : nullptr;
        HGDIOBJ oldFont = nullptr;
        if (hdc) {
            oldFont = SelectObject(hdc, uiFont());
        }
        for (size_t i = 1; i <= text_.size(); ++i) {
            int w = static_cast<int>(i) * 8;
            if (hdc) {
                const std::wstring prefix = toWide(text_.substr(0, i));
                SIZE sz{};
                GetTextExtentPoint32W(hdc, prefix.c_str(), static_cast<int>(prefix.size()), &sz);
                w = sz.cx;
            }
            if (w <= localX) {
                cursor_ = i;
            } else {
                break;
            }
        }
        if (hdc) {
            if (oldFont) {
                SelectObject(hdc, oldFont);
            }
            ReleaseDC(parent_->xid(), hdc);
        }
        requestRedraw();
        return;
    }

    if (!focused_ || !enabled_) {
        return;
    }

    bool changed = false;
    if (ev.message == WM_KEYDOWN) {
        switch (ev.wParam) {
        case VK_LEFT:
            if (cursor_ > 0) {
                --cursor_;
            }
            break;
        case VK_RIGHT:
            if (cursor_ < text_.size()) {
                ++cursor_;
            }
            break;
        case VK_HOME:
            cursor_ = 0;
            break;
        case VK_END:
            cursor_ = text_.size();
            break;
        case VK_DELETE:
            if (cursor_ < text_.size()) {
                text_.erase(cursor_, 1);
                invokeTextChanged();
                changed = true;
            }
            break;
        default:
            break;
        }
        (void)changed;
        requestRedraw();
        return;
    }

    if (ev.message == WM_CHAR) {
        wchar_t ch = static_cast<wchar_t>(ev.wParam);
        if (ch == L'\b') {
            if (cursor_ > 0 && !text_.empty()) {
                text_.erase(cursor_ - 1, 1);
                --cursor_;
                invokeTextChanged();
            }
        } else if (ch == L'\r' || ch == L'\n') {
            // Single-line text boxes ignore Enter.
        } else if (ch >= 32) {
            const std::string utf8 = utf8FromWide(&ch, 1);
            text_.insert(cursor_, utf8);
            cursor_ += utf8.size();
            invokeTextChanged();
        }
        requestRedraw();
    }
}

void Neu_Passwordbox::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    const std::string old = text_;
    text_.assign(old.size(), '*');
    Neu_Textbox::draw(d, drawable, gc, theme);
    text_ = old;
}

void Neu_Multilinetextbox::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    Neu_Rect r = bounds();
    const int contentWidth = std::max(1, r.width - 24);
    const int lineHeight = 20;
    std::vector<std::string> lines;
    if (wordWrap_) {
        auto logical = logicalLinesWin32(text_);
        for (const auto& baseLine : logical) {
            auto wrapped = wrapTextToWidth(d, drawable, gc, theme, baseLine, contentWidth);
            lines.insert(lines.end(), wrapped.begin(), wrapped.end());
        }
    } else {
        lines = logicalLinesWin32(text_);
    }
    int maxWidth = r.width;
    for (const auto& line : lines) {
        maxWidth = std::max(maxWidth, measureTextWidth(d, drawable, gc, theme, line) + 24);
    }
    setAutoScroll(true);
    setVirtualSize(std::max(r.width, maxWidth), std::max(r.height, static_cast<int>(lines.size()) * lineHeight + 16));

    int saved = SaveDC(drawable);
    IntersectClipRect(drawable, r.x + 4, r.y + 4, r.x + r.width - 14, r.y + r.height - 14);
    int y = r.y + 8 - scrollY_;
    for (const auto& line : lines) {
        if (y >= r.y + 6 && y < r.y + r.height - 10) {
            if (wordWrap_) {
                drawText(d, drawable, gc, theme, truncateTextToWidth(d, drawable, gc, theme, line, contentWidth), r.x + 10, y);
            } else {
                drawText(d, drawable, gc, theme, line, r.x + 10 - scrollX_, y);
            }
        }
        y += lineHeight;
    }

    if (focused_) {
        size_t lineIndex = 0;
        size_t lineStart = 0;
        const size_t caretBytes = std::min(cursor_, text_.size());
        for (size_t i = 0; i < caretBytes; ++i) {
            if (text_[i] == '\n') {
                ++lineIndex;
                lineStart = i + 1;
            }
        }
        const std::string prefix = text_.substr(lineStart, caretBytes - lineStart);
        const int caretX = r.x + 10 + measureTextWidth(d, drawable, gc, theme, prefix) - (wordWrap_ ? 0 : scrollX_);
        const int caretY = r.y + 8 + static_cast<int>(lineIndex) * lineHeight - scrollY_;
        if (caretY >= r.y + 6 && caretY < r.y + r.height - 10) {
            HPEN pen = CreatePen(PS_SOLID, 1, rgb(theme.focus));
            HGDIOBJ old = SelectObject(drawable, pen);
            const int caretTop = caretY;
            const int caretBottom = std::min(r.y + r.height - 8, caretY + uiFontPixelHeightWin32(drawable));
            MoveToEx(drawable, caretX, caretTop, nullptr);
            LineTo(drawable, caretX, caretBottom);
            SelectObject(drawable, old);
            DeleteObject(pen);
        }
    }
    RestoreDC(drawable, saved);
    drawScrollbars(d, drawable, gc, theme);
    drawHintPopup(d, drawable, gc, theme);
}

void Neu_Multilinetextbox::handleXEvent(XEvent& ev)
{
    if (ev.message == WM_LBUTTONDOWN && contains(ev.x, ev.y)) {
        Neu_Control::handleXEvent(ev);
        if (activeScrollDrag_ != 0) {
            return;
        }

        Neu_Rect r = bounds();
        constexpr int lineHeight = 20;
        const int contentLeft = r.x + 10;
        const int contentTop = r.y + 8;
        const int desiredLine = std::max(0, (ev.y - contentTop + scrollY_) / lineHeight);
        const int localX = ev.x - contentLeft + (wordWrap_ ? 0 : scrollX_);
        const auto lines = logicalLinesWin32(text_);
        const int clampedLine = std::min(desiredLine, std::max(0, static_cast<int>(lines.size()) - 1));
        const size_t start = lineStartOffsetWin32(text_, clampedLine);
        const size_t end = lineEndOffsetWin32(text_, start);

        HDC hdc = parent_ && parent_->xid() ? GetDC(parent_->xid()) : nullptr;
        cursor_ = static_cast<size_t>(byteOffsetFromXWin32(hdc, text_, start, end, localX, false));
        if (hdc) {
            ReleaseDC(parent_->xid(), hdc);
        }
        requestRedraw();
        return;
    }

    if (focused_ && ev.message == WM_CHAR && (ev.wParam == L'\r' || ev.wParam == L'\n')) {
        const size_t insertAt = std::min(cursor_, text_.size());
        text_.insert(insertAt, 1, '\n');
        cursor_ = insertAt + 1;
        invokeTextChanged();
        requestRedraw();
        return;
    }
    Neu_Textbox::handleXEvent(ev);
}

void Neu_Listbox::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    Neu_Rect r = bounds();
    constexpr int rowHeight = 22;
    setAutoScroll(true);
    setVirtualSize(r.width, std::max(r.height, static_cast<int>(items_.size()) * rowHeight + 16));
    int saved = SaveDC(drawable);
    IntersectClipRect(drawable, r.x + 4, r.y + 4, r.x + r.width - 14, r.y + r.height - 14);
    int y = r.y + 18 - scrollY_;
    for (size_t i = 0; i < items_.size(); ++i, y += rowHeight) {
        if (y < r.y + 8) {
            continue;
        }
        if (y >= r.y + r.height - 8) {
            break;
        }
        const bool selected = multiSelect_ ? selectedIndices_.count(static_cast<int>(i)) != 0U : static_cast<int>(i) == selected_;
        const bool hovered = static_cast<int>(i) == hoveredIndex_;
        if (selected || hovered) {
            RECT sr{r.x + 4, y - 16, r.x + r.width - 14, y + 4};
            HBRUSH b = CreateSolidBrush(rgb(selected ? theme.pressed : theme.hover));
            FillRect(drawable, &sr, b);
            DeleteObject(b);
        }
        RECT textClip{r.x + 9, y - 16, r.x + r.width - 16, y + 5};
        int cellSaved = SaveDC(drawable);
        IntersectClipRect(drawable, textClip.left, textClip.top, textClip.right, textClip.bottom);
        drawText(d, drawable, gc, theme, truncateTextToWidth(d, drawable, gc, theme, items_[i], r.width - 28), r.x + 10, y - 14);
        RestoreDC(drawable, cellSaved);
    }
    RestoreDC(drawable, saved);
    drawScrollbars(d, drawable, gc, theme);
    drawHintPopup(d, drawable, gc, theme);
}

void Neu_Listbox::handleXEvent(XEvent& ev)
{
    Neu_Control::handleXEvent(ev);
    if (!contains(ev.x, ev.y)) {
        if (ev.message == WM_MOUSELEAVE && hoveredIndex_ != -1) {
            hoveredIndex_ = -1;
            requestRedraw();
        }
        return;
    }

    Neu_Rect r = bounds();
    const int row = (ev.y - r.y + scrollY_) / 22;
    const bool validRow = row >= 0 && row < static_cast<int>(items_.size());

    if (ev.message == WM_MOUSEMOVE) {
        const int newHover = validRow ? row : -1;
        if (newHover != hoveredIndex_) {
            hoveredIndex_ = newHover;
            requestRedraw();
        }
        return;
    }

    if (ev.message == WM_LBUTTONUP && validRow) {
        selected_ = row;
        if (multiSelect_) {
            if (shiftDownWin32() && anchorIndex_ >= 0) {
                selectedIndices_.clear();
                const int a = std::min(anchorIndex_, row);
                const int b = std::max(anchorIndex_, row);
                for (int i = a; i <= b; ++i) {
                    selectedIndices_.insert(i);
                }
            } else if (ctrlDownWin32()) {
                if (selectedIndices_.count(row) != 0U) {
                    selectedIndices_.erase(row);
                } else {
                    selectedIndices_.insert(row);
                }
                anchorIndex_ = row;
            } else {
                selectedIndices_.clear();
                selectedIndices_.insert(row);
                anchorIndex_ = row;
            }
        } else {
            selectedIndices_.clear();
            selectedIndices_.insert(row);
            anchorIndex_ = row;
        }
        if (callbacks_.onSelectionChanged) {
            callbacks_.onSelectionChanged(this,
                                          selected_,
                                          0,
                                          items_[static_cast<size_t>(selected_)].c_str(),
                                          callbacks_.userData);
        }
        requestRedraw();
    }
}

void Neu_ComboBox::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    Neu_Rect r = bounds();
    const int buttonW = 24;
    const std::string selectedText = selected_ >= 0 && selected_ < static_cast<int>(items_.size())
                                     ? items_[static_cast<size_t>(selected_)]
                                     : std::string{};
    RECT textClip = safeRectWin32(r.x + textOffset_.left + 6,
                                  r.y + 4,
                                  r.x + r.width - buttonW - textOffset_.right - 2,
                                  r.y + r.height - 4);
    int saved = SaveDC(drawable);
    IntersectClipRect(drawable, textClip.left, textClip.top, textClip.right, textClip.bottom);
    const int textWidth = neuMaxIntWin32(1, static_cast<int>(textClip.right - textClip.left) - 2);
    drawText(d,
             drawable,
             gc,
             theme,
             truncateTextToWidth(d, drawable, gc, theme, selectedText, textWidth),
             textClip.left,
             r.y + neuMaxIntWin32(4, (r.height - 16) / 2));
    RestoreDC(drawable, saved);

    HPEN pen = CreatePen(PS_SOLID, 1, rgb(theme.border));
    HGDIOBJ oldPen = SelectObject(drawable, pen);
    const int buttonLeft = r.x + r.width - buttonW;
    MoveToEx(drawable, buttonLeft, r.y + 4, nullptr);
    LineTo(drawable, buttonLeft, r.y + r.height - 4);
    SelectObject(drawable, oldPen);
    DeleteObject(pen);
    drawCenteredTriangleWin32(drawable, rgb(theme.text), buttonLeft + buttonW / 2, r.y + r.height / 2, false);

    if (open_ && !items_.empty()) {
        const int itemH = 22;
        const int visibleRows = std::min<int>(8, static_cast<int>(items_.size()));
        const int dropH = neuMaxIntWin32(itemH, visibleRows * itemH + 4);
        const int dropW = r.width;
        const int dropX = r.x;
        const int dropY = r.y + r.height + 2;
        const int listRight = dropX + dropW - (static_cast<int>(items_.size()) > visibleRows ? 12 : 0);
        fillRound(drawable,
                  Neu_Rect{dropX, dropY, dropW, dropH},
                  std::max(2, theme.radius - 2),
                  rgb(theme.glass),
                  rgb(theme.focus));
        int listSaved = SaveDC(drawable);
        IntersectClipRect(drawable, dropX + 2, dropY + 2, listRight - 2, dropY + dropH - 2);
        const int maxScroll = neuMaxIntWin32(0, static_cast<int>(items_.size()) * itemH - (dropH - 4));
        if (scrollY_ > maxScroll) {
            scrollY_ = maxScroll;
        }
        int y = dropY + 4 - scrollY_;
        for (size_t i = 0; i < items_.size(); ++i, y += itemH) {
            if (y + itemH < dropY + 2) {
                continue;
            }
            if (y > dropY + dropH - 2) {
                break;
            }
            const bool selected = selectedIndices_.count(static_cast<int>(i)) != 0U || selected_ == static_cast<int>(i);
            const bool hover = hoveredIndex_ == static_cast<int>(i);
            if (selected || hover) {
                fillRound(drawable,
                          Neu_Rect{dropX + 3, y, neuMaxIntWin32(1, listRight - dropX - 6), itemH},
                          std::max(2, theme.radius - 4),
                          rgb(selected ? theme.pressed : theme.hover),
                          rgb(selected ? theme.focus : theme.border));
            }
            const int avail = neuMaxIntWin32(1, listRight - dropX - 10);
            drawText(d,
                     drawable,
                     gc,
                     theme,
                     truncateTextToWidth(d, drawable, gc, theme, items_[i], avail),
                     dropX + 6,
                     y + 4);
        }
        RestoreDC(drawable, listSaved);
        if (static_cast<int>(items_.size()) > visibleRows) {
            RECT track{dropX + dropW - 10, dropY + 4, dropX + dropW - 4, dropY + dropH - 4};
            HBRUSH trackBrush = CreateSolidBrush(rgb(Neu_MixColor(theme.glass, theme.border, 0.35)));
            FillRect(drawable, &track, trackBrush);
            DeleteObject(trackBrush);
            const int trackH = neuMaxIntWin32(1, static_cast<int>(track.bottom - track.top));
            const int thumbH = neuMaxIntWin32(16, trackH * visibleRows / static_cast<int>(items_.size()));
            const int thumbY = static_cast<int>(track.top) + (maxScroll > 0 ? (trackH - thumbH) * scrollY_ / maxScroll : 0);
            fillRound(drawable,
                      Neu_Rect{static_cast<int>(track.left), thumbY, static_cast<int>(track.right - track.left), thumbH},
                      3,
                      rgb(theme.accent),
                      rgb(theme.focus));
        }
    }

    drawHintPopup(d, drawable, gc, theme);
}

void Neu_ComboBox::handleXEvent(XEvent& ev)
{
    Neu_Control::handleXEvent(ev);
    Neu_Rect r = bounds();
    const int itemH = 22;
    const int visibleRows = std::min<int>(8, static_cast<int>(items_.size()));
    const int dropH = neuMaxIntWin32(itemH, visibleRows * itemH + 4);
    const int dropX = r.x;
    const int dropY = r.y + r.height + 2;
    const bool inBase = contains(ev.x, ev.y);
    const bool inDrop = open_ && ev.x >= dropX && ev.x <= dropX + r.width && ev.y >= dropY && ev.y <= dropY + dropH;

    if (ev.message == WM_MOUSEWHEEL && inDrop && !items_.empty()) {
        const int delta = GET_WHEEL_DELTA_WPARAM(ev.wParam);
        const int maxScroll = neuMaxIntWin32(0, static_cast<int>(items_.size()) * itemH - (dropH - 4));
        scrollY_ = neuMaxIntWin32(0, neuMinIntWin32(maxScroll, scrollY_ + (delta < 0 ? itemH : -itemH)));
        requestRedraw();
        return;
    }

    if (ev.message == WM_MOUSEMOVE) {
        if (inDrop) {
            hoveredIndex_ = neuMaxIntWin32(0, neuMinIntWin32(static_cast<int>(items_.size()) - 1, (ev.y - (dropY + 4) + scrollY_) / itemH));
        } else if (inBase) {
            hoveredIndex_ = selected_;
        } else {
            hoveredIndex_ = -1;
        }
        requestRedraw();
        return;
    }

    if (ev.message == WM_LBUTTONDOWN && inBase) {
        open_ = !open_;
        requestRedraw();
        return;
    }

    if (ev.message == WM_LBUTTONDOWN && inDrop && !items_.empty()) {
        const int idx = neuMaxIntWin32(0, neuMinIntWin32(static_cast<int>(items_.size()) - 1, (ev.y - (dropY + 4) + scrollY_) / itemH));
        selected_ = idx;
        if (shiftDownWin32() && multiSelect_ && anchorIndex_ >= 0) {
            selectedIndices_.clear();
            const int a = std::min(anchorIndex_, idx);
            const int b = std::max(anchorIndex_, idx);
            for (int i = a; i <= b; ++i) {
                selectedIndices_.insert(i);
            }
        } else if (ctrlDownWin32() && multiSelect_) {
            if (selectedIndices_.count(idx) != 0U) {
                selectedIndices_.erase(idx);
            } else {
                selectedIndices_.insert(idx);
            }
            anchorIndex_ = idx;
        } else {
            selectedIndices_.clear();
            selectedIndices_.insert(idx);
            anchorIndex_ = idx;
            open_ = false;
        }
        if (callbacks_.onSelectionChanged) {
            callbacks_.onSelectionChanged(this,
                                          selected_,
                                          0,
                                          items_[static_cast<size_t>(selected_)].c_str(),
                                          callbacks_.userData);
        }
        requestRedraw();
        return;
    }

    if (ev.message == WM_LBUTTONDOWN && open_ && !inDrop && !inBase) {
        open_ = false;
        requestRedraw();
    }
}

void Neu_ListView::setColumnWidths(const std::vector<int>& widths)
{
    columnWidths_.clear();
    columnWidths_.reserve(widths.size());
    for (int width : widths) {
        columnWidths_.push_back(neuMaxIntWin32(kListMinColumnWidthWin32, width));
    }
    requestRedraw();
}

void Neu_ListView::setColumnWidth(size_t column, int width)
{
    if (columnWidths_.size() <= column) {
        columnWidths_.resize(column + 1, kListDefaultColumnWidthWin32);
    }
    columnWidths_[column] = neuMaxIntWin32(kListMinColumnWidthWin32, width);
    requestRedraw();
}

int Neu_ListView::columnWidth(size_t column) const
{
    return effectiveColumnWidth(column, bounds().width);
}

int Neu_ListView::effectiveColumnWidth(size_t column, int controlWidth) const
{
    if (column < columnWidths_.size() && columnWidths_[column] > 0) {
        return neuMaxIntWin32(kListMinColumnWidthWin32, columnWidths_[column]);
    }
    if (controlWidth > 0) {
        return neuMaxIntWin32(kListDefaultColumnWidthWin32, neuMinIntWin32(360, (controlWidth - 24) * 6 / 10));
    }
    return kListDefaultColumnWidthWin32;
}

int Neu_ListView::totalColumnWidth(size_t columnCount, int controlWidth) const
{
    int total = 0;
    for (size_t col = 0; col < columnCount; ++col) {
        total += effectiveColumnWidth(col, controlWidth);
    }
    return total;
}

Neu_TypedValue Neu_ListView::cellValue(size_t r, size_t c) const
{
    if (!model_ || r >= model_->size() || c >= (*model_)[r].size()) {
        return Neu_TypeInterpreter::interpret("");
    }
    return Neu_TypeInterpreter::interpret((*model_)[r][c]);
}

void Neu_ListView::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    if (!model_) {
        return;
    }

    Neu_Rect r = bounds();
    const int viewportLeft = r.x + 4;
    const int viewportRight = r.x + r.width - 14;
    const int viewportTop = r.y + 4;
    const int viewportBottom = r.y + r.height - 14;
    const int bodyTop = viewportTop + headerHeight_;
    size_t maxCols = 0;
    for (const auto& row : *model_) {
        maxCols = std::max(maxCols, row.size());
    }
    const int totalWidth = totalColumnWidth(maxCols, r.width);
    const int viewportWidth = std::max(1, viewportRight - viewportLeft);
    const int virtualWidth = totalWidth > viewportWidth ? std::max(r.width + 1, totalWidth + 28) : r.width;
    setAutoScroll(true);
    setVirtualSize(virtualWidth,
                   neuMaxIntWin32(r.height, headerHeight_ + static_cast<int>(model_->size()) * kListRowHeightWin32 + 16));

    int saved = SaveDC(drawable);
    IntersectClipRect(drawable, viewportLeft, viewportTop, viewportRight, viewportBottom);

    fillRound(drawable,
              Neu_Rect{viewportLeft, viewportTop, neuMaxIntWin32(1, viewportRight - viewportLeft), neuMaxIntWin32(1, bodyTop - viewportTop)},
              std::max(2, theme.radius - 2),
              rgb(darkerWin32(theme.glass)),
              rgb(theme.border));

    int headerX = r.x + 8 - scrollX_;
    for (size_t col = 0; col < maxCols; ++col) {
        const int cw = effectiveColumnWidth(col, r.width);
        RECT hcell{headerX - 4, viewportTop, headerX + cw - 4, bodyTop};
        if (hcell.right >= viewportLeft && hcell.left <= viewportRight) {
            RECT visibleHeader = safeRectWin32(neuMaxIntWin32(static_cast<int>(hcell.left), viewportLeft),
                                               static_cast<int>(hcell.top),
                                               neuMinIntWin32(static_cast<int>(hcell.right), viewportRight),
                                               static_cast<int>(hcell.bottom));
            FrameRect(drawable, &visibleHeader, reinterpret_cast<HBRUSH>(GetStockObject(GRAY_BRUSH)));
            int cellSaved = SaveDC(drawable);
            IntersectClipRect(drawable, visibleHeader.left + 3, visibleHeader.top, visibleHeader.right - 2, visibleHeader.bottom);
            const std::string title = col == 0 ? "Column 1" : "Column " + std::to_string(col + 1);
            drawText(d,
                     drawable,
                     gc,
                     theme,
                     truncateTextToWidth(d, drawable, gc, theme, title, neuMaxIntWin32(1, static_cast<int>(visibleHeader.right - visibleHeader.left) - 8)),
                     visibleHeader.left + 5,
                     viewportTop + std::max(4, (headerHeight_ - 16) / 2));
            RestoreDC(drawable, cellSaved);
            HPEN pen = CreatePen(PS_SOLID, 1, rgb(theme.focus));
            HGDIOBJ oldPen = SelectObject(drawable, pen);
            MoveToEx(drawable, hcell.right, viewportTop + 3, nullptr);
            LineTo(drawable, hcell.right, bodyTop - 3);
            SelectObject(drawable, oldPen);
            DeleteObject(pen);
        }
        headerX += cw;
    }

    IntersectClipRect(drawable, viewportLeft, bodyTop, viewportRight, viewportBottom);
    const int listFontHeight = uiFontPixelHeightWin32(drawable);
    int rowTop = bodyTop - scrollY_;
    for (size_t row = 0; row < model_->size(); ++row, rowTop += kListRowHeightWin32) {
        const int rowBottom = rowTop + kListRowHeightWin32;
        if (rowBottom <= bodyTop) {
            continue;
        }
        if (rowTop >= viewportBottom) {
            break;
        }
        const bool rowSelected = multiSelect_ ? selectedRows_.count(static_cast<int>(row)) != 0U : static_cast<int>(row) == selectedRow_;
        const bool rowHovered = static_cast<int>(row) == hoveredRow_;
        if (rowSelected || rowHovered) {
            fillRound(drawable,
                      Neu_Rect{viewportLeft, neuMaxIntWin32(rowTop, bodyTop), std::max(1, viewportRight - viewportLeft), neuMaxIntWin32(1, neuMinIntWin32(rowBottom, viewportBottom) - neuMaxIntWin32(rowTop, bodyTop))},
                      std::max(2, theme.radius - 3),
                      rgb(rowSelected ? theme.pressed : theme.hover),
                      rgb(rowSelected ? theme.focus : theme.border));
        }
        int x = r.x + 10 - scrollX_;
        for (size_t col = 0; col < (*model_)[row].size(); ++col) {
            const int cw = effectiveColumnWidth(col, r.width);
            if (x + cw < viewportLeft) {
                x += cw;
                continue;
            }
            if (x >= viewportRight) {
                break;
            }
            RECT cell{x - 3, rowTop + 2, x + cw - 4, rowBottom - 2};
            const int clippedCellLeft = neuMaxIntWin32(static_cast<int>(cell.left), viewportLeft);
            const int clippedCellTop = neuMaxIntWin32(static_cast<int>(cell.top), bodyTop);
            const int clippedCellRight = neuMinIntWin32(static_cast<int>(cell.right), viewportRight);
            const int clippedCellBottom = neuMinIntWin32(static_cast<int>(cell.bottom), viewportBottom);
            RECT visibleCell = safeRectWin32(clippedCellLeft, clippedCellTop, clippedCellRight, clippedCellBottom);
            if (static_cast<int>(row) == selectedRow_ && static_cast<int>(col) == selectedCol_) {
                fillRound(drawable,
                          Neu_Rect{static_cast<int>(visibleCell.left), static_cast<int>(visibleCell.top),
                                   neuMaxIntWin32(1, static_cast<int>(visibleCell.right - visibleCell.left)),
                                   neuMaxIntWin32(1, static_cast<int>(visibleCell.bottom - visibleCell.top))},
                          std::max(2, theme.radius - 4),
                          rgb(theme.hover),
                          rgb(theme.focus));
            }
            FrameRect(drawable, &visibleCell, reinterpret_cast<HBRUSH>(GetStockObject(GRAY_BRUSH)));
            if (visibleCell.right > visibleCell.left + 4) {
                int cellSaved = SaveDC(drawable);
                IntersectClipRect(drawable,
                                  static_cast<int>(visibleCell.left) + 2,
                                  static_cast<int>(visibleCell.top),
                                  static_cast<int>(visibleCell.right) - 2,
                                  static_cast<int>(visibleCell.bottom));
                const int drawX = neuMaxIntWin32(x, static_cast<int>(visibleCell.left) + 2);
                const int visibleWidth = neuMaxIntWin32(1, static_cast<int>(visibleCell.right) - drawX - 2);
                const std::string cellText = truncateTextToWidth(d, drawable, gc, theme, (*model_)[row][col], visibleWidth);
                const int textTop = neuMaxIntWin32(static_cast<int>(visibleCell.top) + 1,
                                                    static_cast<int>(visibleCell.top) + neuMaxIntWin32(0, (static_cast<int>(visibleCell.bottom - visibleCell.top) - listFontHeight) / 2));
                drawText(d, drawable, gc, theme, cellText, drawX, textTop);
                RestoreDC(drawable, cellSaved);
            }
            x += cw;
        }
    }
    RestoreDC(drawable, saved);
    drawScrollbars(d, drawable, gc, theme);
    drawHintPopup(d, drawable, gc, theme);
}

void Neu_ListView::handleXEvent(XEvent& ev)
{
    if (!model_) {
        Neu_Control::handleXEvent(ev);
        return;
    }

    Neu_Rect r = bounds();
    const int viewportTop = r.y + 4;
    const int bodyTop = viewportTop + headerHeight_;
    const int viewportRight = r.x + r.width - 14;
    size_t maxCols = 0;
    for (const auto& rowData : *model_) {
        maxCols = std::max(maxCols, rowData.size());
    }

    if (resizingColumn_ >= 0) {
        if (ev.message == WM_MOUSEMOVE) {
            setColumnWidth(static_cast<size_t>(resizingColumn_), resizeStartWidth_ + (ev.x - resizeStartX_));
            return;
        }
        if (ev.message == WM_LBUTTONUP) {
            resizingColumn_ = -1;
            requestRedraw();
            return;
        }
    }

    if ((ev.message == WM_LBUTTONDOWN || ev.message == WM_MOUSEMOVE)
        && contains(ev.x, ev.y)
        && headerResizable_
        && ev.y >= viewportTop
        && ev.y <= bodyTop) {
        int x = r.x + 10 - scrollX_;
        for (size_t col = 0; col < maxCols; ++col) {
            const int cw = effectiveColumnWidth(col, r.width);
            const int boundary = x + cw - 4;
            if (std::abs(ev.x - boundary) <= 5 && boundary < viewportRight) {
                if (ev.message == WM_LBUTTONDOWN) {
                    resizingColumn_ = static_cast<int>(col);
                    resizeStartX_ = ev.x;
                    resizeStartWidth_ = cw;
                }
                requestRedraw();
                return;
            }
            x += cw;
        }
        if (ev.message == WM_LBUTTONDOWN) {
            return;
        }
    }

    Neu_Control::handleXEvent(ev);
    if (!contains(ev.x, ev.y)) {
        if (ev.message == WM_MOUSELEAVE && (hoveredRow_ != -1 || hoveredCol_ != -1)) {
            hoveredRow_ = -1;
            hoveredCol_ = -1;
            requestRedraw();
        }
        return;
    }

    int row = -1;
    int col = -1;
    if (ev.y >= bodyTop) {
        row = (ev.y - bodyTop + scrollY_) / kListRowHeightWin32;
        int x = r.x + 10 - scrollX_;
        for (size_t c = 0; c < maxCols; ++c) {
            const int cw = effectiveColumnWidth(c, r.width);
            if (ev.x >= x - 4 && ev.x < x + cw - 4) {
                col = static_cast<int>(c);
                break;
            }
            x += cw;
        }
    }
    const bool validRow = row >= 0 && row < static_cast<int>(model_->size());
    const bool validCol = validRow && col >= 0 && col < static_cast<int>((*model_)[static_cast<size_t>(row)].size());

    if (ev.message == WM_MOUSEMOVE) {
        const int newRow = validRow ? row : -1;
        const int newCol = validCol ? col : -1;
        if (newRow != hoveredRow_ || newCol != hoveredCol_) {
            hoveredRow_ = newRow;
            hoveredCol_ = newCol;
            requestRedraw();
        }
        return;
    }

    if (ev.message == WM_LBUTTONUP && validCol) {
        selectedRow_ = row;
        selectedCol_ = col;
        if (multiSelect_) {
            if (shiftDownWin32() && anchorRow_ >= 0) {
                selectedRows_.clear();
                const int a = std::min(anchorRow_, row);
                const int b = std::max(anchorRow_, row);
                for (int i = a; i <= b; ++i) {
                    selectedRows_.insert(i);
                }
            } else if (ctrlDownWin32()) {
                if (selectedRows_.count(row) != 0U) {
                    selectedRows_.erase(row);
                } else {
                    selectedRows_.insert(row);
                }
                anchorRow_ = row;
            } else {
                selectedRows_.clear();
                selectedRows_.insert(row);
                anchorRow_ = row;
            }
        } else {
            selectedRows_.clear();
            selectedRows_.insert(row);
            anchorRow_ = row;
        }
        if (callbacks_.onSelectionChanged) {
            callbacks_.onSelectionChanged(this,
                                          row,
                                          col,
                                          (*model_)[static_cast<size_t>(row)][static_cast<size_t>(col)].c_str(),
                                          callbacks_.userData);
        }
        requestRedraw();
    }
}

namespace {
struct TreeRowInfoWin {
    size_t modelRow{0};
    int depth{0};
    std::string label;
    std::string path;
    bool hasChildren{false};
};

static std::vector<std::string> rowPathWin(const std::vector<std::string>& row)
{
    std::vector<std::string> path;
    for (const auto& cell : row) {
        if (!cell.empty()) {
            path.push_back(cell);
        }
    }
    return path;
}

static std::string joinPathWin(const std::vector<std::string>& path, size_t count)
{
    std::string result;
    for (size_t index = 0; index < count && index < path.size(); ++index) {
        if (!result.empty()) {
            result += '/';
        }
        result += path[index];
    }
    return result;
}

static bool collapsedAncestorWin(const std::vector<std::string>& path, const std::set<std::string>& collapsed)
{
    for (size_t depth = 1; depth < path.size(); ++depth) {
        if (collapsed.count(joinPathWin(path, depth)) != 0U) {
            return true;
        }
    }
    return false;
}

static std::vector<TreeRowInfoWin> buildTreeRowsWin(const Neu_StringTable* model, const std::set<std::string>& collapsed)
{
    std::vector<TreeRowInfoWin> rows;
    if (!model) {
        return rows;
    }
    std::map<std::string, bool> hasChild;
    std::vector<std::vector<std::string>> paths;
    for (const auto& source : *model) {
        auto path = rowPathWin(source);
        for (size_t depth = 1; depth < path.size(); ++depth) {
            hasChild[joinPathWin(path, depth)] = true;
        }
        paths.push_back(path);
    }
    for (size_t i = 0; i < paths.size(); ++i) {
        const auto& path = paths[i];
        if (path.empty() || collapsedAncestorWin(path, collapsed)) {
            continue;
        }
        const std::string key = joinPathWin(path, path.size());
        rows.push_back({i, static_cast<int>(path.size() - 1), path.back(), key, hasChild.count(key) != 0U});
    }
    return rows;
}
}

void Neu_TreeView::expandAll()
{
    collapsedPaths_.clear();
    requestRedraw();
}

void Neu_TreeView::collapseAll()
{
    collapsedPaths_.clear();
    if (model()) {
        for (const auto& row : *model()) {
            auto path = rowPathWin(row);
            for (size_t depth = 1; depth <= path.size(); ++depth) {
                collapsedPaths_.insert(joinPathWin(path, depth));
            }
        }
    }
    requestRedraw();
}

void Neu_TreeView::toggleNodePath(const std::string& path)
{
    if (collapsedPaths_.count(path)) {
        collapsedPaths_.erase(path);
    } else {
        collapsedPaths_.insert(path);
    }
    requestRedraw();
}

bool Neu_TreeView::isPathCollapsed(const std::string& path) const
{
    return collapsedPaths_.count(path) != 0;
}

void Neu_TreeView::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    const auto rows = buildTreeRowsWin(model(), collapsedPaths_);
    Neu_Rect r = bounds();
    constexpr int rowHeight = kTreeRowHeightWin32;
    constexpr int headerH = kTreeHeaderHeightWin32;
    int maxWidth = std::max(r.width, treeColumnWidth_ + 24);
    for (const auto& row : rows) {
        const int indent = row.depth * 18;
        const std::string label = (row.hasChildren ? (isPathCollapsed(row.path) ? "+ " : "- ") : "  ") + row.label;
        maxWidth = std::max(maxWidth, indent + measureTextWidth(d, drawable, gc, theme, label) + 56);
    }
    treeColumnWidth_ = std::max(96, std::min(std::max(maxWidth, treeColumnWidth_), std::max(180, r.width * 3)));
    setAutoScroll(true);
    setVirtualSize(std::max(r.width, std::max(maxWidth, treeColumnWidth_ + 28)),
                   std::max(r.height, headerH + static_cast<int>(rows.size()) * rowHeight + 18));

    const int viewportLeft = r.x + 4;
    const int viewportRight = r.x + r.width - 14;
    const int viewportTop = r.y + 4;
    const int viewportBottom = r.y + r.height - 14;
    const int bodyTop = viewportTop + headerH;
    int saved = SaveDC(drawable);
    IntersectClipRect(drawable, viewportLeft, viewportTop, viewportRight, viewportBottom);

    fillRound(drawable,
              Neu_Rect{viewportLeft, viewportTop, neuMaxIntWin32(1, viewportRight - viewportLeft), neuMaxIntWin32(1, bodyTop - viewportTop)},
              std::max(2, theme.radius - 2),
              rgb(darkerWin32(theme.glass)),
              rgb(theme.border));

    const int headerX = r.x + 8 - scrollX_;
    const int headerRight = headerX + treeColumnWidth_;
    RECT visibleHeader = safeRectWin32(neuMaxIntWin32(headerX - 4, viewportLeft),
                                       viewportTop,
                                       neuMinIntWin32(headerRight, viewportRight),
                                       bodyTop);
    FrameRect(drawable, &visibleHeader, reinterpret_cast<HBRUSH>(GetStockObject(GRAY_BRUSH)));
    int headerSaved = SaveDC(drawable);
    IntersectClipRect(drawable, visibleHeader.left + 4, visibleHeader.top, visibleHeader.right - 2, visibleHeader.bottom);
    drawText(d,
             drawable,
             gc,
             theme,
             truncateTextToWidth(d, drawable, gc, theme, "Tree", neuMaxIntWin32(1, static_cast<int>(visibleHeader.right - visibleHeader.left) - 8)),
             visibleHeader.left + 5,
             viewportTop + std::max(4, (headerH - 16) / 2));
    RestoreDC(drawable, headerSaved);
    HPEN headerPen = CreatePen(PS_SOLID, 1, rgb(theme.focus));
    HGDIOBJ oldHeaderPen = SelectObject(drawable, headerPen);
    const int boundaryX = std::min(viewportRight - 1, headerRight - 1);
    MoveToEx(drawable, boundaryX, viewportTop + 3, nullptr);
    LineTo(drawable, boundaryX, bodyTop - 3);
    SelectObject(drawable, oldHeaderPen);
    DeleteObject(headerPen);

    IntersectClipRect(drawable, viewportLeft, bodyTop, viewportRight, viewportBottom);
    const int treeFontHeight = uiFontPixelHeightWin32(drawable);
    int rowTop = bodyTop - scrollY_;
    for (size_t i = 0; i < rows.size(); ++i, rowTop += rowHeight) {
        const int rowBottom = rowTop + rowHeight;
        if (rowBottom <= bodyTop) {
            continue;
        }
        if (rowTop >= viewportBottom) {
            break;
        }
        const bool selected = multiSelect_ ? selectedVisibleRows_.count(static_cast<int>(i)) != 0U : static_cast<int>(i) == selectedVisibleRow_;
        const bool hovered = static_cast<int>(i) == hoveredVisibleRow_;
        if (selected || hovered) {
            fillRound(drawable,
                      Neu_Rect{viewportLeft, neuMaxIntWin32(rowTop, bodyTop), std::max(1, viewportRight - viewportLeft), neuMaxIntWin32(1, neuMinIntWin32(rowBottom, viewportBottom) - neuMaxIntWin32(rowTop, bodyTop))},
                      std::max(2, theme.radius - 3),
                      rgb(selected ? theme.pressed : theme.hover),
                      rgb(selected ? theme.focus : theme.border));
        }
        const auto& row = rows[i];
        const int indent = row.depth * 18;
        const int textX = r.x + 10 + indent - scrollX_;
        const int columnRight = r.x + 8 + treeColumnWidth_ - scrollX_;
        const std::string label = (row.hasChildren ? (isPathCollapsed(row.path) ? "+ " : "- ") : "  ") + row.label;
        RECT textClip = safeRectWin32(neuMaxIntWin32(textX, viewportLeft),
                                      rowTop + 2,
                                      neuMinIntWin32(columnRight, viewportRight),
                                      rowBottom - 2);
        if (textClip.right > textClip.left + 2) {
            int cellSaved = SaveDC(drawable);
            const int clipTop = neuMaxIntWin32(static_cast<int>(textClip.top), bodyTop);
            const int clipBottom = neuMinIntWin32(static_cast<int>(textClip.bottom), viewportBottom);
            IntersectClipRect(drawable, static_cast<int>(textClip.left), clipTop, static_cast<int>(textClip.right), clipBottom);
            const int drawX = neuMaxIntWin32(textX, static_cast<int>(textClip.left) + 2);
            const int visibleWidth = neuMaxIntWin32(1, static_cast<int>(textClip.right) - drawX - 2);
            drawText(d,
                     drawable,
                     gc,
                     theme,
                     truncateTextToWidth(d, drawable, gc, theme, label, visibleWidth),
                     drawX,
                     neuMaxIntWin32(clipTop + 1, clipTop + neuMaxIntWin32(0, (clipBottom - clipTop - treeFontHeight) / 2)));
            RestoreDC(drawable, cellSaved);
        }
    }
    RestoreDC(drawable, saved);
    drawScrollbars(d, drawable, gc, theme);
    drawHintPopup(d, drawable, gc, theme);
}

void Neu_TreeView::handleXEvent(XEvent& ev)
{
    if (!model()) {
        Neu_Control::handleXEvent(ev);
        return;
    }

    Neu_Rect r = bounds();
    constexpr int headerH = kTreeHeaderHeightWin32;
    constexpr int rowHeight = kTreeRowHeightWin32;
    const int viewportLeft = r.x + 4;
    const int viewportRight = r.x + r.width - 14;
    const int viewportTop = r.y + 4;
    const int bodyTop = viewportTop + headerH;

    if (headerResizeActive_) {
        if (ev.message == WM_MOUSEMOVE) {
            setTreeColumnWidth(headerResizeStartWidth_ + ev.x - headerResizeStartX_);
            return;
        }
        if (ev.message == WM_LBUTTONUP) {
            headerResizeActive_ = false;
            requestRedraw();
            return;
        }
    }

    if ((ev.message == WM_LBUTTONDOWN || ev.message == WM_MOUSEMOVE)
        && contains(ev.x, ev.y)
        && ev.y >= viewportTop
        && ev.y <= bodyTop) {
        const int boundaryX = r.x + 8 + treeColumnWidth_ - scrollX_;
        if (std::abs(ev.x - boundaryX) <= 6 && boundaryX < viewportRight) {
            if (ev.message == WM_LBUTTONDOWN) {
                headerResizeActive_ = true;
                headerResizeStartX_ = ev.x;
                headerResizeStartWidth_ = treeColumnWidth_;
            }
            requestRedraw();
            return;
        }
        if (ev.message == WM_LBUTTONDOWN) {
            return;
        }
    }

    Neu_Control::handleXEvent(ev);
    if (!contains(ev.x, ev.y)) {
        if (ev.message == WM_MOUSELEAVE && hoveredVisibleRow_ != -1) {
            hoveredVisibleRow_ = -1;
            requestRedraw();
        }
        return;
    }

    if (ev.x >= r.x + r.width - 14 || ev.y >= r.y + r.height - 14 || ev.y < bodyTop) {
        return;
    }

    const int rowIndex = (ev.y - bodyTop + scrollY_) / rowHeight;
    const auto rows = buildTreeRowsWin(model(), collapsedPaths_);
    const bool validRow = rowIndex >= 0 && rowIndex < static_cast<int>(rows.size());

    if (ev.message == WM_MOUSEMOVE) {
        const int newHover = validRow ? rowIndex : -1;
        if (newHover != hoveredVisibleRow_) {
            hoveredVisibleRow_ = newHover;
            requestRedraw();
        }
        return;
    }

    if (ev.message == WM_LBUTTONUP && validRow) {
        selectedVisibleRow_ = rowIndex;
        if (multiSelect_) {
            if (shiftDownWin32() && anchorVisibleRow_ >= 0) {
                selectedVisibleRows_.clear();
                const int a = std::min(anchorVisibleRow_, rowIndex);
                const int b = std::max(anchorVisibleRow_, rowIndex);
                for (int i = a; i <= b; ++i) {
                    selectedVisibleRows_.insert(i);
                }
            } else if (ctrlDownWin32()) {
                if (selectedVisibleRows_.count(rowIndex) != 0U) {
                    selectedVisibleRows_.erase(rowIndex);
                } else {
                    selectedVisibleRows_.insert(rowIndex);
                }
                anchorVisibleRow_ = rowIndex;
            } else {
                selectedVisibleRows_.clear();
                selectedVisibleRows_.insert(rowIndex);
                anchorVisibleRow_ = rowIndex;
            }
        } else {
            selectedVisibleRows_.clear();
            selectedVisibleRows_.insert(rowIndex);
            anchorVisibleRow_ = rowIndex;
        }

        const auto& row = rows[static_cast<size_t>(rowIndex)];
        if (row.hasChildren && !ctrlDownWin32() && !shiftDownWin32()) {
            toggleNodePath(row.path);
        }
        if (callbacks_.onSelectionChanged) {
            callbacks_.onSelectionChanged(this,
                                          static_cast<int>(row.modelRow),
                                          0,
                                          row.label.c_str(),
                                          callbacks_.userData);
        }
        requestRedraw();
    }
}

void Neu_Placement::add(std::shared_ptr<Neu_Control> child)
{
    if (!child) {
        return;
    }
    child->setParent(parent_);
    children_.push_back(child);
}

void Neu_Placement::setParent(Neu_Window* parent)
{
    Neu_Control::setParent(parent);
    for (auto& child : children_) {
        if (child) {
            child->setParent(parent);
        }
    }
}

void Neu_Placement::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    for (auto& c : children_) {
        if (c && c->visible()) {
            c->draw(d, drawable, gc, theme);
        }
    }
}

void Neu_Placement::handleXEvent(XEvent& ev)
{
    for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
        auto& child = *it;
        if (child && child->visible() && child->enabled() && child->contains(ev.x, ev.y)) {
            child->handleXEvent(ev);
            return;
        }
    }
    Neu_Control::handleXEvent(ev);
}

void Neu_PopWindowMenu::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Placement::draw(d, drawable, gc, theme);
}

void Neu_ScrollBar::setRange(int total, int page, int value)
{
    total_ = std::max(1, total);
    page_ = std::max(1, page);
    value_ = std::max(0, std::min(value, std::max(0, total_ - page_)));
    requestRedraw();
}

void Neu_ScrollBar::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    Neu_Rect r = bounds();
    const int track = std::max(1, vertical_ ? r.height : r.width);
    const int thumb = std::max(18, track * page_ / std::max(1, total_));
    const int maxValue = std::max(1, total_ - page_);
    const int pos = (track - thumb) * value_ / maxValue;
    HBRUSH b = CreateSolidBrush(rgb(theme.focus));
    if (vertical_) {
        RECT th{r.x + 2, r.y + pos, r.x + r.width - 2, r.y + pos + thumb};
        FillRect(drawable, &th, b);
    } else {
        RECT th{r.x + pos, r.y + 2, r.x + pos + thumb, r.y + r.height - 2};
        FillRect(drawable, &th, b);
    }
    DeleteObject(b);
}

void Neu_ScrollBar::handleXEvent(XEvent& ev)
{
    if ((ev.message == WM_LBUTTONDOWN || ev.message == WM_MOUSEMOVE) && (dragging_ || contains(ev.x, ev.y))) {
        Neu_Rect r = bounds();
        const int coordinate = vertical_ ? ev.y - r.y : ev.x - r.x;
        const int track = std::max(1, vertical_ ? r.height : r.width);
        const int maxValue = std::max(1, total_ - page_);
        if (ev.message == WM_LBUTTONDOWN) {
            dragging_ = true;
        }
        setRange(total_, page_, coordinate * maxValue / track);
        return;
    }
    if (ev.message == WM_LBUTTONUP) {
        dragging_ = false;
    }
    Neu_Control::handleXEvent(ev);
}

void Neu_ScrollWindow::add(std::shared_ptr<Neu_Control> child)
{
    if (!child) {
        return;
    }
    Neu_Layout childLayout = child->layout();
    const Neu_Rect r = bounds();
    if (childLayout.left >= r.x && childLayout.top >= r.y) {
        childLayout.left -= r.x;
        childLayout.top -= r.y;
        child->setLayout(childLayout);
    }
    Neu_Placement::add(child);
}

void Neu_ScrollWindow::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    Neu_Rect r = bounds();
    const int viewportLeft = r.x + 4;
    const int viewportTop = r.y + 4;
    const int viewportRight = r.x + r.width - 14;
    const int viewportBottom = r.y + r.height - 14;
    const int viewportWidth = std::max(1, viewportRight - viewportLeft);
    const int viewportHeight = std::max(1, viewportBottom - viewportTop);

    int saved = SaveDC(drawable);
    IntersectClipRect(drawable, viewportLeft, viewportTop, viewportRight, viewportBottom);
    int maxRight = viewportWidth;
    int maxBottom = viewportHeight;
    for (const auto& child : children()) {
        if (child && child->visible()) {
            Neu_Layout original = child->layout();
            Neu_Layout shifted = original;
            shifted.left = viewportLeft + original.left - scrollX();
            shifted.top = viewportTop + original.top - scrollY();
            child->setLayout(shifted);
            Neu_Rect cr = child->bounds();
            const bool intersects = cr.x + cr.width >= viewportLeft
                                    && cr.y + cr.height >= viewportTop
                                    && cr.x <= viewportRight
                                    && cr.y <= viewportBottom;
            if (intersects) {
                int childSaved = SaveDC(drawable);
                IntersectClipRect(drawable, viewportLeft, viewportTop, viewportRight, viewportBottom);
                child->draw(d, drawable, gc, theme);
                RestoreDC(drawable, childSaved);
            }
            maxRight = std::max(maxRight, original.left + original.width + 20);
            maxBottom = std::max(maxBottom, original.top + original.height + 20);
            child->setLayout(original);
        }
    }
    RestoreDC(drawable, saved);
    setAutoScroll(true);
    setVirtualSize(std::max(virtualSize().width, maxRight), std::max(virtualSize().height, maxBottom));
    drawScrollbars(d, drawable, gc, theme);
    drawHintPopup(d, drawable, gc, theme);
}

void Neu_ScrollWindow::handleXEvent(XEvent& ev)
{
    const bool inside = contains(ev.x, ev.y);
    Neu_Control::handleXEvent(ev);
    if (!inside || activeScrollDrag_ != 0 || ev.message == WM_MOUSEWHEEL || ev.message == WM_MOUSEHWHEEL) {
        return;
    }

    Neu_Rect r = bounds();
    const int viewportLeft = r.x + 4;
    const int viewportTop = r.y + 4;
    const int viewportRight = r.x + r.width - 14;
    const int viewportBottom = r.y + r.height - 14;
    if ((ev.message == WM_MOUSEMOVE || ev.message == WM_LBUTTONDOWN || ev.message == WM_LBUTTONUP)
        && (ev.x < viewportLeft || ev.x > viewportRight || ev.y < viewportTop || ev.y > viewportBottom)) {
        return;
    }

    XEvent adjusted = ev;
    if (ev.message == WM_MOUSEMOVE || ev.message == WM_LBUTTONDOWN || ev.message == WM_LBUTTONUP) {
        adjusted.x = ev.x - viewportLeft + scrollX();
        adjusted.y = ev.y - viewportTop + scrollY();
        for (auto it = children().rbegin(); it != children().rend(); ++it) {
            auto& child = *it;
            if (child && child->visible() && child->enabled() && child->contains(adjusted.x, adjusted.y)) {
                child->handleXEvent(adjusted);
                return;
            }
        }
    }
}

void Neu_Label::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Rect r = bounds();
    const auto off = textOffset_;
    if (borderVisible_) {
        HPEN pen = CreatePen(PS_SOLID, 1, rgb(theme.border));
        HGDIOBJ oldPen = SelectObject(drawable, pen);
        HGDIOBJ oldBrush = SelectObject(drawable, GetStockObject(NULL_BRUSH));
        RoundRect(drawable, r.x, r.y, r.x + r.width, r.y + r.height, 8, 8);
        SelectObject(drawable, oldBrush);
        SelectObject(drawable, oldPen);
        DeleteObject(pen);
    }

    const int iconSize = icon().width() > 0 ? 16 : 0;
    const int iconSpace = iconSize > 0 ? iconSize + 6 : 0;
    const int contentLeft = r.x + off.left;
    const int contentTop = r.y + off.top;
    const int contentRight = r.x + r.width - off.right;
    const int contentBottom = r.y + r.height - off.bottom;
    const int textLeft = contentLeft + iconSpace;
    const int width = std::max(1, contentRight - textLeft);
    const int contentHeight = std::max(1, contentBottom - contentTop);

    int saved = SaveDC(drawable);
    IntersectClipRect(drawable, r.x, r.y, r.x + r.width, r.y + r.height);
    if (iconSize > 0) {
        drawIconBmp(d, drawable, gc, contentLeft, contentTop + std::max(0, (contentHeight - iconSize) / 2), iconSize);
    }
    IntersectClipRect(drawable, textLeft, contentTop, contentRight, contentBottom);

    int x = textLeft;
    if (!richTextFragments().empty()) {
        for (const auto& f : richTextFragments()) {
            if (x >= contentRight) {
                break;
            }
            const int remaining = std::max(1, contentRight - x);
            const std::string visible = truncateTextToWidth(d, drawable, gc, theme, f.text, remaining);
            drawTextColored(d,
                            drawable,
                            gc,
                            theme,
                            visible,
                            x,
                            contentTop + std::max(1, (contentHeight - 16) / 2),
                            f.useFontColor ? f.fontColor : theme.text,
                            f.bold,
                            f.italic,
                            f.underline,
                            f.strikethrough,
                            f.doubleStrikethrough,
                            f.monospace,
                            f.headingLevel);
            x += measureTextWidth(d, drawable, gc, theme, visible, f.bold, f.italic, f.monospace, f.headingLevel) + 2;
        }
    } else if (wordWrap_) {
        auto lines = wrapTextToWidth(d, drawable, gc, theme, text(), width);
        int y = contentTop + 2;
        for (const auto& line : lines) {
            if (y > contentBottom - 12) {
                break;
            }
            const std::string visible = truncateTextToWidth(d, drawable, gc, theme, line, width);
            drawText(d, drawable, gc, theme, visible, alignedTextX(d, drawable, gc, theme, visible, textLeft, width), y);
            y += 18;
        }
    } else {
        const std::string visible = truncateTextToWidth(d, drawable, gc, theme, text(), width);
        drawText(d, drawable, gc, theme, visible, alignedTextX(d, drawable, gc, theme, visible, textLeft, width), contentTop + std::max(1, (contentHeight - 16) / 2));
    }
    RestoreDC(drawable, saved);
    drawHintPopup(d, drawable, gc, theme);
}

void Neu_MultilineLabel::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Rect r = bounds();
    const auto off = textOffset_;
    if (borderVisible_) {
        HPEN pen = CreatePen(PS_SOLID, 1, rgb(theme.border));
        HGDIOBJ oldPen = SelectObject(drawable, pen);
        HGDIOBJ oldBrush = SelectObject(drawable, GetStockObject(NULL_BRUSH));
        RoundRect(drawable, r.x, r.y, r.x + r.width, r.y + r.height, 8, 8);
        SelectObject(drawable, oldBrush);
        SelectObject(drawable, oldPen);
        DeleteObject(pen);
    }
    const int iconSize = icon().width() > 0 ? 20 : 0;
    const int iconSpace = iconSize > 0 ? iconSize + 6 : 0;
    const int contentLeft = r.x + off.left;
    const int contentTop = r.y + off.top;
    const int contentRight = r.x + r.width - off.right - 10;
    const int contentBottom = r.y + r.height - off.bottom;
    const int textLeft = contentLeft + iconSpace;
    const int contentWidth = std::max(1, contentRight - textLeft);
    int saved = SaveDC(drawable);
    IntersectClipRect(drawable, r.x, r.y, r.x + r.width, r.y + r.height);
    if (iconSize > 0) {
        drawIconBmp(d, drawable, gc, contentLeft, contentTop + 2, iconSize);
    }
    IntersectClipRect(drawable, textLeft, contentTop, contentRight, contentBottom);
    std::vector<std::string> lines;
    auto logical = logicalLinesWin32(text());
    for (const auto& base : logical) {
        auto wrapped = wrapTextToWidth(d, drawable, gc, theme, base, contentWidth);
        lines.insert(lines.end(), wrapped.begin(), wrapped.end());
    }
    int y = contentTop + 2 - scrollY();
    for (const auto& line : lines) {
        if (y >= contentTop && y < contentBottom - 12) {
            const std::string visible = truncateTextToWidth(d, drawable, gc, theme, line, contentWidth);
            drawText(d, drawable, gc, theme, visible, alignedTextX(d, drawable, gc, theme, visible, textLeft, contentWidth), y);
        }
        y += 18;
    }
    RestoreDC(drawable, saved);
    setAutoScroll(true);
    setVirtualSize(r.width, std::max(r.height, static_cast<int>(lines.size()) * 18 + off.top + off.bottom + 12));
    drawScrollbars(d, drawable, gc, theme);
    drawHintPopup(d, drawable, gc, theme);
}

void Neu_RichTextCode::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    Neu_Rect r = bounds();
    const int toolbarH = toolbarVisible_ ? 34 : 0;
    if (toolbarVisible_) {
        RECT tb{r.x + 2, r.y + 2, r.x + r.width - 2, r.y + toolbarH};
        HBRUSH b = CreateSolidBrush(RGB(224, 232, 244));
        FillRect(drawable, &tb, b);
        DeleteObject(b);
        const char* tools[] = {"B", "I", "U", "S", "DS", "H1", "H2", "Mono", "Font", "Text", "BG", "HL", "Left", "Center", "Right", "Wrap"};
        int tx = r.x + 8;
        for (const char* tool : tools) {
            RECT br{tx, r.y + 7, tx + 42, r.y + 27};
            FrameRect(drawable, &br, reinterpret_cast<HBRUSH>(GetStockObject(GRAY_BRUSH)));
            drawText(d, drawable, gc, theme, tool, tx + 4, r.y + 9);
            tx += 46;
            if (tx > r.x + r.width - 50) {
                break;
            }
        }
    }

    const int contentLeft = r.x + 56;
    const int contentRight = r.x + r.width - 14;
    const int contentTop = r.y + toolbarH + 4;
    const int contentBottom = r.y + r.height - 14;
    const int contentWidth = std::max(1, contentRight - contentLeft);
    int y = contentTop + 2 - scrollY();
    int maxWidth = r.width;
    int logicalLineNo = 1;

    int saved = SaveDC(drawable);
    IntersectClipRect(drawable, r.x + 4, contentTop, contentRight, contentBottom);

    auto drawCodeLine = [&](const std::string& line,
                            const Neu_TextFragment* fragment,
                            int lineHeight,
                            int lineNo) {
        const std::vector<std::string> visualLines = wordWrap_ ? wrapTextToWidth(d, drawable, gc, theme, line, contentWidth)
                                                              : std::vector<std::string>{line};
        for (const auto& visualLine : visualLines) {
            const bool visibleY = y >= contentTop && y < contentBottom;
            const bool bold = fragment ? fragment->bold : false;
            const bool italic = fragment ? fragment->italic : false;
            const bool underline = fragment ? fragment->underline : false;
            const bool strike = fragment ? fragment->strikethrough : false;
            const bool dstrike = fragment ? fragment->doubleStrikethrough : false;
            const bool mono = fragment ? fragment->monospace : true;
            const int heading = fragment ? fragment->headingLevel : 0;
            const Neu_Color color = fragment && fragment->useFontColor ? fragment->fontColor : defaultFontColor_;
            const int measured = measureTextWidth(d, drawable, gc, theme, visualLine, bold, italic, mono, heading);
            maxWidth = std::max(maxWidth, measured + 82);
            if (visibleY) {
                drawText(d, drawable, gc, theme, std::to_string(lineNo), r.x + 8, y);
                int textSaved = SaveDC(drawable);
                IntersectClipRect(drawable, contentLeft, contentTop, contentRight, contentBottom);
                if (fragment && fragment->useHighlightColor) {
                    HBRUSH hb = CreateSolidBrush(rgb(fragment->highlightColor));
                    RECT hr{contentLeft - scrollX(), y - lineHeight + 5, std::min(contentRight, contentLeft - scrollX() + measured + 4), y + 4};
                    FillRect(drawable, &hr, hb);
                    DeleteObject(hb);
                }
                const std::string rendered = wordWrap_ ? visualLine : truncateTextToWidth(d, drawable, gc, theme, visualLine, std::max(contentWidth, measured));
                int tx = contentLeft - (wordWrap_ ? 0 : scrollX());
                if (textAlignment_ != Neu_TextAlignment::Left && wordWrap_) {
                    tx = alignedTextX(d, drawable, gc, theme, rendered, contentLeft, contentWidth);
                }
                drawTextColored(d, drawable, gc, theme, rendered, tx, y, color, bold, italic, underline, strike, dstrike, mono, heading);
                RestoreDC(drawable, textSaved);
            }
            y += lineHeight;
        }
    };

    if (!richTextFragments().empty()) {
        for (const auto& f : richTextFragments()) {
            const auto fragmentLines = logicalLinesWin32(f.text);
            const int lineHeight = lineHeightForHeadingWin32(f.headingLevel);
            for (const auto& line : fragmentLines) {
                drawCodeLine(line, &f, lineHeight, logicalLineNo++);
            }
        }
    } else {
        const auto lines = logicalLinesWin32(text_);
        for (const auto& line : lines) {
            drawCodeLine(line, nullptr, 20, logicalLineNo++);
        }
    }

    if (!readOnly_ && focused_) {
        const size_t caretBytes = std::min(cursor_, text_.size());
        size_t lineIndex = 0;
        size_t lineStart = 0;
        caretLinePrefixWin32(text_, caretBytes, lineIndex, lineStart);
        const std::string prefix = text_.substr(lineStart, caretBytes - lineStart);
        const int caretX = contentLeft + measureTextWidth(d, drawable, gc, theme, prefix, false, false, true) - (wordWrap_ ? 0 : scrollX());
        const int caretY = contentTop + 2 + static_cast<int>(lineIndex) * 20 - scrollY();
        if (caretY >= contentTop && caretY < contentBottom) {
            HPEN pen = CreatePen(PS_SOLID, 1, rgb(theme.focus));
            HGDIOBJ old = SelectObject(drawable, pen);
            const int caretTop = caretY;
            const int caretBottom = std::min(contentBottom, caretY + uiFontPixelHeightWin32(drawable, false, false, true));
            MoveToEx(drawable, caretX, caretTop, nullptr);
            LineTo(drawable, caretX, caretBottom);
            SelectObject(drawable, old);
            DeleteObject(pen);
        }
    }

    RestoreDC(drawable, saved);
    setAutoScroll(true);
    setVirtualSize(std::max(r.width, maxWidth), std::max(r.height, y - r.y + scrollY() + 20));
    drawScrollbars(d, drawable, gc, theme);
    drawHintPopup(d, drawable, gc, theme);
}

void Neu_RichTextCode::handleXEvent(XEvent& ev)
{
    if (readOnly_) {
        Neu_Control::handleXEvent(ev);
        return;
    }

    if (ev.message == WM_LBUTTONDOWN && contains(ev.x, ev.y)) {
        Neu_Control::handleXEvent(ev);
        if (activeScrollDrag_ != 0) {
            return;
        }
        Neu_Rect r = bounds();
        const int toolbarH = toolbarVisible_ ? 34 : 0;
        constexpr int lineHeight = 20;
        const int contentLeft = r.x + 56;
        const int contentTop = r.y + toolbarH + 4;
        const int contentBottom = r.y + r.height - 14;
        if (ev.y >= contentTop && ev.y <= contentBottom) {
            const int desiredLine = std::max(0, (ev.y - (contentTop + 2) + scrollY_) / lineHeight);
            const auto lines = logicalLinesWin32(text_);
            const int clampedLine = std::min(desiredLine, std::max(0, static_cast<int>(lines.size()) - 1));
            const size_t start = lineStartOffsetWin32(text_, clampedLine);
            const size_t end = lineEndOffsetWin32(text_, start);
            const int localX = ev.x - contentLeft + (wordWrap_ ? 0 : scrollX_);
            HDC hdc = parent_ && parent_->xid() ? GetDC(parent_->xid()) : nullptr;
            cursor_ = static_cast<size_t>(byteOffsetFromXWin32(hdc, text_, start, end, localX, true));
            if (hdc) {
                ReleaseDC(parent_->xid(), hdc);
            }
            requestRedraw();
            return;
        }
    }

    if (focused_ && ev.message == WM_CHAR && ev.wParam == L'\b') {
        if (cursor_ > 0 && !text_.empty()) {
            cursor_ = std::min(cursor_, text_.size());
            size_t eraseAt = cursor_ - 1;
            size_t eraseCount = 1;
            if (cursor_ >= 2 && text_[cursor_ - 2] == '\r' && text_[cursor_ - 1] == '\n') {
                eraseAt = cursor_ - 2;
                eraseCount = 2;
            }
            text_.erase(eraseAt, eraseCount);
            cursor_ = eraseAt;
            invokeTextChanged();

            size_t lineIndex = 0;
            size_t lineStart = 0;
            caretLinePrefixWin32(text_, cursor_, lineIndex, lineStart);
            const size_t lineEnd = lineEndOffsetWin32(text_, lineStart);
            if (cursor_ > lineEnd) {
                cursor_ = lineEnd;
            }

            HDC hdc = parent_ && parent_->xid() ? GetDC(parent_->xid()) : nullptr;
            int prefixWidth = 0;
            if (cursor_ > lineStart) {
                std::string prefix = text_.substr(lineStart, cursor_ - lineStart);
                while (!prefix.empty() && (prefix.back() == '\r' || prefix.back() == '\n')) {
                    prefix.pop_back();
                }
                prefixWidth = hdc ? textWidthWin32(hdc, prefix, true) : static_cast<int>(prefix.size()) * 8;
            }
            if (hdc) {
                ReleaseDC(parent_->xid(), hdc);
            }

            const Neu_Rect r = bounds();
            const int contentWidth = std::max(1, r.width - 70);
            if (prefixWidth <= contentWidth - 12) {
                scrollX_ = 0;
            } else if (prefixWidth - scrollX_ > contentWidth - 8) {
                scrollX_ = std::max(0, prefixWidth - contentWidth + 8);
            } else if (prefixWidth < scrollX_ + 4) {
                scrollX_ = std::max(0, prefixWidth - 8);
            }
            requestRedraw();
        }
        return;
    }

    Neu_Multilinetextbox::handleXEvent(ev);
}

void Neu_ProgressSquare::setProgress(float progress)
{
    progress_ = std::max(0.0f, std::min(1.0f, progress));
    requestRedraw();
}

void Neu_ProgressSquare::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    Neu_Rect r = bounds();
    const int side = std::max(8, std::min(r.width, r.height) - 18);
    const int x = r.x + (r.width - side) / 2;
    const int y = r.y + (r.height - side) / 2;
    const int cx = x + side / 2;
    const int perimeter = side / 2 + side + side + side + side / 2;
    int remaining = static_cast<int>(perimeter * progress_);
    HPEN pen = CreatePen(PS_SOLID, 5, rgb(progress_ > 0.85f ? Neu_Color{255, 215, 60, 255} : theme.accent));
    HGDIOBJ old = SelectObject(drawable, pen);
    auto drawPart = [&](int x1, int y1, int x2, int y2) {
        if (remaining <= 0) return;
        int len = std::max(std::abs(x2 - x1), std::abs(y2 - y1));
        int used = std::min(remaining, len);
        int ex = x1 + (len ? (x2 - x1) * used / len : 0);
        int ey = y1 + (len ? (y2 - y1) * used / len : 0);
        MoveToEx(drawable, x1, y1, nullptr);
        LineTo(drawable, ex, ey);
        remaining -= used;
    };
    drawPart(cx, y, x + side, y);
    drawPart(x + side, y, x + side, y + side);
    drawPart(x + side, y + side, x, y + side);
    drawPart(x, y + side, x, y);
    drawPart(x, y, cx, y);
    SelectObject(drawable, old);
    DeleteObject(pen);
    drawText(d, drawable, gc, theme, std::to_string(static_cast<int>(progress_ * 100.0f)) + "%", x + side / 2 - 16, y + side / 2 - 8);
}

void Neu_ImageView::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    Neu_Rect r = bounds();
    drawIconBmp(d, drawable, gc, r.x + 6, r.y + 6, std::min(r.width, r.height) - 12);
}

void Neu_ReadOnlyRichText::setIconList(const std::vector<std::string>& bmpIconPaths)
{
    iconPaths_ = bmpIconPaths;
}

size_t Neu_ReadOnlyRichText::iconIndexForText(const std::string& text) const
{
    size_t c = 0;
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '#' && (i == 0 || text[i - 1] != '\\')) {
            ++c;
        }
    }
    return iconPaths_.empty() ? 0 : std::min(c, iconPaths_.size() - 1);
}

void Neu_ReadOnlyRichText::crlf()
{
    contentCursorX_ = 12;
    contentCursorY_ += 30 + labelLineSpacing_;
    appendNextInline_ = false;
}

void Neu_ReadOnlyRichText::addLabel(const std::string& text)
{
    const std::string clean = unescapeHashesWin32(text);
    const int available = std::max(40, layout().width - 36);
    int width = std::min(available, std::max(60, static_cast<int>(clean.size()) * 8 + 30));
    if (!appendNextInline_) {
        contentCursorX_ = 12;
    }
    if (contentCursorX_ + width > layout().width - 12) {
        contentCursorX_ = 12;
        contentCursorY_ += 30 + labelLineSpacing_;
    }
    auto l = std::make_shared<Neu_Label>(Neu_Layout{layout().left + contentCursorX_, layout().top + contentCursorY_, width, 28});
    l->setText(clean);
    l->setTextTruncation(true);
    if (!iconPaths_.empty()) {
        l->setIconBmp(iconPaths_[iconIndexForText(text)]);
    }
    add(l);
    if (appendNextInline_) {
        contentCursorX_ += width + labelSpacing_;
        appendNextInline_ = false;
    } else {
        contentCursorX_ = 12;
        contentCursorY_ += 30 + labelLineSpacing_;
    }
    setContentSize(layout().width, contentCursorY_ + 50);
}

void Neu_ReadOnlyRichText::addMultilineLabel(const std::string& text)
{
    const std::string clean = unescapeHashesWin32(text);
    const int available = std::max(40, layout().width - 36);
    if (!appendNextInline_) {
        contentCursorX_ = 12;
    }
    int estimatedLines = 1;
    for (char ch : clean) {
        if (ch == '\n') ++estimatedLines;
    }
    estimatedLines += static_cast<int>(clean.size()) / std::max(1, available / 8);
    const int height = std::max(56, estimatedLines * (18 + labelLineSpacing_) + 12);
    auto l = std::make_shared<Neu_MultilineLabel>(Neu_Layout{layout().left + contentCursorX_, layout().top + contentCursorY_, available, height});
    l->setText(clean);
    l->setWordWrap(true);
    l->setAutoScroll(true);
    if (!iconPaths_.empty()) {
        l->setIconBmp(iconPaths_[iconIndexForText(text)]);
    }
    add(l);
    if (appendNextInline_) {
        contentCursorX_ += available + labelSpacing_;
        appendNextInline_ = false;
    } else {
        contentCursorX_ = 12;
        contentCursorY_ += height + labelLineSpacing_;
    }
    setContentSize(layout().width, contentCursorY_ + 50);
}

namespace {
static std::string truncateApproxWin(const std::string& text, int width)
{
    const int maxChars = std::max(0, width / 7);
    if (static_cast<int>(text.size()) <= maxChars) {
        return text;
    }
    if (maxChars <= 3) {
        return text.substr(0, static_cast<size_t>(maxChars));
    }
    return text.substr(0, static_cast<size_t>(maxChars - 3)) + "...";
}

static void drawPlainTextWin(HDC hdc,
                             const Neu_Theme& theme,
                             const std::string& text,
                             int left,
                             int top,
                             int right,
                             int bottom,
                             COLORREF color,
                             bool underline = false)
{
    RECT clip{left, top, std::max(left + 1, right), std::max(top + 1, bottom)};
    int saved = SaveDC(hdc);
    IntersectClipRect(hdc, clip.left, clip.top, clip.right, clip.bottom);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, color);
    HFONT font = createTextFont(false, false, underline, false, false, 0);
    HGDIOBJ oldFont = SelectObject(hdc, font ? font : uiFont());
    const std::string visible = truncateApproxWin(text, clip.right - clip.left);
    std::wstring w = toWide(visible);
    ExtTextOutW(hdc, clip.left, clip.top, ETO_CLIPPED, &clip, w.c_str(), static_cast<UINT>(w.size()), nullptr);
    SelectObject(hdc, oldFont);
    if (font) {
        DeleteObject(font);
    }
    (void)theme;
    RestoreDC(hdc, saved);
}
}

void Neu_CheckBox::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    (void)d;
    (void)gc;
    Neu_Rect r = bounds();
    RECT bg{r.x, r.y, r.x + r.width, r.y + r.height};
    HBRUSH b = CreateSolidBrush(rgb(hover_ ? theme.highlight : theme.glass));
    FillRect(drawable, &bg, b);
    DeleteObject(b);

    const int box = std::max(12, std::min(18, r.height - 8));
    RECT cb{r.x + 4, r.y + (r.height - box) / 2, r.x + 4 + box, r.y + (r.height - box) / 2 + box};
    drawSupersampledRoundRectWin32(drawable,
                                   cb,
                                   4,
                                   rgb(hover_ ? theme.hover : theme.background),
                                   rgb(theme.glass),
                                   rgb(focused_ ? theme.focus : theme.border),
                                   true);
    if (checked_) {
        HPEN pen = CreatePen(PS_SOLID, 2, rgb(theme.focus));
        HGDIOBJ old = SelectObject(drawable, pen);
        SetBkMode(drawable, TRANSPARENT);
        MoveToEx(drawable, cb.left + 3, cb.top + box / 2, nullptr);
        LineTo(drawable, cb.left + box / 2, cb.bottom - 3);
        LineTo(drawable, cb.right - 3, cb.top + 3);
        MoveToEx(drawable, cb.left + 3, cb.top + box / 2 + 1, nullptr);
        LineTo(drawable, cb.left + box / 2, cb.bottom - 2);
        SelectObject(drawable, old);
        DeleteObject(pen);
    }
    const int left = cb.right + 7 + textOffset_.left;
    drawPlainTextWin(drawable,
                     theme,
                     text_,
                     left,
                     r.y + textOffset_.top + std::max(0, (r.height - 16) / 2),
                     r.x + r.width - textOffset_.right,
                     r.y + r.height - textOffset_.bottom,
                     rgb(theme.text));
    drawHintPopup(d, drawable, gc, theme);
}

void Neu_CheckBox::handleXEvent(XEvent& ev)
{
    Neu_Control::handleXEvent(ev);
    if (ev.message == WM_LBUTTONUP && contains(ev.x, ev.y)) {
        checked_ = !checked_;
        invokeClick();
        requestRedraw();
    }
}

void Neu_RadioButton::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    (void)d;
    (void)gc;
    Neu_Rect r = bounds();
    RECT bg{r.x, r.y, r.x + r.width, r.y + r.height};
    HBRUSH b = CreateSolidBrush(rgb(hover_ ? theme.highlight : theme.glass));
    FillRect(drawable, &bg, b);
    DeleteObject(b);
    const int box = std::max(12, std::min(18, r.height - 8));
    const int x = r.x + 4;
    const int y = r.y + (r.height - box) / 2;
    RECT outer{x, y, x + box, y + box};
    drawSupersampledEllipseWin32(drawable,
                                 outer,
                                 rgb(hover_ ? theme.hover : theme.background),
                                 rgb(theme.glass),
                                 rgb(focused_ ? theme.focus : theme.border),
                                 true);
    if (checked_) {
        RECT dot{x + 4, y + 4, x + box - 4, y + box - 4};
        drawSupersampledEllipseWin32(drawable, dot, rgb(theme.glass), rgb(theme.focus), rgb(theme.focus), true);
    }
    const int left = x + box + 7 + textOffset_.left;
    drawPlainTextWin(drawable,
                     theme,
                     text_,
                     left,
                     r.y + textOffset_.top + std::max(0, (r.height - 16) / 2),
                     r.x + r.width - textOffset_.right,
                     r.y + r.height - textOffset_.bottom,
                     rgb(theme.text));
    drawHintPopup(d, drawable, gc, theme);
}


void Neu_ToggleButton::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    (void)d;
    (void)gc;
    Neu_Rect r = bounds();
    fillRound(drawable,
              r,
              theme.radius,
              rgb(checked_ ? theme.pressed : (hover_ ? theme.hover : theme.glass)),
              rgb(focused_ ? theme.focus : theme.border));

    const int left = r.x + textOffset_.left;
    const int top = r.y + textOffset_.top;
    const int right = r.x + r.width - textOffset_.right;
    const int bottom = r.y + r.height - textOffset_.bottom;
    const int width = std::max(1, right - left);
    const int height = std::max(1, bottom - top);

    const int saved = SaveDC(drawable);
    IntersectClipRect(drawable, left, top, right, bottom);
    const std::string label = text_.empty() ? (checked_ ? "On" : "Off") : text_;
    const std::string visible = truncateTextToWidth(d, drawable, gc, theme, label, width);
    drawText(d,
             drawable,
             gc,
             theme,
             visible,
             alignedTextX(d, drawable, gc, theme, visible, left, width),
             top + std::max(1, (height - 16) / 2));
    RestoreDC(drawable, saved);
    drawHintPopup(d, drawable, gc, theme);
}

void Neu_ProgressBar::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    Neu_Rect r = bounds();
    RECT inner{r.x + 5, r.y + 5, r.x + r.width - 5, r.y + r.height - 5};
    const int innerW = std::max(1, static_cast<int>(inner.right - inner.left));
    const int innerH = std::max(1, static_cast<int>(inner.bottom - inner.top));
    const int fillW = std::max(0, static_cast<int>(innerW * progress_));
    if (fillW > 0) {
        fillRound(drawable,
                  Neu_Rect{inner.left, inner.top, fillW, innerH},
                  std::max(2, theme.radius - 4),
                  rgb(theme.focus),
                  rgb(theme.focus));
    }
    drawPlainTextWin(drawable, theme, text_.empty() ? std::to_string(static_cast<int>(progress_ * 100.0f)) + "%" : text_, inner.left + 4, inner.top + 2, inner.right - 4, inner.bottom - 2, rgb(theme.text));
    drawHintPopup(d, drawable, gc, theme);
}

void Neu_Slider::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    Neu_Rect r = bounds();
    const float t = (max_ == min_) ? 0.0f : static_cast<float>(value_ - min_) / static_cast<float>(max_ - min_);
    HPEN track = CreatePen(PS_SOLID, 2, rgb(theme.border));
    HGDIOBJ old = SelectObject(drawable, track);
    if (vertical_) {
        const int x = r.x + r.width / 2;
        MoveToEx(drawable, x, r.y + 8, nullptr);
        LineTo(drawable, x, r.y + r.height - 8);
        SelectObject(drawable, old);
        DeleteObject(track);
        HBRUSH knob = CreateSolidBrush(rgb(theme.focus));
        HGDIOBJ oldBrush = SelectObject(drawable, knob);
        const int y = r.y + 8 + static_cast<int>((r.height - 16) * (1.0f - t));
        Ellipse(drawable, x - 7, y - 7, x + 7, y + 7);
        SelectObject(drawable, oldBrush);
        DeleteObject(knob);
    } else {
        const int y = r.y + r.height / 2;
        MoveToEx(drawable, r.x + 8, y, nullptr);
        LineTo(drawable, r.x + r.width - 8, y);
        SelectObject(drawable, old);
        DeleteObject(track);
        HBRUSH knob = CreateSolidBrush(rgb(theme.focus));
        HGDIOBJ oldBrush = SelectObject(drawable, knob);
        const int x = r.x + 8 + static_cast<int>((r.width - 16) * t);
        Ellipse(drawable, x - 7, y - 7, x + 7, y + 7);
        SelectObject(drawable, oldBrush);
        DeleteObject(knob);
    }
    drawHintPopup(d, drawable, gc, theme);
}

void Neu_Slider::handleXEvent(XEvent& ev)
{
    Neu_Control::handleXEvent(ev);
    if (ev.message == WM_LBUTTONDOWN && contains(ev.x, ev.y)) {
        dragging_ = true;
    }
    if (ev.message == WM_LBUTTONUP) {
        dragging_ = false;
    }
    if ((dragging_ && ev.message == WM_MOUSEMOVE) || (ev.message == WM_LBUTTONDOWN && contains(ev.x, ev.y))) {
        Neu_Rect r = bounds();
        float t = vertical_ ? 1.0f - static_cast<float>(ev.y - r.y - 8) / static_cast<float>(std::max(1, r.height - 16))
                            : static_cast<float>(ev.x - r.x - 8) / static_cast<float>(std::max(1, r.width - 16));
        t = std::max(0.0f, std::min(1.0f, t));
        setValue(min_ + static_cast<int>((max_ - min_) * t + 0.5f));
    }
}

void Neu_Spinner::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    Neu_Rect r = bounds();
    const int buttonW = 22;
    const int buttonLeft = r.x + r.width - buttonW;
    drawPlainTextWin(drawable, theme, text_, r.x + 8 + textOffset_.left, r.y + 4 + textOffset_.top, buttonLeft - textOffset_.right, r.y + r.height - 4 - textOffset_.bottom, rgb(theme.text));
    HPEN pen = CreatePen(PS_SOLID, 1, rgb(theme.border));
    HGDIOBJ oldPen = SelectObject(drawable, pen);
    MoveToEx(drawable, buttonLeft, r.y + 3, nullptr);
    LineTo(drawable, buttonLeft, r.y + r.height - 3);
    SelectObject(drawable, oldPen);
    DeleteObject(pen);
    const int arrowX = buttonLeft + buttonW / 2;
    drawArrowWin32(drawable, arrowX, r.y + std::max(8, r.height / 4), true, rgb(theme.text));
    drawArrowWin32(drawable, arrowX, r.y + r.height - std::max(8, r.height / 4), false, rgb(theme.text));
    drawHintPopup(d, drawable, gc, theme);
}

void Neu_Spinner::handleXEvent(XEvent& ev)
{
    Neu_Control::handleXEvent(ev);
    if (ev.message == WM_LBUTTONUP && contains(ev.x, ev.y)) {
        Neu_Rect r = bounds();
        if (ev.x >= r.x + r.width - 24) {
            setValue(value_ + (ev.y < r.y + r.height / 2 ? 1 : -1));
            invokeTextChanged();
            invokeClick();
        }
    }
}

void Neu_GroupBox::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Rect r = bounds();
    HPEN pen = CreatePen(PS_SOLID, 1, rgb(theme.border));
    HGDIOBJ oldPen = SelectObject(drawable, pen);
    HGDIOBJ oldBrush = SelectObject(drawable, GetStockObject(NULL_BRUSH));
    Rectangle(drawable, r.x, r.y + 8, r.x + r.width, r.y + r.height);
    SelectObject(drawable, oldBrush);
    SelectObject(drawable, oldPen);
    DeleteObject(pen);
    drawText(d, drawable, gc, theme, text_, r.x + 10 + textOffset_.left, r.y + 2 + textOffset_.top);
    for (auto& c : children()) {
        if (c && c->visible()) {
            c->draw(d, drawable, gc, theme);
        }
    }
    drawHintPopup(d, drawable, gc, theme);
}

void Neu_Separator::draw(Display*, Drawable drawable, GC, const Neu_Theme& theme)
{
    Neu_Rect r = bounds();
    HPEN pen = CreatePen(PS_SOLID, 1, rgb(theme.border));
    HGDIOBJ old = SelectObject(drawable, pen);
    if (vertical_) {
        const int x = r.x + r.width / 2;
        MoveToEx(drawable, x, r.y, nullptr);
        LineTo(drawable, x, r.y + r.height);
    } else {
        const int y = r.y + r.height / 2;
        MoveToEx(drawable, r.x, y, nullptr);
        LineTo(drawable, r.x + r.width, y);
    }
    SelectObject(drawable, old);
    DeleteObject(pen);
}

void Neu_LinkLabel::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    (void)d;
    (void)gc;
    Neu_Rect r = bounds();
    drawPlainTextWin(drawable, theme, text_, r.x + textOffset_.left, r.y + textOffset_.top + std::max(0, (r.height - 16) / 2), r.x + r.width - textOffset_.right, r.y + r.height - textOffset_.bottom, rgb(theme.focus), true);
    drawHintPopup(d, drawable, gc, theme);
}

void Neu_LinkLabel::handleXEvent(XEvent& ev)
{
    Neu_Control::handleXEvent(ev);
    if (ev.message == WM_LBUTTONUP && contains(ev.x, ev.y)) {
        invokeClick();
    }
}

void Neu_ToolBar::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    for (auto& c : children()) {
        if (c && c->visible()) {
            c->draw(d, drawable, gc, theme);
        }
    }
    drawHintPopup(d, drawable, gc, theme);
}

int Neu_TabView::addTab(const std::string& title, std::shared_ptr<Neu_Placement> page)
{
    titles_.push_back(title);
    pages_.push_back(page);
    if (page) {
        page->setParent(parent_);
    }
    return static_cast<int>(titles_.size()) - 1;
}

void Neu_TabView::setParent(Neu_Window* parent)
{
    Neu_Control::setParent(parent);
    for (auto& page : pages_) {
        if (page) {
            page->setParent(parent);
        }
    }
}

void Neu_TabView::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    Neu_Rect r = bounds();
    const int tabH = 26;
    int x = r.x + 6;
    for (size_t i = 0; i < titles_.size(); ++i) {
        const int w = std::max(72, static_cast<int>(titles_[i].size()) * 8 + 18);
        RECT tr{x, r.y + 4, x + w, r.y + 4 + tabH};
        HBRUSH b = CreateSolidBrush(rgb(static_cast<int>(i) == selectedTab_ ? theme.pressed : theme.hover));
        FillRect(drawable, &tr, b);
        DeleteObject(b);
        FrameRect(drawable, &tr, reinterpret_cast<HBRUSH>(GetStockObject(GRAY_BRUSH)));
        drawPlainTextWin(drawable, theme, titles_[i], x + 8, r.y + 8, x + w - 4, r.y + 4 + tabH, rgb(theme.text));
        x += w + 2;
    }
    if (selectedTab_ >= 0 && selectedTab_ < static_cast<int>(pages_.size()) && pages_[static_cast<size_t>(selectedTab_)]) {
        pages_[static_cast<size_t>(selectedTab_)]->draw(d, drawable, gc, theme);
    }
    drawHintPopup(d, drawable, gc, theme);
}

void Neu_TabView::handleXEvent(XEvent& ev)
{
    Neu_Rect r = bounds();
    if ((ev.message == WM_LBUTTONDOWN || ev.message == WM_LBUTTONUP) && contains(ev.x, ev.y)) {
        int x = r.x + 6;
        for (size_t i = 0; i < titles_.size(); ++i) {
            const int w = std::max(72, static_cast<int>(titles_[i].size()) * 8 + 18);
            if (ev.y >= r.y + 4 && ev.y <= r.y + 30 && ev.x >= x && ev.x <= x + w) {
                selectedTab_ = static_cast<int>(i);
                requestRedraw();
                return;
            }
            x += w + 2;
        }
    }
    if (selectedTab_ >= 0 && selectedTab_ < static_cast<int>(pages_.size()) && pages_[static_cast<size_t>(selectedTab_)]) {
        pages_[static_cast<size_t>(selectedTab_)]->handleXEvent(ev);
        return;
    }
    Neu_Control::handleXEvent(ev);
}

void Neu_Splitter::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    Neu_Rect r = bounds();
    const int split = vertical_ ? std::max(24, std::min(splitPosition_, r.width - 24))
                                : std::max(24, std::min(splitPosition_, r.height - 24));
    const int splitAbs = vertical_ ? r.x + split : r.y + split;

    for (auto& child : children()) {
        if (!child || !child->visible()) {
            continue;
        }
        Neu_Rect cr = child->bounds();
        int saved = SaveDC(drawable);
        if (vertical_) {
            const bool leftPane = cr.x + cr.width / 2 < splitAbs;
            const int clipLeft = leftPane ? r.x + 2 : splitAbs + 2;
            const int clipRight = leftPane ? splitAbs - 2 : r.x + r.width - 2;
            if (clipRight > clipLeft) {
                IntersectClipRect(drawable, clipLeft, r.y + 2, clipRight, r.y + r.height - 2);
                child->draw(d, drawable, gc, theme);
            }
        } else {
            const bool topPane = cr.y + cr.height / 2 < splitAbs;
            const int clipTop = topPane ? r.y + 2 : splitAbs + 2;
            const int clipBottom = topPane ? splitAbs - 2 : r.y + r.height - 2;
            if (clipBottom > clipTop) {
                IntersectClipRect(drawable, r.x + 2, clipTop, r.x + r.width - 2, clipBottom);
                child->draw(d, drawable, gc, theme);
            }
        }
        RestoreDC(drawable, saved);
    }

    HBRUSH b = CreateSolidBrush(rgb(theme.focus));
    if (vertical_) {
        RECT sr{splitAbs - 2, r.y + 4, splitAbs + 2, r.y + r.height - 4};
        FillRect(drawable, &sr, b);
    } else {
        RECT sr{r.x + 4, splitAbs - 2, r.x + r.width - 4, splitAbs + 2};
        FillRect(drawable, &sr, b);
    }
    DeleteObject(b);
    drawHintPopup(d, drawable, gc, theme);
}

void Neu_Splitter::handleXEvent(XEvent& ev)
{
    Neu_Rect r = bounds();
    const int sash = 5;
    const int split = vertical_ ? std::max(24, std::min(splitPosition_, r.width - 24))
                                : std::max(24, std::min(splitPosition_, r.height - 24));
    const int splitAbs = vertical_ ? r.x + split : r.y + split;
    const bool onSash = vertical_
                        ? (ev.x >= splitAbs - sash && ev.x <= splitAbs + sash && ev.y >= r.y && ev.y <= r.y + r.height)
                        : (ev.y >= splitAbs - sash && ev.y <= splitAbs + sash && ev.x >= r.x && ev.x <= r.x + r.width);

    if (ev.message == WM_LBUTTONDOWN && onSash) {
        dragging_ = true;
        return;
    }
    if (ev.message == WM_LBUTTONUP) {
        dragging_ = false;
    }
    if (dragging_ && ev.message == WM_MOUSEMOVE) {
        setSplitPosition(vertical_ ? ev.x - r.x : ev.y - r.y);
        return;
    }

    if (ev.message == WM_MOUSEMOVE || ev.message == WM_LBUTTONDOWN || ev.message == WM_LBUTTONUP) {
        for (auto it = children().rbegin(); it != children().rend(); ++it) {
            auto& child = *it;
            if (!child || !child->visible() || !child->enabled()) {
                continue;
            }
            const Neu_Rect cr = child->bounds();
            const bool firstPane = vertical_ ? (cr.x + cr.width / 2 < splitAbs) : (cr.y + cr.height / 2 < splitAbs);
            const bool pointInPane = vertical_
                                     ? (firstPane ? ev.x < splitAbs - sash : ev.x > splitAbs + sash)
                                     : (firstPane ? ev.y < splitAbs - sash : ev.y > splitAbs + sash);
            if (pointInPane && child->contains(ev.x, ev.y)) {
                child->handleXEvent(ev);
                return;
            }
        }
    }

    Neu_Control::handleXEvent(ev);
}


Neu_Window::Neu_Window(Neu_Application& app, int width, int height, const std::string& title)
    : app_(app),
      display_(nullptr),
      width_(std::max(1, width)),
      height_(std::max(1, height)),
      title_(title),
      theme_(Neu_Theme::MaterialDark())
{
    Neu_ApplyThemeRenderingOptions(theme_);
}

Neu_Window::~Neu_Window()
{
    close();
}

bool Neu_Window::create()
{
    const DWORD style = WS_OVERLAPPEDWINDOW;
    const DWORD exStyle = WS_EX_APPWINDOW;
    RECT wr{0, 0, width_, height_};
    AdjustWindowRectEx(&wr, style, FALSE, exStyle);

    HWND hwnd = CreateWindowExW(exStyle,
                                kNeuWindowClass,
                                toWide(title_).c_str(),
                                style,
                                CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                wr.right - wr.left,
                                wr.bottom - wr.top,
                                nullptr,
                                nullptr,
                                g_instance ? g_instance : GetModuleHandleW(nullptr),
                                this);
    if (!hwnd) {
        return false;
    }

    window_ = hwnd;
    RECT client{};
    GetClientRect(hwnd, &client);
    width_ = std::max(1, static_cast<int>(client.right - client.left));
    height_ = std::max(1, static_cast<int>(client.bottom - client.top));
    gc_ = GetDC(hwnd);
    app_.registerWindow(this);
    return true;
}

void Neu_Window::show()
{
    ShowWindow(window_, SW_SHOW);
    UpdateWindow(window_);
}

void Neu_Window::releaseBuffers()
{
    if (oldBitmap_ && memoryDc_) {
        SelectObject(memoryDc_, oldBitmap_);
    }
    if (backBitmap_) {
        DeleteObject(backBitmap_);
    }
    if (memoryDc_) {
        DeleteDC(memoryDc_);
    }
    oldBitmap_ = nullptr;
    backBitmap_ = nullptr;
    memoryDc_ = nullptr;
    bufferWidth_ = 0;
    bufferHeight_ = 0;
}

void Neu_Window::close()
{
    if (!window_ || closing_) {
        return;
    }

    closing_ = true;
    setFocusedControl(nullptr);
    captureControl_ = nullptr;
    releaseBuffers();

    HWND hwnd = window_;
    HDC dc = gc_;
    window_ = nullptr;
    gc_ = nullptr;
    app_.unregisterWindow(this);

    if (dc) {
        ReleaseDC(hwnd, dc);
    }

    DestroyWindow(hwnd);

    if (!app_.hasWindows()) {
        app_.quit();
    }
}

void Neu_Window::ensureBuffers()
{
    if (!window_ || width_ <= 0 || height_ <= 0) {
        return;
    }
    if (bufferWidth_ == width_ && bufferHeight_ == height_ && memoryDc_ && backBitmap_) {
        return;
    }

    releaseBuffers();
    HDC dc = GetDC(window_);
    memoryDc_ = CreateCompatibleDC(dc);
    backBitmap_ = CreateCompatibleBitmap(dc, width_, height_);
    oldBitmap_ = static_cast<HBITMAP>(SelectObject(memoryDc_, backBitmap_));
    ReleaseDC(window_, dc);
    bufferWidth_ = width_;
    bufferHeight_ = height_;
}

void Neu_Window::setTheme(const Neu_Theme& theme)
{
    theme_ = theme;
    Neu_ApplyThemeRenderingOptions(theme_);
    requestRedraw();
}

void Neu_Window::drawScene(Drawable target)
{
    Neu_SetCurrentDrawingTheme(theme_);
    RECT rc{0, 0, width_, height_};
    HBRUSH b = CreateSolidBrush(rgb(theme_.background));
    FillRect(target, &rc, b);
    DeleteObject(b);

    for (auto& c : controls_) {
        if (c->visible()) {
            c->drawShadow(nullptr, target, target, theme_);
        }
    }
    for (auto& c : controls_) {
        if (c->visible()) {
            c->draw(nullptr, target, target, theme_);
        }
    }
    for (auto& c : controls_) {
        if (c->visible()) {
            c->drawHintPopup(nullptr, target, target, theme_);
        }
    }
}

void Neu_Window::paint(Drawable target)
{
    Neu_SetCurrentDrawingTheme(theme_);
    if (!target || width_ <= 0 || height_ <= 0) {
        return;
    }

    if (multiStageDoubleBuffering_ && g_options.multiStageDoubleBuffering) {
        ensureBuffers();
        if (memoryDc_) {
            drawScene(memoryDc_);
            BitBlt(target, 0, 0, width_, height_, memoryDc_, 0, 0, SRCCOPY);
            return;
        }
    }

    drawScene(target);
}

void Neu_Window::redraw()
{
    if (!window_) {
        return;
    }
    HDC dc = GetDC(window_);
    paint(dc);
    ReleaseDC(window_, dc);
}

void Neu_Window::invalidate()
{
    if (window_) {
        InvalidateRect(window_, nullptr, FALSE);
    }
}

void Neu_Window::requestRedraw()
{
    invalidate();
}

void Neu_Window::setMultiStageDoubleBuffering(bool enabled)
{
    multiStageDoubleBuffering_ = enabled;
    if (!enabled) {
        releaseBuffers();
    }
    requestRedraw();
}

void Neu_Window::add(std::shared_ptr<Neu_Control> control)
{
    if (control) {
        control->setParent(this);
        controls_.push_back(control);
    }
}

Neu_Control* Neu_Window::hitTest(int x, int y)
{
    auto hitOne = [&](auto&& self, const std::shared_ptr<Neu_Control>& control, int px, int py) -> Neu_Control* {
        if (!control || !control->visible() || !control->enabled()) {
            return nullptr;
        }
        if (!control->contains(px, py)) {
            return nullptr;
        }

        if (auto scroll = dynamic_cast<Neu_ScrollWindow*>(control.get())) {
            const auto sr = scroll->bounds();
            const int childX = px - (sr.x + 4) + scroll->scrollX();
            const int childY = py - (sr.y + 4) + scroll->scrollY();
            const auto& children = scroll->children();
            for (auto child = children.rbegin(); child != children.rend(); ++child) {
                Neu_Control* hit = self(self, *child, childX, childY);
                if (hit) {
                    return hit;
                }
            }
            return scroll;
        }

        if (auto placement = dynamic_cast<Neu_Placement*>(control.get())) {
            const auto& children = placement->children();
            for (auto child = children.rbegin(); child != children.rend(); ++child) {
                Neu_Control* hit = self(self, *child, px, py);
                if (hit) {
                    return hit;
                }
            }
        }

        return control.get();
    };

    for (auto it = controls_.rbegin(); it != controls_.rend(); ++it) {
        Neu_Control* hit = hitOne(hitOne, *it, x, y);
        if (hit) {
            return hit;
        }
    }
    return nullptr;
}


void Neu_Window::setFocusedControl(Neu_Control* control)
{
    if (focusedControl_ == control) {
        return;
    }
    if (focusedControl_) {
        focusedControl_->setFocused(false);
    }
    focusedControl_ = control;
    if (focusedControl_) {
        focusedControl_->setFocused(true);
    }
}

void Neu_Window::handleXEvent(XEvent& ev)
{
    if (ev.message == WM_CLOSE) {
        if (onClose_) {
            onClose_(this, closeUserData_);
        }
        close();
        return;
    }

    if (ev.message == WM_SIZE) {
        width_ = std::max(1, static_cast<int>(LOWORD(ev.lParam)));
        height_ = std::max(1, static_cast<int>(HIWORD(ev.lParam)));
        releaseBuffers();
        requestRedraw();
        return;
    }

    if (ev.message == WM_TIMER) {
        if (hoveredControl_) {
            requestRedraw();
        } else if (window_) {
            KillTimer(window_, 1);
        }
        return;
    }

    if (ev.message == WM_MOUSEMOVE) {
        TRACKMOUSEEVENT tme{};
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = window_;
        TrackMouseEvent(&tme);

        Neu_Control* hoverTarget = hitTest(ev.x, ev.y);
        Neu_Control* target = captureControl_ ? captureControl_ : hoverTarget;
        if (auto combo = dynamic_cast<Neu_ComboBox*>(focusedControl_)) {
            if (combo->isDropDownOpen()) {
                target = combo;
            }
        }
        if (hoverTarget != hoveredControl_) {
            if (hoveredControl_) {
                XEvent leave = ev;
                leave.message = WM_MOUSELEAVE;
                hoveredControl_->handleXEvent(leave);
            }
            hoveredControl_ = hoverTarget;
        }
        if (target) {
            target->handleXEvent(ev);
            SetTimer(window_, 1, 5000, nullptr);
        } else {
            KillTimer(window_, 1);
        }
        return;
    }

    if (ev.message == WM_MOUSELEAVE) {
        if (hoveredControl_) {
            hoveredControl_->handleXEvent(ev);
            hoveredControl_ = nullptr;
        }
        KillTimer(window_, 1);
        return;
    }

    if (ev.message == WM_LBUTTONDOWN) {
        SetFocus(window_);
        if (auto combo = dynamic_cast<Neu_ComboBox*>(focusedControl_)) {
            if (combo->isDropDownOpen()) {
                combo->handleXEvent(ev);
                if (combo->isDropDownOpen()) {
                    captureControl_ = combo;
                    SetCapture(window_);
                }
                return;
            }
        }
        SetCapture(window_);
        Neu_Control* target = hitTest(ev.x, ev.y);
        setFocusedControl(target);
        captureControl_ = target;
        if (target) {
            target->handleXEvent(ev);
        }
        return;
    }

    if (ev.message == WM_LBUTTONUP) {
        ReleaseCapture();
        Neu_Control* target = captureControl_ ? captureControl_ : hitTest(ev.x, ev.y);
        if (target) {
            target->handleXEvent(ev);
        }
        captureControl_ = nullptr;
        return;
    }

    if (ev.message == WM_MOUSEWHEEL || ev.message == WM_MOUSEHWHEEL) {
        if (auto combo = dynamic_cast<Neu_ComboBox*>(focusedControl_)) {
            if (combo->isDropDownOpen()) {
                combo->handleXEvent(ev);
                return;
            }
        }
        Neu_Control* target = hitTest(ev.x, ev.y);
        if (!target) {
            target = focusedControl_;
        }
        if (target) {
            target->handleXEvent(ev);
        }
        return;
    }

    if (ev.message == WM_KEYDOWN || ev.message == WM_CHAR) {
        if (focusedControl_ && focusedControl_->visible() && focusedControl_->enabled()) {
            focusedControl_->handleXEvent(ev);
        }
        return;
    }
}

} // namespace neutrino
#endif
