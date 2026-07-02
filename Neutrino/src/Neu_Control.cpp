#include "Neutrino/Neutrino.hpp"
#include <X11/keysym.h>
#include <algorithm>
#include <cctype>
#include <vector>

#ifdef NEUTRINO_USE_XFT
#include <X11/Xft/Xft.h>
#endif

namespace neutrino {

namespace {

thread_local bool g_hasActiveClip = false;
thread_local XRectangle g_activeClipRect{};

static Neu_Rect intersectRects(const Neu_Rect& a, const Neu_Rect& b)
{
    const int left = std::max(a.x, b.x);
    const int top = std::max(a.y, b.y);
    const int right = std::min(a.x + a.width, b.x + b.width);
    const int bottom = std::min(a.y + a.height, b.y + b.height);
    return {left, top, std::max(0, right - left), std::max(0, bottom - top)};
}

static Neu_Color lighten(const Neu_Color& color, int amount)
{
    auto add = [amount](uint8_t value) -> uint8_t {
        return static_cast<uint8_t>(std::min(255, static_cast<int>(value) + amount));
    };
    return {add(color.r), add(color.g), add(color.b), color.a};
}

static std::string trimRight(const std::string& value)
{
    std::string out = value;
    while (!out.empty() && std::isspace(static_cast<unsigned char>(out.back())) != 0) {
        out.pop_back();
    }
    return out;
}

static std::string fragmentFontName(const Neu_Theme& theme, const Neu_RichTextFragment& fragment)
{
    if (!fragment.fontName.empty()) {
        return fragment.fontName;
    }

    const int heading = std::max(0, std::min(7, fragment.headingLevel));
    const int size = heading > 0 ? std::max(10, 24 - heading * 2) : 10;
    const bool bold = heading > 0 || (fragment.style & Neu_TextStyle_Bold) != 0U;
    const bool italic = (fragment.style & Neu_TextStyle_Italic) != 0U;
    const bool mono = (fragment.style & Neu_TextStyle_Monospaced) != 0U;

    std::ostringstream font;
    font << (mono ? "DejaVu Sans Mono" : "DejaVu Sans") << ":size=" << size
         << ":antialias=true:hinting=true:hintstyle=hintfull:rgba=rgb:lcdfilter=lcddefault";
    if (bold && italic) {
        font << ":style=Bold Italic";
    } else if (bold) {
        font << ":style=Bold";
    } else if (italic) {
        font << ":style=Italic";
    } else if (!theme.fontName.empty()) {
        return theme.fontName;
    }
    return font.str();
}

static std::string combinedText(const std::vector<Neu_RichTextFragment>& fragments)
{
    std::string output;
    for (const auto& fragment : fragments) {
        output += fragment.text;
    }
    return output;
}

static void setClip(Display* display, GC gc, const Neu_Rect& rect)
{
    XRectangle clip{};
    clip.x = static_cast<short>(rect.x);
    clip.y = static_cast<short>(rect.y);
    clip.width = static_cast<unsigned short>(std::max(0, rect.width));
    clip.height = static_cast<unsigned short>(std::max(0, rect.height));
    g_activeClipRect = clip;
    g_hasActiveClip = clip.width > 0 && clip.height > 0;
    XSetClipRectangles(display, gc, 0, 0, &clip, 1, Unsorted);
}

static void clearClip(Display* display, GC gc)
{
    g_hasActiveClip = false;
    XSetClipMask(display, gc, None);
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
        parent_->redraw();
    }
}

void Neu_Control::drawText(Display* display,
                           Drawable drawable,
                           GC gc,
                           const Neu_Theme& theme,
                           const std::string& text,
                           int x,
                           int y)
{
#ifdef NEUTRINO_USE_XFT
    if (display) {
        XftDraw* xftDraw = XftDrawCreate(display,
                                         drawable,
                                         DefaultVisual(display, DefaultScreen(display)),
                                         DefaultColormap(display, DefaultScreen(display)));
        XftFont* font = XftFontOpenName(display, DefaultScreen(display), theme.fontName.c_str());
        if (xftDraw && font) {
            if (g_hasActiveClip) {
                XRectangle xftClip = g_activeClipRect;
                XftDrawSetClipRectangles(xftDraw, 0, 0, &xftClip, 1);
            }
            XRenderColor renderColor{};
            renderColor.red = static_cast<unsigned short>(theme.text.r * 257);
            renderColor.green = static_cast<unsigned short>(theme.text.g * 257);
            renderColor.blue = static_cast<unsigned short>(theme.text.b * 257);
            renderColor.alpha = 0xffff;
            XftColor xftColor{};
            XftColorAllocValue(display,
                               DefaultVisual(display, DefaultScreen(display)),
                               DefaultColormap(display, DefaultScreen(display)),
                               &renderColor,
                               &xftColor);
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
        }
        if (font) {
            XftFontClose(display, font);
        }
        if (xftDraw) {
            XftDrawDestroy(xftDraw);
        }
        if (font) {
            return;
        }
    }
#endif

    XSetForeground(display, gc, Neu_Pixel(display, theme.text));
    XDrawString(display, drawable, gc, x, y, text.c_str(), static_cast<int>(text.size()));
}

int Neu_Control::approximateTextWidth(const std::string& text, uint32_t style, int headingLevel) const
{
    int charWidth = (style & Neu_TextStyle_Monospaced) != 0U ? 8 : 7;
    if ((style & Neu_TextStyle_Bold) != 0U) {
        ++charWidth;
    }
    if (headingLevel > 0) {
        charWidth += std::max(0, 8 - headingLevel);
    }
    return static_cast<int>(text.size()) * charWidth;
}

std::vector<std::string> Neu_Control::wrapTextToWidth(const std::string& text, int maxWidth) const
{
    std::vector<std::string> lines;
    const int safeWidth = std::max(8, maxWidth);
    const int charWidth = 7;
    const size_t maxChars = static_cast<size_t>(std::max(1, safeWidth / charWidth));
    std::stringstream paragraphs(text);
    std::string paragraph;

    while (std::getline(paragraphs, paragraph)) {
        std::istringstream words(paragraph);
        std::string word;
        std::string line;
        while (words >> word) {
            while (word.size() > maxChars) {
                if (!line.empty()) {
                    lines.push_back(trimRight(line));
                    line.clear();
                }
                lines.push_back(word.substr(0, maxChars));
                word.erase(0, maxChars);
            }
            if (!line.empty() && line.size() + 1U + word.size() > maxChars) {
                lines.push_back(trimRight(line));
                line.clear();
            }
            if (!line.empty()) {
                line += ' ';
            }
            line += word;
        }
        if (!line.empty()) {
            lines.push_back(trimRight(line));
        }
        if (paragraph.empty()) {
            lines.emplace_back();
        }
    }

    if (lines.empty()) {
        lines.emplace_back();
    }
    return lines;
}

std::string Neu_Control::truncateTextToWidth(const std::string& text, int maxWidth) const
{
    if (approximateTextWidth(text) <= maxWidth) {
        return text;
    }
    const std::string ellipsis = "...";
    const int available = std::max(0, maxWidth - approximateTextWidth(ellipsis));
    const size_t keep = static_cast<size_t>(std::max(0, available / 7));
    if (keep == 0U) {
        return ellipsis;
    }
    if (keep >= text.size()) {
        return text;
    }
    return text.substr(0, keep) + ellipsis;
}

void Neu_Control::drawTextInRect(Display* display,
                                 Drawable drawable,
                                 GC gc,
                                 const Neu_Theme& theme,
                                 const std::string& text,
                                 const Neu_Rect& rect,
                                 const Neu_TextLayoutOptions& options)
{
    if (rect.width <= 0 || rect.height <= 0) {
        return;
    }

    const Neu_Rect clipRect = intersectRects(rect, bounds());
    if (clipRect.width <= 0 || clipRect.height <= 0) {
        return;
    }

    Neu_Theme localTheme = theme;
    if (hasFontColor_) {
        localTheme.text = fontColor_;
    }
    if (!fontName_.empty()) {
        localTheme.fontName = fontName_;
    }

    if (hasBackgroundColor_) {
        XSetForeground(display, gc, Neu_Pixel(display, backgroundColor_));
        XFillRectangle(display, drawable, gc, clipRect.x, clipRect.y, clipRect.width, clipRect.height);
    }
    if (hasHighlight_) {
        XSetForeground(display, gc, Neu_Pixel(display, highlightColor_));
        XFillRectangle(display, drawable, gc, clipRect.x, clipRect.y, clipRect.width, clipRect.height);
    }

    setClip(display, gc, clipRect);
    const int usableWidth = std::max(1, rect.width - options.padding * 2);
    std::vector<std::string> lines;
    if (options.wordWrap) {
        lines = wrapTextToWidth(text, usableWidth);
    } else {
        lines.push_back(options.truncate ? truncateTextToWidth(text, usableWidth) : text);
    }

    int y = rect.y + options.padding + 14;
    for (const auto& line : lines) {
        if (y > rect.y + rect.height - 2) {
            break;
        }
        int x = rect.x + options.padding;
        const int width = approximateTextWidth(line);
        if (options.align == Neu_TextAlign::Center) {
            x = rect.x + std::max(0, (rect.width - width) / 2);
        } else if (options.align == Neu_TextAlign::Right) {
            x = rect.x + rect.width - options.padding - width;
        }
        drawText(display, drawable, gc, localTheme, line, x, y);
        y += options.lineHeight;
    }
    clearClip(display, gc);
}

void Neu_Control::drawRichTextFragments(Display* display,
                                        Drawable drawable,
                                        GC gc,
                                        const Neu_Theme& theme,
                                        const std::vector<Neu_RichTextFragment>& fragments,
                                        const Neu_Rect& rect,
                                        const Neu_TextLayoutOptions& options)
{
    if (fragments.empty()) {
        drawTextInRect(display, drawable, gc, theme, text_, rect, options);
        return;
    }

    if (options.wordWrap) {
        // Word wrapping preserves boundaries safely by drawing a wrapped combined view first;
        // individual fragment styling remains available for non-wrapped toolbar/read-only snippets.
        drawTextInRect(display, drawable, gc, theme, combinedText(fragments), rect, options);
        return;
    }

    const Neu_Rect clipRect = intersectRects(rect, bounds());
    if (clipRect.width <= 0 || clipRect.height <= 0) {
        return;
    }
    setClip(display, gc, clipRect);
    int totalWidth = 0;
    for (const auto& fragment : fragments) {
        totalWidth += approximateTextWidth(fragment.text, fragment.style, fragment.headingLevel);
    }

    int x = rect.x + options.padding;
    if (options.align == Neu_TextAlign::Center) {
        x = rect.x + std::max(0, (rect.width - totalWidth) / 2);
    } else if (options.align == Neu_TextAlign::Right) {
        x = rect.x + rect.width - options.padding - totalWidth;
    }
    const int baseline = rect.y + options.padding + 14;

    for (const auto& fragment : fragments) {
        if (x >= rect.x + rect.width - options.padding) {
            break;
        }
        Neu_Theme fragmentTheme = theme;
        if (fragment.hasFontColor) {
            fragmentTheme.text = fragment.fontColor;
        }
        fragmentTheme.fontName = fragmentFontName(theme, fragment);
        std::string text = fragment.text;
        if (options.truncate) {
            text = truncateTextToWidth(text, rect.x + rect.width - options.padding - x);
        }
        const int width = approximateTextWidth(text, fragment.style, fragment.headingLevel);
        if (fragment.hasBackgroundColor || fragment.hasHighlight) {
            XSetForeground(display, gc, Neu_Pixel(display, fragment.hasHighlight ? fragment.highlightColor : fragment.backgroundColor));
            XFillRectangle(display, drawable, gc, x, rect.y + 2, std::min(width, rect.x + rect.width - x), options.lineHeight);
        }
        drawText(display, drawable, gc, fragmentTheme, text, x, baseline);
        if ((fragment.style & Neu_TextStyle_Underline) != 0U) {
            XDrawLine(display, drawable, gc, x, baseline + 2, x + width, baseline + 2);
        }
        if ((fragment.style & Neu_TextStyle_Strikethrough) != 0U) {
            XDrawLine(display, drawable, gc, x, baseline - 5, x + width, baseline - 5);
        }
        if ((fragment.style & Neu_TextStyle_DoubleStrikethrough) != 0U) {
            XDrawLine(display, drawable, gc, x, baseline - 7, x + width, baseline - 7);
            XDrawLine(display, drawable, gc, x, baseline - 3, x + width, baseline - 3);
        }
        x += width;
    }
    clearClip(display, gc);
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
    if (!Neu_GetSmoothGraphicsOptions().drawHints || !hover_ || hintText_.empty()) {
        return;
    }

    const auto rect = bounds();
    constexpr int maxWidth = 400;
    constexpr int maxHeight = 500;
    constexpr int padding = 10;
    constexpr int lineHeight = 18;
    const int contentWidth = maxWidth - 2 * padding - 12;
    const auto lines = wrapTextToWidth(hintText_, contentWidth);
    const int naturalHeight = static_cast<int>(lines.size()) * lineHeight + 2 * padding;
    const bool needsScrollbar = naturalHeight > maxHeight;
    const bool needsDropDown = lines.size() > 3U;
    const int visibleHeight = std::min(naturalHeight, maxHeight);
    const int visibleLines = std::max(1, (visibleHeight - 2 * padding) / lineHeight);
    int boxX = rect.x + rect.width + 12;
    int boxY = rect.y;

    if (parent_) {
        if (boxX + maxWidth > parent_->width()) {
            boxX = std::max(6, rect.x - maxWidth - 12);
        }
        if (boxY + visibleHeight > parent_->height()) {
            boxY = std::max(6, parent_->height() - visibleHeight - 6);
        }
    }

    Neu_DrawSmoothDropShadow(display, drawable, gc, theme.shadow, theme.background, boxX, boxY, maxWidth, visibleHeight, 10, 6, 4, 5);
    XSetForeground(display, gc, Neu_Pixel(display, theme.hintBackground));
    Neu_DrawRoundedRect(display, drawable, gc, boxX, boxY, maxWidth, visibleHeight, 10, true);
    XSetForeground(display, gc, Neu_Pixel(display, theme.hintBorder));
    Neu_DrawRoundedRect(display, drawable, gc, boxX, boxY, maxWidth, visibleHeight, 10, false);

    Neu_Rect clip{boxX + padding, boxY + padding, contentWidth, visibleHeight - 2 * padding};
    setClip(display, gc, clip);
    const int count = hintExpanded_ ? std::min(static_cast<int>(lines.size()), visibleLines) : std::min(3, visibleLines);
    for (int index = 0; index < count; ++index) {
        drawText(display, drawable, gc, theme, lines[static_cast<size_t>(index)], clip.x, clip.y + 14 + index * lineHeight);
    }
    clearClip(display, gc);

    if (needsDropDown) {
        drawText(display, drawable, gc, theme, hintExpanded_ ? "^" : "v", boxX + maxWidth - 24, boxY + visibleHeight - 10);
    }

    if (needsScrollbar) {
        XSetForeground(display, gc, Neu_Pixel(display, lighten(theme.hintBorder, 70)));
        XFillRectangle(display, drawable, gc, boxX + maxWidth - 10, boxY + 12, 4, visibleHeight - 24);
        XSetForeground(display, gc, Neu_Pixel(display, theme.hintBorder));
        XFillRectangle(display, drawable, gc, boxX + maxWidth - 11, boxY + 12, 6, std::max(20, visibleHeight / 4));
    }
}

void Neu_Control::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    const auto rect = bounds();
    const Neu_Color fill = hover_ ? theme.hover : theme.glass;
    XSetForeground(display, gc, Neu_Pixel(display, fill));
    Neu_DrawRoundedRect(display, drawable, gc, rect.x, rect.y, rect.width, rect.height, theme.radius, true);
    XSetForeground(display, gc, Neu_Pixel(display, theme.border));
    Neu_DrawRoundedRect(display, drawable, gc, rect.x, rect.y, rect.width, rect.height, theme.radius, false);
}

