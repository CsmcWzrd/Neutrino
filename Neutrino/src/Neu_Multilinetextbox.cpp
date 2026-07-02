#include "Neutrino/Neutrino.hpp"
#include <X11/keysym.h>

namespace neutrino {

void Neu_Multilinetextbox::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(display, drawable, gc, theme);
    const auto rect = bounds();
    std::stringstream stream(text_);
    std::string line;
    int y = rect.y + 18 - scrollY();
    int count = 0;

    while (std::getline(stream, line)) {
        if (y >= rect.y + 12 && y < rect.y + rect.height) {
            drawText(display, drawable, gc, theme, line, rect.x + 8 - scrollX(), y);
        }
        y += 16;
        ++count;
    }
    setVirtualSize(std::max(rect.width, 800), std::max(rect.height, count * 16 + 12));
    drawScrollbars(display, drawable, gc, theme);

    if (focused_) {
        XSetForeground(display, gc, Neu_Pixel(display, theme.accent));
        XDrawLine(display, drawable, gc, rect.x + 8, rect.y + rect.height - 10, rect.x + 20, rect.y + rect.height - 10);
    }
}

void Neu_Multilinetextbox::handleXEvent(XEvent& event)
{
    if (event.type == KeyPress && focused_ && XLookupKeysym(&event.xkey, 0) == XK_Return) {
        const size_t insertAt = std::min(cursor_, text_.size());
        text_.insert(insertAt, 1, '\n');
        cursor_ = insertAt + 1;
        invokeTextChanged();
        requestRedraw();
        Neu_Control::handleXEvent(event);
        return;
    }

    Neu_Textbox::handleXEvent(event);
}

} // namespace neutrino
