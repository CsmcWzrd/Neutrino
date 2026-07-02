#include "Neutrino/Neutrino.hpp"

namespace neutrino {

void Neu_ScrollWindow::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(display, drawable, gc, theme);
    for (const auto& child : children()) {
        if (child && child->visible()) {
            child->draw(display, drawable, gc, theme);
            child->drawHintPopup(display, drawable, gc, theme);
        }
    }
    drawScrollbars(display, drawable, gc, theme);
}

void Neu_ScrollWindow::handleXEvent(XEvent& event)
{
    Neu_Placement::handleXEvent(event);
    Neu_Control::handleXEvent(event);
}

} // namespace neutrino
