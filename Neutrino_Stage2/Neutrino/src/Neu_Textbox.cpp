#include "Neutrino/Neutrino.hpp"
#include <X11/keysym.h>
#include <X11/Xutil.h>

namespace neutrino {

namespace {

static std::string g_neuClipboardText;

static void setClip(Display* display, GC gc, const Neu_Rect& rect, int inset)
{
    XRectangle clip{static_cast<short>(rect.x + inset),
                    static_cast<short>(rect.y + inset),
                    static_cast<unsigned short>(std::max(1, rect.width - 2 * inset)),
                    static_cast<unsigned short>(std::max(1, rect.height - 2 * inset))};
    XSetClipRectangles(display, gc, 0, 0, &clip, 1, Unsorted);
}

} // namespace

void Neu_Textbox::selectAll()
{
    selectionStart_ = 0;
    selectionEnd_ = text_.size();
    cursor_ = selectionEnd_;
    requestRedraw();
}

void Neu_Textbox::clearSelection()
{
    selectionStart_ = cursor_;
    selectionEnd_ = cursor_;
    requestRedraw();
}

void Neu_Textbox::setSelection(size_t start, size_t end)
{
    selectionStart_ = std::min(start, text_.size());
    selectionEnd_ = std::min(end, text_.size());
    cursor_ = selectionEnd_;
    requestRedraw();
}

bool Neu_Textbox::hasSelection() const
{
    return selectionStart_ != selectionEnd_;
}

std::string Neu_Textbox::selectedText() const
{
    if (!hasSelection()) {
        return {};
    }
    const size_t a = selectionStart();
    const size_t b = selectionEnd();
    return text_.substr(a, b - a);
}

void Neu_Textbox::deleteSelection()
{
    if (!hasSelection()) {
        return;
    }
    const size_t a = selectionStart();
    const size_t b = selectionEnd();
    text_.erase(a, b - a);
    cursor_ = a;
    selectionStart_ = cursor_;
    selectionEnd_ = cursor_;
    invokeTextChanged();
}

void Neu_Textbox::replaceSelectionWith(const std::string& text)
{
    if (hasSelection()) {
        const size_t a = selectionStart();
        const size_t b = selectionEnd();
        text_.replace(a, b - a, text);
        cursor_ = a + text.size();
    } else {
        cursor_ = std::min(cursor_, text_.size());
        text_.insert(cursor_, text);
        cursor_ += text.size();
    }
    selectionStart_ = cursor_;
    selectionEnd_ = cursor_;
    invokeTextChanged();
}

void Neu_Textbox::moveCursorWithSelection(size_t newCursor, bool extendSelection)
{
    newCursor = std::min(newCursor, text_.size());
    if (extendSelection) {
        if (!hasSelection()) {
            selectionStart_ = cursor_;
        }
        selectionEnd_ = newCursor;
    } else {
        selectionStart_ = newCursor;
        selectionEnd_ = newCursor;
    }
    cursor_ = newCursor;
    requestRedraw();
}

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
    if (focused_ && hasSelection()) {
        const size_t a = selectionStart();
        const size_t b = selectionEnd();
        const int sx = textLeft - textScrollX_ + measureTextWidth(display, drawable, gc, theme, text_.substr(0, a));
        const int ex = textLeft - textScrollX_ + measureTextWidth(display, drawable, gc, theme, text_.substr(0, b));
        XSetForeground(display, gc, Neu_Pixel(display, theme.highlight));
        XFillRectangle(display, drawable, gc, std::min(sx, ex), rect.y + 6, std::max(1, std::abs(ex - sx)), rect.height - 12);
    }

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
        size_t newCursor = 0;
        for (size_t i = 1; i <= text_.size(); ++i) {
            if (measureTextWidth(nullptr, 0, 0, Neu_Theme{}, text_.substr(0, i)) <= localX) {
                newCursor = i;
            } else {
                break;
            }
        }
        moveCursorWithSelection(newCursor, (event.xbutton.state & ShiftMask) != 0);
        return;
    }

    if (event.type == KeyPress && focused_) {
        char buffer[32] = {0};
        KeySym keySymbol = 0;
        const int count = XLookupString(&event.xkey, buffer, sizeof(buffer) - 1, &keySymbol, nullptr);
        const bool ctrl = (event.xkey.state & ControlMask) != 0;
        const bool shift = (event.xkey.state & ShiftMask) != 0;

        if (ctrl && (keySymbol == XK_a || keySymbol == XK_A)) {
            selectAll();
            return;
        }
        if (ctrl && (keySymbol == XK_c || keySymbol == XK_C)) {
            if (hasSelection()) {
                g_neuClipboardText = selectedText();
            }
            return;
        }
        if (ctrl && (keySymbol == XK_x || keySymbol == XK_X)) {
            if (hasSelection()) {
                g_neuClipboardText = selectedText();
                deleteSelection();
            }
            requestRedraw();
            return;
        }
        if (ctrl && (keySymbol == XK_v || keySymbol == XK_V)) {
            if (!g_neuClipboardText.empty()) {
                replaceSelectionWith(g_neuClipboardText);
            }
            requestRedraw();
            return;
        }

        if (keySymbol == XK_BackSpace) {
            if (hasSelection()) {
                deleteSelection();
            } else if (cursor_ > 0 && !text_.empty()) {
                text_.erase(cursor_ - 1, 1);
                --cursor_;
                clearSelection();
                invokeTextChanged();
            }
        } else if (keySymbol == XK_Delete) {
            if (hasSelection()) {
                deleteSelection();
            } else if (cursor_ < text_.size()) {
                text_.erase(cursor_, 1);
                clearSelection();
                invokeTextChanged();
            }
        } else if (keySymbol == XK_Left) {
            moveCursorWithSelection(cursor_ > 0 ? cursor_ - 1 : 0, shift);
        } else if (keySymbol == XK_Right) {
            moveCursorWithSelection(cursor_ < text_.size() ? cursor_ + 1 : text_.size(), shift);
        } else if (keySymbol == XK_Home) {
            moveCursorWithSelection(0, shift);
        } else if (keySymbol == XK_End) {
            moveCursorWithSelection(text_.size(), shift);
        } else if (keySymbol == XK_Return) {
            // Single-line text boxes ignore Enter. Multiline boxes override this.
        } else if (count > 0 && !ctrl && static_cast<unsigned char>(buffer[0]) >= 32) {
            replaceSelectionWith(std::string(buffer, static_cast<size_t>(count)));
        }

        requestRedraw();
        return;
    }

    Neu_Control::handleXEvent(event);
}

} // namespace neutrino
