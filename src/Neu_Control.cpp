#include "Neutrino/Neutrino.hpp"
#include <X11/keysym.h>
#include <algorithm>
#include <vector>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <map>
#include <tuple>

#ifdef NEUTRINO_USE_XFT
#include <X11/Xft/Xft.h>
#endif

namespace neutrino {

namespace {

static Neu_Color lighten(const Neu_Color& color, int amount)
{
    auto add = [amount](uint8_t value) -> uint8_t {
        return static_cast<uint8_t>(std::min(255, static_cast<int>(value) + amount));
    };
    return {add(color.r), add(color.g), add(color.b), color.a};
}

static int fontPixelHeight(int headingLevel)
{
    if (headingLevel <= 0) {
        return 16;
    }
    return std::max(18, 34 - headingLevel * 2);
}

static std::string xftFontName(const Neu_Theme& theme,
                               bool bold,
                               bool italic,
                               bool monospace,
                               int headingLevel)
{
    std::string font = monospace
                       ? "DejaVu Sans Mono:size=10:antialias=true:hinting=true:hintstyle=hintfull:rgba=rgb:lcdfilter=lcddefault"
                       : theme.fontName;
    if (headingLevel > 0) {
        font = (monospace ? "DejaVu Sans Mono:size=" : "DejaVu Sans:size=")
               + std::to_string(std::max(11, 22 - headingLevel))
               + ":antialias=true:hinting=true:hintstyle=hintfull:rgba=rgb:lcdfilter=lcddefault";
    }
    if (bold && font.find("style=") == std::string::npos) {
        font += ":style=Bold";
    }
    if (italic && font.find("style=") == std::string::npos) {
        font += ":style=Italic";
    }
    return font;
}

static bool envFlag(const char* name, bool defaultValue)
{
    const char* value = std::getenv(name);
    if (!value || !*value) {
        return defaultValue;
    }
    if (std::strcmp(value, "0") == 0 || std::strcmp(value, "false") == 0 || std::strcmp(value, "FALSE") == 0 || std::strcmp(value, "no") == 0 || std::strcmp(value, "NO") == 0) {
        return false;
    }
    return true;
}

static bool envEquals(const char* name, const char* expected)
{
    const char* value = std::getenv(name);
    return value && std::strcmp(value, expected) == 0;
}

enum class X11TextBackend { Core, Xft, Auto, DualFallback };

static X11TextBackend x11TextBackend()
{
    // Use exactly one text backend by default.  The previous Linux text fix kept
    // a core-font safety pass enabled after successful Xft drawing, which made
    // every label appear twice with a small offset and doubled text-rendering
    // work.  Core/Xutf8 is the safest default for VM, Xvfb and older X11
    // servers.  Developers who want Xft anti-aliased text can explicitly set:
    //   NEUTRINO_X11_TEXT_BACKEND=xft
    // or use auto to try Xft and fall back only when Xft cannot draw.
    const char* value = std::getenv("NEUTRINO_X11_TEXT_BACKEND");
    if (value && *value) {
        if (std::strcmp(value, "xft") == 0 || std::strcmp(value, "Xft") == 0) {
            return X11TextBackend::Xft;
        }
        if (std::strcmp(value, "auto") == 0 || std::strcmp(value, "AUTO") == 0) {
            return X11TextBackend::Auto;
        }
        if (std::strcmp(value, "dual") == 0 || std::strcmp(value, "fallback") == 0) {
            return X11TextBackend::DualFallback;
        }
    }
    if (envFlag("NEUTRINO_DISABLE_XFT", false)
        || envEquals("NEUTRINO_X11_TEXT_BACKEND", "core")
        || envEquals("NEUTRINO_X11_TEXT_BACKEND", "xlib")) {
        return X11TextBackend::Core;
    }
    return X11TextBackend::Core;
}

static bool useXftText()
{
    const X11TextBackend backend = x11TextBackend();
    return backend == X11TextBackend::Xft
           || backend == X11TextBackend::Auto
           || backend == X11TextBackend::DualFallback;
}

static bool allowCoreTextFallback()
{
    // Off by default to prevent duplicate/overlapped text.  Only the explicit
    // dual/fallback backend draws a second core-font pass after successful Xft.
    return x11TextBackend() == X11TextBackend::DualFallback
           || envFlag("NEUTRINO_X11_TEXT_CORE_FALLBACK", false);
}

static bool xrenderExtensionAvailable(Display* display)
{
    if (!display) {
        return false;
    }
    int opcode = 0;
    int event = 0;
    int error = 0;
    return XQueryExtension(display, "RENDER", &opcode, &event, &error) != 0;
}

static XFontStruct* coreFont(Display* display, bool monospace, int headingLevel)
{
    if (!display) {
        return nullptr;
    }
    const int sizeKey = headingLevel > 0 ? 1 : 0;
    static std::map<std::tuple<Display*, bool, int>, XFontStruct*> cache;
    const auto key = std::make_tuple(display, monospace, sizeKey);
    auto found = cache.find(key);
    if (found != cache.end()) {
        return found->second;
    }

    const char* largeCandidates[] = {
        "10x20",
        "9x15",
        "fixed",
        "*-fixed-*-*-*-*-20-*-*-*-*-*-iso10646-1",
        "*-fixed-*-*-*-*-20-*-*-*-*-*-iso8859-1",
        nullptr
    };
    const char* normalCandidates[] = {
        "9x15",
        "8x13",
        "fixed",
        "*-fixed-*-*-*-*-13-*-*-*-*-*-iso10646-1",
        "*-fixed-*-*-*-*-13-*-*-*-*-*-iso8859-1",
        nullptr
    };
    const char** candidates = sizeKey ? largeCandidates : normalCandidates;
    XFontStruct* font = nullptr;
    for (int i = 0; candidates[i] && !font; ++i) {
        font = XLoadQueryFont(display, candidates[i]);
    }
    cache[key] = font;
    return font;
}

static XFontSet coreFontSet(Display* display)
{
    if (!display || envFlag("NEUTRINO_X11_TEXT_NO_FONTSET", false)) {
        return nullptr;
    }
    static std::map<Display*, XFontSet> cache;
    auto found = cache.find(display);
    if (found != cache.end()) {
        return found->second;
    }

    const char* candidates[] = {
        "-*-fixed-*-*-*-*-*-*-*-*-*-*-iso10646-1",
        "-*-fixed-*-*-*-*-*-*-*-*-*-*-*",
        "fixed",
        nullptr
    };

    XFontSet set = nullptr;
    for (int i = 0; candidates[i] && !set; ++i) {
        char** missing = nullptr;
        int missingCount = 0;
        char* defaultString = nullptr;
        set = XCreateFontSet(display, candidates[i], &missing, &missingCount, &defaultString);
        if (missing) {
            XFreeStringList(missing);
        }
    }
    cache[display] = set;
    return set;
}

static int measureCoreTextWidth(Display* display,
                                GC gc,
                                const std::string& text,
                                bool bold,
                                bool monospace,
                                int headingLevel)
{
    if (display) {
        if (XFontSet set = coreFontSet(display)) {
            XRectangle ink{};
            XRectangle logical{};
            Xutf8TextExtents(set, text.c_str(), static_cast<int>(text.size()), &ink, &logical);
            return std::max(0, static_cast<int>(logical.width)) + (bold ? 1 : 0);
        }
        if (XFontStruct* font = coreFont(display, monospace, headingLevel)) {
            return std::max(0, XTextWidth(font, text.c_str(), static_cast<int>(text.size()))) + (bold ? 1 : 0);
        }
        if (gc) {
            XFontStruct* font = XQueryFont(display, XGContextFromGC(gc));
            if (font) {
                const int width = XTextWidth(font, text.c_str(), static_cast<int>(text.size())) + (bold ? 1 : 0);
                XFreeFontInfo(nullptr, font, 1);
                return std::max(0, width);
            }
        }
    }
    int charWidth = monospace ? 8 : 7;
    if (bold) {
        ++charWidth;
    }
    if (headingLevel > 0) {
        charWidth += std::max(1, 8 - headingLevel);
    }
    return static_cast<int>(text.size()) * charWidth;
}

static void drawCoreText(Display* display,
                         Drawable drawable,
                         GC gc,
                         const std::string& text,
                         int x,
                         int y,
                         const Neu_Color& color,
                         bool bold,
                         bool underline,
                         bool strikethrough,
                         bool doubleStrikethrough,
                         bool monospace,
                         int headingLevel)
{
    if (!display || !gc || text.empty()) {
        return;
    }
    XSetForeground(display, gc, Neu_Pixel(display, color));

    if (XFontSet set = coreFontSet(display)) {
        Xutf8DrawString(display, drawable, set, gc, x, y, text.c_str(), static_cast<int>(text.size()));
        if (bold) {
            Xutf8DrawString(display, drawable, set, gc, x + 1, y, text.c_str(), static_cast<int>(text.size()));
        }
    } else {
        if (XFontStruct* font = coreFont(display, monospace, headingLevel)) {
            XSetFont(display, gc, font->fid);
        }
        XDrawString(display, drawable, gc, x, y, text.c_str(), static_cast<int>(text.size()));
        if (bold) {
            XDrawString(display, drawable, gc, x + 1, y, text.c_str(), static_cast<int>(text.size()));
        }
    }

    const int width = measureCoreTextWidth(display, gc, text, bold, monospace, headingLevel);
    if (underline) {
        XDrawLine(display, drawable, gc, x, y + 2, x + width, y + 2);
    }
    if (strikethrough || doubleStrikethrough) {
        XDrawLine(display, drawable, gc, x, y - 6, x + width, y - 6);
        if (doubleStrikethrough) {
            XDrawLine(display, drawable, gc, x, y - 3, x + width, y - 3);
        }
    }
}

static std::vector<std::string> logicalLines(const std::string& text)
{
    std::vector<std::string> out;
    std::stringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        out.push_back(line);
    }
    if (out.empty()) {
        out.push_back({});
    }
    return out;
}

} // namespace

