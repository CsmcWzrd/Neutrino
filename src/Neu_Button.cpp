#include "Neutrino/Neutrino.hpp"

namespace neutrino {

void Neu_Button::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    const auto rect = bounds();
    const Neu_Color fill = pressed_ ? theme.pressed : (hover_ ? theme.highlight : theme.glass);

    XSetForeground(display, gc, Neu_Pixel(display, fill));
    Neu_DrawRoundedRect(display, drawable, gc, rect.x, rect.y, rect.width, rect.height, theme.radius, true);
    XSetForeground(display, gc, Neu_Pixel(display, theme.border));
    Neu_DrawRoundedRect(display, drawable, gc, rect.x, rect.y, rect.width, rect.height, theme.radius, false);
    int textX = rect.x + 12;
    if (!icon().pixels().empty()) {
        drawIconBmp(display, drawable, gc, rect.x + 10, rect.y + std::max(2, (rect.height - 20) / 2), 20);
        textX += 26;
    }
    drawText(display, drawable, gc, theme, text_, textX, rect.y + rect.height / 2 + 5);
}

void Neu_Button::handleXEvent(XEvent& event)
{
    if (event.type == ButtonPress && contains(event.xbutton.x, event.xbutton.y)) {
        pressed_ = true;
        requestRedraw();
    }

    if (event.type == ButtonRelease) {
        const bool hit = contains(event.xbutton.x, event.xbutton.y);
        const bool wasPressed = pressed_;
        if (wasPressed && hit) {
            invokeClick();
        }
        pressed_ = false;
        if (wasPressed) {
            requestRedraw();
        }
    }

    Neu_Control::handleXEvent(event);
}

} // namespace neutrino
