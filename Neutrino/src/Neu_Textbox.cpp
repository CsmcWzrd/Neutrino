#include "Neutrino/Neutrino.hpp"
#include <X11/keysym.h>

namespace neutrino {

void Neu_Textbox::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(display, drawable, gc, theme);
    const auto rect = bounds();
    const int pad = 8;
    Neu_Rect textRect{rect.x + pad, rect.y + 3, rect.width - 2 * pad, rect.height - 6};
    drawTextInRect(display,
                   drawable,
                   gc,
                   theme,
                   text_,
                   textRect,
                   Neu_TextLayoutOptions{false, true, Neu_TextAlign::Left, 18, 0});
    if (focused_) {
        const int caretX = std::min(textRect.x + textRect.width - 1,
                                    textRect.x + approximateTextWidth(text_.substr(0, cursor_)) - scrollX_);
        XSetForeground(display, gc, Neu_Pixel(display, theme.accent));
        XDrawLine(display, drawable, gc, caretX, rect.y + 6, caretX, rect.y + rect.height - 7);
    }
}

void Neu_Textbox::handleXEvent(XEvent& event)
{
    if (event.type == ButtonPress) {
        focused_ = contains(event.xbutton.x, event.xbutton.y);
        if (focused_) {
            const auto rect = bounds();
            const int localX = std::max(0, event.xbutton.x - rect.x - 8 + scrollX_);
            cursor_ = std::min(text_.size(), static_cast<size_t>(localX / 7));
        }
        requestRedraw();
    }

    if (event.type == KeyPress && focused_) {
        char buffer[8] = {0};
        KeySym keySymbol = 0;
        const int count = XLookupString(&event.xkey, buffer, sizeof(buffer) - 1, &keySymbol, nullptr);

        if (keySymbol == XK_BackSpace) {
            if (cursor_ > 0 && !text_.empty()) {
                text_.erase(cursor_ - 1, 1);
                --cursor_;
                invokeTextChanged();
            }
        } else if (keySymbol == XK_Delete) {
            if (cursor_ < text_.size()) {
                text_.erase(cursor_, 1);
                invokeTextChanged();
            }
        } else if (keySymbol == XK_Left) {
            if (cursor_ > 0) {
                --cursor_;
            }
        } else if (keySymbol == XK_Right) {
            if (cursor_ < text_.size()) {
                ++cursor_;
            }
        } else if (keySymbol == XK_Home) {
            cursor_ = 0;
        } else if (keySymbol == XK_End) {
            cursor_ = text_.size();
        } else if (keySymbol == XK_Return) {
        } else if (count > 0 && buffer[0] >= 32) {
            text_.insert(cursor_, buffer, static_cast<size_t>(count));
            cursor_ += static_cast<size_t>(count);
            invokeTextChanged();
        }

        requestRedraw();
    }

    Neu_Control::handleXEvent(event);
}

} // namespace neutrino