Neu_Control::Neu_Control(Neu_Layout layout)
    : layout_(layout)
{
}

bool Neu_Control::contains(int x, int y) const
{
    const auto rect = bounds();
    return x >= rect.x && y >= rect.y && x < rect.x + rect.width && y < rect.y + rect.height;
}

void Neu_Control::setText(const std::string& text)
{
    text_ = text;
    invokeTextChanged();
    requestRedraw();
}

void Neu_Control::requestRedraw()
{
    if (parent_) {
        parent_->requestRedraw();
    }
}

int Neu_Control::measureTextWidth(Display* display,
                                  Drawable drawable,
                                  GC gc,
                                  const Neu_Theme& theme,
                                  const std::string& text,
                                  bool bold,
                                  bool italic,
                                  bool monospace,
                                  int headingLevel) const
{
(void)drawable;
#ifdef NEUTRINO_USE_XFT
    if (display && useXftText() && xrenderExtensionAvailable(display)) {
        XftFont* font = XftFontOpenName(display,
                                        DefaultScreen(display),
                                        xftFontName(theme, bold, italic, monospace, headingLevel).c_str());
        if (font) {
            XGlyphInfo extents{};
            XftTextExtentsUtf8(display,
                               font,
                               reinterpret_cast<const FcChar8*>(text.c_str()),
                               static_cast<int>(text.size()),
                               &extents);
            XftFontClose(display, font);
            return std::max(0, static_cast<int>(extents.xOff));
        }
    }
#else
    (void)theme;
    (void)italic;
#endif
    return measureCoreTextWidth(display, gc, text, bold, monospace, headingLevel);
}

