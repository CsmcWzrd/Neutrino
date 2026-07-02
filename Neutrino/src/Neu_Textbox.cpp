#include "Neutrino/Neutrino.hpp"
#include <X11/keysym.h>

namespace neutrino {

void Neu_Textbox::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(display, drawable, gc, theme);
    const auto rect = bounds();
    const int textX = rect.x + 8;
    const int textY = rect.y + rect.height / 2 + 5;
    drawText(display, drawable, gc, theme, text_, textX, textY);

    if (focused_) {
        const int caretX = textX + static_cast<int>(std::min<size_t>(cursor_, text_.size())) * 8;
        XSetForeground(display, gc, Neu_Pixel(display, theme.accent));
        XDrawLine(display, drawable, gc, caretX, rect.y + 8, caretX, rect.y + rect.height - 8);
    }
}

void Neu_Textbox::handleXEvent(XEvent& event)
{
    if (event.type == ButtonPress && contains(event.xbutton.x, event.xbutton.y)) {
        cursor_ = text_.size();
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
            // Single-line text boxes ignore Enter. Multiline boxes override this.
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