bool Neu_Control::handleScrollMouseEvent(XEvent& event)
{
    if (!autoScroll_) {
        return false;
    }

    const auto rect = bounds();
    const bool needVertical = virtualSize_.height > rect.height;
    const bool needHorizontal = virtualSize_.width > rect.width;
    const int verticalTrackX = rect.x + rect.width - 12;
    const int verticalTrackY = rect.y + 8;
    const int verticalTrackH = std::max(1, rect.height - 16);
    const int horizontalTrackX = rect.x + 8;
    const int horizontalTrackY = rect.y + rect.height - 12;
    const int horizontalTrackW = std::max(1, rect.width - 16);

    if (event.type == ButtonPress && contains(event.xbutton.x, event.xbutton.y)) {
        if (event.xbutton.button == Button4) {
            setScrollOffset(scrollX_, scrollY_ - 32);
            return true;
        }
        if (event.xbutton.button == Button5) {
            setScrollOffset(scrollX_, scrollY_ + 32);
            return true;
        }
        if (event.xbutton.button == 6) {
            setScrollOffset(scrollX_ - 32, scrollY_);
            return true;
        }
        if (event.xbutton.button == 7) {
            setScrollOffset(scrollX_ + 32, scrollY_);
            return true;
        }
        if (needVertical && event.xbutton.x >= verticalTrackX - 4 && event.xbutton.x <= verticalTrackX + 10
            && event.xbutton.y >= verticalTrackY && event.xbutton.y <= verticalTrackY + verticalTrackH) {
            scrollDrag_ = true;
            scrollDragVertical_ = true;
            scrollDragAnchor_ = event.xbutton.y;
            scrollDragStartValue_ = scrollY_;
            const int maxY = std::max(1, virtualSize_.height - rect.height);
            setScrollOffset(scrollX_, (event.xbutton.y - verticalTrackY) * maxY / verticalTrackH);
            return true;
        }
        if (needHorizontal && event.xbutton.y >= horizontalTrackY - 4 && event.xbutton.y <= horizontalTrackY + 10
            && event.xbutton.x >= horizontalTrackX && event.xbutton.x <= horizontalTrackX + horizontalTrackW) {
            scrollDrag_ = true;
            scrollDragVertical_ = false;
            scrollDragAnchor_ = event.xbutton.x;
            scrollDragStartValue_ = scrollX_;
            const int maxX = std::max(1, virtualSize_.width - rect.width);
            setScrollOffset((event.xbutton.x - horizontalTrackX) * maxX / horizontalTrackW, scrollY_);
            return true;
        }
    }

    if (event.type == MotionNotify && scrollDrag_) {
        if (scrollDragVertical_ && needVertical) {
            const int maxY = std::max(1, virtualSize_.height - rect.height);
            const int delta = event.xmotion.y - scrollDragAnchor_;
            setScrollOffset(scrollX_, scrollDragStartValue_ + delta * maxY / verticalTrackH);
        } else if (needHorizontal) {
            const int maxX = std::max(1, virtualSize_.width - rect.width);
            const int delta = event.xmotion.x - scrollDragAnchor_;
            setScrollOffset(scrollDragStartValue_ + delta * maxX / horizontalTrackW, scrollY_);
        }
        return true;
    }

    if (event.type == ButtonRelease && scrollDrag_) {
        scrollDrag_ = false;
        return true;
    }

    return false;
}