std::vector<std::string> Neu_Control::wrapTextToWidth(Display* display,
                                                       Drawable drawable,
                                                       GC gc,
                                                       const Neu_Theme& theme,
                                                       const std::string& text,
                                                       int maxWidth) const
{
    std::vector<std::string> wrapped;
    if (maxWidth <= 0) {
        wrapped.push_back({});
        return wrapped;
    }

    for (const auto& logical : logicalLines(text)) {
        if (logical.empty()) {
            wrapped.push_back({});
            continue;
        }

        std::string current;
        size_t lastBreak = std::string::npos;
        for (size_t i = 0; i < logical.size(); ++i) {
            const char ch = logical[i];
            const std::string candidate = current + ch;
            if (!current.empty() && measureTextWidth(display, drawable, gc, theme, candidate) > maxWidth) {
                if (lastBreak != std::string::npos && lastBreak + 1 < current.size()) {
                    wrapped.push_back(current.substr(0, lastBreak + 1));
                    current = current.substr(lastBreak + 1);
                    lastBreak = std::string::npos;
                    for (size_t j = 0; j < current.size(); ++j) {
                        if (current[j] == ' ' || current[j] == '\t') {
                            lastBreak = j;
                        }
                    }
                } else {
                    wrapped.push_back(current);
                    current.clear();
                    lastBreak = std::string::npos;
                }
            }
            current.push_back(ch);
            if (ch == ' ' || ch == '\t') {
                lastBreak = current.size() - 1;
            }
        }
        wrapped.push_back(current);
    }

    if (wrapped.empty()) {
        wrapped.push_back({});
    }
    return wrapped;
}

