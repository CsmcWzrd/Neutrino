#include "Neutrino/Neutrino.hpp"

namespace neutrino {

void Neu_Placement::setParent(Neu_Window* parent)
{
    Neu_Control::setParent(parent);
    for (auto& child : children_) {
        if (child) {
            child->setParent(parent);
        }
    }
}

void Neu_Placement::add(std::shared_ptr<Neu_Control> child)
{
    if (!child) {
        return;
    }
    child->setParent(parent_);
    children_.push_back(child);
}

void Neu_Placement::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(display, drawable, gc, theme);
    const auto rect = bounds();
    XRectangle clip{};
    clip.x = static_cast<short>(rect.x + 1);
    clip.y = static_cast<short>(rect.y + 1);
    clip.width = static_cast<unsigned short>(std::max(0, rect.width - 2));
    clip.height = static_cast<unsigned short>(std::max(0, rect.height - 2));
    XSetClipRectangles(display, gc, 0, 0, &clip, 1, Unsorted);
    for (auto& child : children_) {
        if (child && child->visible()) {
            child->draw(display, drawable, gc, theme);
        }
    }
    XSetClipMask(display, gc, None);
}

void Neu_Placement::handleXEvent(XEvent& event)
{
    for (auto& child : children_) {
        if (child && child->visible()) {
            child->handleXEvent(event);
        }
    }
    Neu_Control::handleXEvent(event);
}

} // namespace neutrino
