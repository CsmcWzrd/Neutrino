#include "Neutrino/Neutrino.hpp"
#include <X11/keysym.h>
#include <algorithm>
#include <vector>

#ifdef NEUTRINO_USE_XFT
#include <X11/Xft/Xft.h>
#endif

namespace neutrino {

namespace {

static std::vector<std::string> wrapHintText(const std::string& text, size_t maxChars)
{
    std::vector<std::string> lines;
    std::string current;
    std::string word;
    std::istringstream input(text);

    while (input >> word) {
        if (!current.empty() && current.size() + 1U + word.size() > maxChars) {
            lines.push_back(current);
            current.clear();
        }

        if (!current.empty()) {
            current += ' ';
        }
        current += word;
    }

    if (!current.empty()) {
        lines.push_back(current);
    }

    if (lines.empty() && !text.empty()) {
        lines.push_back(text.substr(0, maxChars));
    }

    return lines;
}

static Neu_Color lighten(const Neu_Color& color, int amount)
{
    auto add = [amount](uint8_t value) -> uint8_t {
        return static_cast<uint8_t>(std::min(255, static_cast<int>(value) + amount));
    };
    return {add(color.r), add(color.g), add(color.b), color.a};
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
    constexpr int charWidth = 7;
    constexpr int lineHeight = 18;
    constexpr int padding = 10;
    const auto lines = wrapHintText(hintText_, static_cast<size_t>((maxWidth - 2 * padding) / charWidth));
    const int naturalHeight = static_cast<int>(lines.size()) * lineHeight + 2 * padding;
    const bool needsScrollbar = naturalHeight > maxHeight;
    const bool needsDropDown = lines.size() > 3U;
    const int visibleHeight = std::min(naturalHeight, maxHeight);
    const int visibleLines = std::max(1, (visibleHeight - 2 * padding) / lineHeight);
    const int boxWidth = maxWidth;
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

    const int count = hintExpanded_ ? std::min(static_cast<int>(lines.size()), visibleLines) : std::min(3, visibleLines);
    for (int index = 0; index < count; ++index) {
        drawText(display,
                 drawable,
                 gc,
                 theme,
                 lines[static_cast<size_t>(index)],
                 boxX + padding,
                 boxY + padding + 14 + index * lineHeight);
    }

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
    const Neu_Color fill = hover_ ? theme.hover : theme.glass;
    XSetForeground(display, gc, Neu_Pixel(display, fill));
    Neu_DrawRoundedRect(display, drawable, gc, rect.x, rect.y, rect.width, rect.height, theme.radius, true);
    XSetForeground(display, gc, Neu_Pixel(display, focused_ ? theme.accent : theme.border));
    Neu_DrawRoundedRect(display, drawable, gc, rect.x, rect.y, rect.width, rect.height, theme.radius, false);
    if (focused_) {
        XSetForeground(display, gc, Neu_Pixel(display, theme.accent));
        Neu_DrawRoundedRect(display, drawable, gc, rect.x + 2, rect.y + 2, rect.width - 4, rect.height - 4, std::max(2, theme.radius - 2), false);
    }
}

void Neu_Control::handleXEvent(XEvent& event)
{
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

    if (event.type == ButtonPress && autoScroll_ && contains(event.xbutton.x, event.xbutton.y)) {
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

} // namespace neutrino

// Scroll support additions
namespace neutrino {

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