std::string Neu_Control::truncateTextToWidth(Display* display,
                                             Drawable drawable,
                                             GC gc,
                                             const Neu_Theme& theme,
                                             const std::string& text,
                                             int maxWidth) const
{
    if (maxWidth <= 0 || text.empty()) {
        return {};
    }
    if (measureTextWidth(display, drawable, gc, theme, text) <= maxWidth) {
        return text;
    }
    const std::string ellipsis = "...";
    std::string out;
    for (char ch : text) {
        std::string candidate = out + ch + ellipsis;
        if (measureTextWidth(display, drawable, gc, theme, candidate) > maxWidth) {
            break;
        }
        out.push_back(ch);
    }
    return out + ellipsis;
}

int Neu_Control::alignedTextX(Display* display,
                              Drawable drawable,
                              GC gc,
                              const Neu_Theme& theme,
                              const std::string& text,
                              int left,
                              int width) const
{
    const int textWidth = measureTextWidth(display, drawable, gc, theme, text);
    if (textAlignment_ == Neu_TextAlignment::Center) {
        return left + std::max(0, (width - textWidth) / 2);
    }
    if (textAlignment_ == Neu_TextAlignment::Right) {
        return left + std::max(0, width - textWidth);
    }
    return left;
}

void Neu_Control::drawText(Display* display,
                           Drawable drawable,
                           GC gc,
                           const Neu_Theme& theme,
                           const std::string& text,
                           int x,
                           int y)
{
    drawTextColored(display, drawable, gc, theme, text, x, y, theme.text);
}

