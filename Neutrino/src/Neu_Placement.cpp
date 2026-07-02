#include "Neutrino/Neutrino.hpp"

namespace neutrino {

void Neu_Placement::add(std::shared_ptr<Neu_Control> child)
{
    if (!child) {
        return;
    }

    child->setParent(parent_);
    children_.push_back(child);
}

void Neu_Placement::setParent(Neu_Window* parent)
{
    Neu_Control::setParent(parent);
    for (auto& child : children_) {
        if (child) {
            child->setParent(parent);
        }
    }
}

void Neu_Placement::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(display, drawable, gc, theme);

    for (auto& child : children_) {
        if (child->visible()) {
            child->draw(display, drawable, gc, theme);
        }
    }
}

void Neu_Placement::handleXEvent(XEvent& event)
{
    int x = 0;
    int y = 0;
    if (event.type == MotionNotify) {
        x = event.xmotion.x;
        y = event.xmotion.y;
    } else if (event.type == ButtonPress || event.type == ButtonRelease) {
        x = event.xbutton.x;
        y = event.xbutton.y;
    }

    if (event.type == MotionNotify || event.type == ButtonPress || event.type == ButtonRelease) {
        for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
            const auto& child = *it;
            if (child && child->visible() && child->enabled() && child->contains(x, y)) {
                child->handleXEvent(event);
                return;
            }
        }
    }

    Neu_Control::handleXEvent(event);
}

} // namespace neutrino