void Neu_Control::handleXEvent(XEvent& event)
{
    if (handleScrollMouseEvent(event)) {
        return;
    }

    if (event.type == LeaveNotify && hover_) {
        hover_ = false;
        if (callbacks_.onBlur) {
            callbacks_.onBlur(this, callbacks_.userData);
        }
        requestRedraw();
    }

    if (event.type == MotionNotify) {
        const bool newHover = contains(event.xmotion.x, event.xmotion.y);
        if (newHover != hover_) {
            hover_ = newHover;
            if (hover_ && callbacks_.onFocus) {
                callbacks_.onFocus(this, callbacks_.userData);
            } else if (!hover_ && callbacks_.onBlur) {
                callbacks_.onBlur(this, callbacks_.userData);
            }
            requestRedraw();
        }
    }

    if (event.type == ButtonRelease && hover_ && !hintText_.empty()) {
        const auto rect = bounds();
        if (event.xbutton.x > rect.x + rect.width && event.xbutton.x < rect.x + rect.width + 420) {
            hintExpanded_ = !hintExpanded_;
            requestRedraw();
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
    scrollX_ = std::max(0, std::min(x, maxX));
    scrollY_ = std::max(0, std::min(y, maxY));
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
        XSetForeground(display, gc, Neu_Pixel(display, theme.accent));
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
        XSetForeground(display, gc, Neu_Pixel(display, theme.accent));
        XFillRectangle(display, drawable, gc, thumbX, trackY - 1, thumbW, 7);
    }
}

} // namespace neutrino