void Neu_Control::drawTextColored(Display* display,
                                  Drawable drawable,
                                  GC gc,
                                  const Neu_Theme& theme,
                                  const std::string& text,
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
bool xftDrew = false;
#ifdef NEUTRINO_USE_XFT
    if (display && useXftText() && xrenderExtensionAvailable(display)) {
        XftDraw* xftDraw = XftDrawCreate(display,
                                         drawable,
                                         DefaultVisual(display, DefaultScreen(display)),
                                         DefaultColormap(display, DefaultScreen(display)));
        XftFont* font = XftFontOpenName(display,
                                        DefaultScreen(display),
                                        xftFontName(theme, bold, italic, monospace, headingLevel).c_str());
        if (xftDraw && font) {
            XRenderColor renderColor{};
            renderColor.red = static_cast<unsigned short>(color.r * 257);
            renderColor.green = static_cast<unsigned short>(color.g * 257);
            renderColor.blue = static_cast<unsigned short>(color.b * 257);
            renderColor.alpha = 0xffff;
            XftColor xftColor{};
            if (XftColorAllocValue(display,
                                   DefaultVisual(display, DefaultScreen(display)),
                                   DefaultColormap(display, DefaultScreen(display)),
                                   &renderColor,
                                   &xftColor)) {
                XftDrawStringUtf8(xftDraw,
                                  &xftColor,
                                  font,
                                  x,
                                  y,
                                  reinterpret_cast<const FcChar8*>(text.c_str()),
                                  static_cast<int>(text.size()));
                XftColorFree(display,
                             DefaultVisual(display, DefaultScreen(display)),
                             DefaultColormap(display, DefaultScreen(display)),
                             &xftColor);
                xftDrew = true;
            }
        }
        if (font) {
            XftFontClose(display, font);
        }
        if (xftDraw) {
            XftDrawDestroy(xftDraw);
        }
        if (xftDrew) {
            const int width = measureTextWidth(display, drawable, gc, theme, text, bold, italic, monospace, headingLevel);
            XSetForeground(display, gc, Neu_Pixel(display, color));
            if (underline) {
                XDrawLine(display, drawable, gc, x, y + 2, x + width, y + 2);
            }
            if (strikethrough || doubleStrikethrough) {
                XDrawLine(display, drawable, gc, x, y - 6, x + width, y - 6);
                if (doubleStrikethrough) {
                    XDrawLine(display, drawable, gc, x, y - 3, x + width, y - 3);
                }
            }
            if (!allowCoreTextFallback()) {
                return;
            }
        }
    }
#else
    (void)theme;
    (void)italic;
#endif

    if (!xftDrew || allowCoreTextFallback()) {
        drawCoreText(display,
                     drawable,
                     gc,
                     text,
                     x,
                     y,
                     color,
                     bold,
                     underline && !xftDrew,
                     strikethrough && !xftDrew,
                     doubleStrikethrough && !xftDrew,
                     monospace,
                     headingLevel);
    }
}

