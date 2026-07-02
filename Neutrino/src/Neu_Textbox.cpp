#include "Neutrino/Neutrino.hpp"
#include <X11/keysym.h>

namespace neutrino {

namespace {

static void setClip(Display* display, GC gc, const Neu_Rect& rect, int inset)
{
    XRectangle clip{static_cast<short>(rect.x + inset),
                    static_cast<short>(rect.y + inset),
                    static_cast<unsigned short>(std::max(1, rect.width - 2 * inset)),
                    static_cast<unsigned short>(std::max(1, rect.height - 2 * inset))};
    XSetClipRectangles(display, gc, 0, 0, &clip, 1, Unsorted);
}

} // namespace

void Neu_Textbox::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(display, drawable, gc, theme);
    const auto rect = bounds();
    const int textLeft = rect.x + 8;
    const int textTop = rect.y + rect.height / 2 + 5;
    const int textWidth = std::max(1, rect.width - 20);

    const size_t caretBytes = std::min(cursor_, text_.size());
    const std::string prefix = text_.substr(0, caretBytes);
    const int caretWidth = measureTextWidth(display, drawable, gc, theme, prefix);
    const int fullWidth = measureTextWidth(display, drawable, gc, theme, text_);
    if (focused_) {
        if (caretWidth - textScrollX_ > textWidth - 3) {
            textScrollX_ = std::max(0, caretWidth - textWidth + 3);
        }
        if (caretWidth - textScrollX_ < 0) {
            textScrollX_ = std::max(0, caretWidth - 3);
        }
    } else if (fullWidth <= textWidth) {
        textScrollX_ = 0;
    }

    setClip(display, gc, rect, 4);
    if (focused_ || fullWidth > textWidth) {
        drawText(display, drawable, gc, theme, text_, textLeft - textScrollX_, textTop);
    } else {
        std::string visible = truncateText_ ? truncateTextToWidth(display, drawable, gc, theme, text_, textWidth) : text_;
        drawText(display,
                 drawable,
                 gc,
                 theme,
                 visible,
                 alignedTextX(display, drawable, gc, theme, visible, textLeft, textWidth),
                 textTop);
    }

    if (focused_) {
        const int caretX = textLeft + caretWidth - textScrollX_;
        XSetForeground(display, gc, Neu_Pixel(display, theme.accent));
        XDrawLine(display, drawable, gc, caretX, rect.y + 8, caretX, rect.y + rect.height - 8);
    }
    XSetClipMask(display, gc, None);
    drawHintPopup(display, drawable, gc, theme);
}

void Neu_Textbox::handleXEvent(XEvent& event)
{
    if (event.type == ButtonPress && contains(event.xbutton.x, event.xbutton.y)) {
        const auto rect = bounds();
        const int localX = event.xbutton.x - (rect.x + 8) + textScrollX_;
        cursor_ = 0;
        for (size_t i = 1; i <= text_.size(); ++i) {
            if (measureTextWidth(nullptr, 0, 0, Neu_Theme{}, text_.substr(0, i)) <= localX) {
                cursor_ = i;
            } else {
                break;
            }
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