void Neu_Control::drawIconBmp(Display* display, Drawable drawable, GC gc, int x, int y, int maxSize)
{
    if (icon_.pixels().empty() || icon_.width() <= 0 || icon_.height() <= 0 || maxSize <= 0) {
        return;
    }

    const int drawWidth = std::min(icon_.width(), maxSize);
    const int drawHeight = std::min(icon_.height(), maxSize);
    for (int iy = 0; iy < drawHeight; ++iy) {
        for (int ix = 0; ix < drawWidth; ++ix) {
            const uint32_t pixel = icon_.pixels()[static_cast<size_t>(iy * icon_.width() + ix)];
            const unsigned char alpha = static_cast<unsigned char>((pixel >> 24) & 0xffU);
            if (alpha < 8) {
                continue;
            }
            const Neu_Color color{static_cast<uint8_t>((pixel >> 16) & 0xffU),
                                  static_cast<uint8_t>((pixel >> 8) & 0xffU),
                                  static_cast<uint8_t>(pixel & 0xffU),
                                  alpha};
            XSetForeground(display, gc, Neu_Pixel(display, color));
            XDrawPoint(display, drawable, gc, x + ix, y + iy);
        }
    }
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

void Neu_Control::drawShadow(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    if (!Neu_GetSmoothGraphicsOptions().drawShadows) {
        return;
    }

    const auto rect = bounds();
    Neu_DrawSmoothDropShadow(display,
                             drawable,
                             gc,
                             theme.shadow,
                             theme.background,
                             rect.x,
                             rect.y,
                             rect.width,
                             rect.height,
                             theme.radius,
                             theme.shadowSize,
                             theme.shadowOffsetX,
                             theme.shadowOffsetY);
}

void Neu_Control::drawHintPopup(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    if (!Neu_GetSmoothGraphicsOptions().drawHints || !hover_ || !hoverHintArmed_ || hintText_.empty()) {
        return;
    }

    const auto elapsed = std::chrono::steady_clock::now() - hoverStartTime_;
    if (elapsed < std::chrono::seconds(5)) {
        return;
    }

    const auto rect = bounds();
    constexpr int maxHeight = 500;
    constexpr int lineHeight = 18;
    constexpr int padding = 10;
    int boxWidth = 400;
    if (parent_) {
        boxWidth = std::min(boxWidth, std::max(140, parent_->width() - 12));
    }
    const int contentWidth = std::max(40, boxWidth - 2 * padding - 16);
    const auto lines = wrapTextToWidth(display, drawable, gc, theme, hintText_, contentWidth);
    const int naturalHeight = static_cast<int>(lines.size()) * lineHeight + 2 * padding;
    const bool needsScrollbar = naturalHeight > maxHeight;
    const bool needsDropDown = lines.size() > 3U;
    const int visibleHeight = std::min(naturalHeight, maxHeight);
    const int visibleLines = std::max(1, (visibleHeight - 2 * padding) / lineHeight);
    int boxX = rect.x + rect.width + 12;
    int boxY = rect.y;

    if (parent_) {
        if (boxX + boxWidth > parent_->width()) {
            boxX = std::max(6, rect.x - boxWidth - 12);
        }
        if (boxY + visibleHeight > parent_->height()) {
            boxY = std::max(6, parent_->height() - visibleHeight - 6);
        }
    }

    Neu_DrawSmoothDropShadow(display,
                             drawable,
                             gc,
                             theme.shadow,
                             theme.background,
                             boxX,
                             boxY,
                             boxWidth,
                             visibleHeight,
                             10,
                             6,
                             4,
                             5);
    XSetForeground(display, gc, Neu_Pixel(display, theme.hintBackground));
    Neu_DrawRoundedRect(display, drawable, gc, boxX, boxY, boxWidth, visibleHeight, 10, true);
    XSetForeground(display, gc, Neu_Pixel(display, theme.hintBorder));
    Neu_DrawRoundedRect(display, drawable, gc, boxX, boxY, boxWidth, visibleHeight, 10, false);

    XRectangle clip{static_cast<short>(boxX + padding),
                    static_cast<short>(boxY + padding),
                    static_cast<unsigned short>(std::max(1, contentWidth)),
                    static_cast<unsigned short>(std::max(1, visibleHeight - 2 * padding))};
    XSetClipRectangles(display, gc, 0, 0, &clip, 1, Unsorted);

    const int count = hintExpanded_ ? std::min(static_cast<int>(lines.size()), visibleLines) : std::min(3, visibleLines);
    for (int index = 0; index < count; ++index) {
        drawText(display,
                 drawable,
                 gc,
                 theme,
                 truncateTextToWidth(display, drawable, gc, theme, lines[static_cast<size_t>(index)], contentWidth),
                 boxX + padding,
                 boxY + padding + 14 + index * lineHeight);
    }
    XSetClipMask(display, gc, None);

    if (needsDropDown) {
        drawText(display,
                 drawable,
                 gc,
                 theme,
                 hintExpanded_ ? "^" : "v",
                 boxX + boxWidth - 24,
                 boxY + visibleHeight - 10);
    }

    if (needsScrollbar) {
        XSetForeground(display, gc, Neu_Pixel(display, lighten(theme.hintBorder, 70)));
        XFillRectangle(display, drawable, gc, boxX + boxWidth - 10, boxY + 12, 4, visibleHeight - 24);
        XSetForeground(display, gc, Neu_Pixel(display, theme.hintBorder));
        XFillRectangle(display, drawable, gc, boxX + boxWidth - 11, boxY + 12, 6, std::max(20, visibleHeight / 4));
    }
}

void Neu_Control::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    const auto rect = bounds();
    const std::string cls = className();
    const bool suppressHoverFill = cls == "Neu_RichTextCode"
                                   || cls == "Neu_ReadOnlyRichText"
                                   || cls == "Neu_Placement"
                                   || cls == "Neu_ScrollWindow"
                                   || cls == "Neu_ListView"
                                   || cls == "Neu_TreeView"
                                   || cls == "Neu_Multilinetextbox";
    const Neu_Color fill = (hover_ && !suppressHoverFill) ? theme.highlight : theme.glass;
    XSetForeground(display, gc, Neu_Pixel(display, fill));
    Neu_DrawRoundedRect(display, drawable, gc, rect.x, rect.y, rect.width, rect.height, theme.radius, true);
    XSetForeground(display, gc, Neu_Pixel(display, focused_ ? theme.focus : theme.border));
    Neu_DrawRoundedRect(display, drawable, gc, rect.x, rect.y, rect.width, rect.height, theme.radius, false);
    if (focused_ && !suppressHoverFill) {
        XSetForeground(display, gc, Neu_Pixel(display, theme.focus));
        Neu_DrawRoundedRect(display, drawable, gc, rect.x + 2, rect.y + 2, rect.width - 4, rect.height - 4, std::max(2, theme.radius - 2), false);
    }
}

void Neu_Control::handleXEvent(XEvent& event)
{
    const auto resetHintAnchor = [&](int x, int y) {
        hoverAnchorX_ = x;
        hoverAnchorY_ = y;
        hoverStartTime_ = std::chrono::steady_clock::now();
        hoverHintArmed_ = true;
        hintExpanded_ = false;
    };

    if (event.type == LeaveNotify) {
        if (hover_) {
            hover_ = false;
            hoverHintArmed_ = false;
            activeScrollDrag_ = 0;
            if (callbacks_.onBlur) {
                callbacks_.onBlur(this, callbacks_.userData);
            }
            requestRedraw();
        }
    }

    if (event.type == MotionNotify) {
        const int mx = event.xmotion.x;
        const int my = event.xmotion.y;
        const bool newHover = contains(mx, my);
        if (newHover != hover_) {
            hover_ = newHover;
            if (hover_) {
                resetHintAnchor(mx, my);
                if (callbacks_.onFocus) {
                    callbacks_.onFocus(this, callbacks_.userData);
                }
            } else {
                hoverHintArmed_ = false;
                if (callbacks_.onBlur) {
                    callbacks_.onBlur(this, callbacks_.userData);
                }
            }
            requestRedraw();
        } else if (hover_) {
            if (std::abs(mx - hoverAnchorX_) > 4 || std::abs(my - hoverAnchorY_) > 4) {
                resetHintAnchor(mx, my);
                requestRedraw();
            } else if (std::chrono::steady_clock::now() - hoverStartTime_ >= std::chrono::seconds(5)) {
                requestRedraw();
            }
        }
    }

    const auto updateScrollFromPointer = [&](int px, int py) {
        const auto rect = bounds();
        if (activeScrollDrag_ == 1) {
            const int trackY = rect.y + 8;
            const int trackH = std::max(1, rect.height - 16);
            const int maxY = std::max(1, virtualSize_.height - rect.height);
            setScrollOffset(scrollX_, (py - trackY) * maxY / trackH);
            return true;
        }
        if (activeScrollDrag_ == 2) {
            const int trackX = rect.x + 8;
            const int trackW = std::max(1, rect.width - 16);
            const int maxX = std::max(1, virtualSize_.width - rect.width);
            setScrollOffset((px - trackX) * maxX / trackW, scrollY_);
            return true;
        }
        return false;
    };

    if (event.type == MotionNotify && activeScrollDrag_ != 0) {
        updateScrollFromPointer(event.xmotion.x, event.xmotion.y);
        return;
    }

    if (event.type == ButtonRelease && activeScrollDrag_ != 0) {
        activeScrollDrag_ = 0;
        requestRedraw();
        return;
    }

    if (event.type == ButtonRelease && hover_ && !hintText_.empty()) {
        const auto rect = bounds();
        if (event.xbutton.x > rect.x + rect.width && event.xbutton.x < rect.x + rect.width + 420) {
            hintExpanded_ = !hintExpanded_;
            requestRedraw();
        }
    }

    if (event.type == ButtonPress && autoScroll_ && contains(event.xbutton.x, event.xbutton.y)) {
        const auto rect = bounds();
        const bool needVertical = virtualSize_.height > rect.height;
        const bool needHorizontal = virtualSize_.width > rect.width;
        if (event.xbutton.button == Button1 && needVertical && event.xbutton.x >= rect.x + rect.width - 14) {
            activeScrollDrag_ = 1;
            updateScrollFromPointer(event.xbutton.x, event.xbutton.y);
            return;
        }
        if (event.xbutton.button == Button1 && needHorizontal && event.xbutton.y >= rect.y + rect.height - 14) {
            activeScrollDrag_ = 2;
            updateScrollFromPointer(event.xbutton.x, event.xbutton.y);
            return;
        }
        if (event.xbutton.button == Button4) {
            setScrollOffset(scrollX_, scrollY_ - 32);
            return;
        }
        if (event.xbutton.button == Button5) {
            setScrollOffset(scrollX_, scrollY_ + 32);
            return;
        }
        if (event.xbutton.button == 6) {
            setScrollOffset(scrollX_ - 32, scrollY_);
            return;
        }
        if (event.xbutton.button == 7) {
            setScrollOffset(scrollX_ + 32, scrollY_);
            return;
        }
    }

    if (event.type == KeyPress && callbacks_.onKeyDown) {
        callbacks_.onKeyDown(this, XLookupKeysym(&event.xkey, 0), event.xkey.state, callbacks_.userData);
    }
}

void Neu_Control::setScrollOffset(int x, int y)
{
    const auto rect = bounds();
    const int maxX = std::max(0, virtualSize_.width - rect.width);
    const int maxY = std::max(0, virtualSize_.height - rect.height);
    const int newScrollX = std::max(0, std::min(x, maxX));
    const int newScrollY = std::max(0, std::min(y, maxY));
    if (newScrollX == scrollX_ && newScrollY == scrollY_) {
        return;
    }
    scrollX_ = newScrollX;
    scrollY_ = newScrollY;
    if (callbacks_.onScroll) {
        callbacks_.onScroll(this, scrollX_, scrollY_, callbacks_.userData);
    }
    requestRedraw();
}

void Neu_Control::setVirtualSize(int width, int height)
{
    virtualSize_.width = std::max(0, width);
    virtualSize_.height = std::max(0, height);
    const auto rect = bounds();
    const int maxX = std::max(0, virtualSize_.width - rect.width);
    const int maxY = std::max(0, virtualSize_.height - rect.height);
    scrollX_ = std::max(0, std::min(scrollX_, maxX));
    scrollY_ = std::max(0, std::min(scrollY_, maxY));
}

void Neu_Control::drawScrollbars(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    if (!autoScroll_) {
        return;
    }

    const auto rect = bounds();
    const bool needVertical = virtualSize_.height > rect.height;
    const bool needHorizontal = virtualSize_.width > rect.width;

    if (needVertical) {
        const int trackX = rect.x + rect.width - 10;
        const int trackY = rect.y + 8;
        const int trackH = std::max(1, rect.height - 16);
        const int thumbH = std::max(20, trackH * rect.height / std::max(1, virtualSize_.height));
        const int maxY = std::max(1, virtualSize_.height - rect.height);
        const int thumbY = trackY + (trackH - thumbH) * scrollY_ / maxY;
        XSetForeground(display, gc, Neu_Pixel(display, Neu_Color{205, 215, 226, 210}));
        XFillRectangle(display, drawable, gc, trackX, trackY, 5, trackH);
        XSetForeground(display, gc, Neu_Pixel(display, theme.focus));
        XFillRectangle(display, drawable, gc, trackX - 1, thumbY, 7, thumbH);
    }

    if (needHorizontal) {
        const int trackX = rect.x + 8;
        const int trackY = rect.y + rect.height - 10;
        const int trackW = std::max(1, rect.width - 16);
        const int thumbW = std::max(20, trackW * rect.width / std::max(1, virtualSize_.width));
        const int maxX = std::max(1, virtualSize_.width - rect.width);
        const int thumbX = trackX + (trackW - thumbW) * scrollX_ / maxX;
        XSetForeground(display, gc, Neu_Pixel(display, Neu_Color{205, 215, 226, 210}));
        XFillRectangle(display, drawable, gc, trackX, trackY, trackW, 5);
        XSetForeground(display, gc, Neu_Pixel(display, theme.focus));
        XFillRectangle(display, drawable, gc, thumbX, trackY - 1, thumbW, 7);
    }
}

} // namespace neutrino
