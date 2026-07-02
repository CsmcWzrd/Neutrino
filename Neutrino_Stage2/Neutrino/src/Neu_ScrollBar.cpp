#include "Neutrino/Neutrino.hpp"

namespace neutrino {

void Neu_ScrollBar::setRange(int total, int page, int value)
{
    total_ = std::max(1, total);
    page_ = std::max(1, page);
    value_ = std::max(0, std::min(value, std::max(0, total_ - page_)));
    requestRedraw();
}

void Neu_ScrollBar::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    const auto rect = bounds();
    XSetForeground(display, gc, Neu_Pixel(display, Neu_Color{215, 225, 235, 230}));
    Neu_DrawRoundedRect(display, drawable, gc, rect.x, rect.y, rect.width, rect.height, 6, true);
    const int track = std::max(1, vertical_ ? rect.height : rect.width);
    const int thumb = std::max(18, track * page_ / std::max(1, total_));
    const int maxValue = std::max(1, total_ - page_);
    const int pos = (track - thumb) * value_ / maxValue;
    XSetForeground(display, gc, Neu_Pixel(display, hover_ ? theme.pressed : theme.accent));
    if (vertical_) {
        Neu_DrawRoundedRect(display, drawable, gc, rect.x + 2, rect.y + pos, rect.width - 4, thumb, 5, true);
    } else {
        Neu_DrawRoundedRect(display, drawable, gc, rect.x + pos, rect.y + 2, thumb, rect.height - 4, 5, true);
    }
    drawHintPopup(display, drawable, gc, theme);
}

void Neu_ScrollBar::handleXEvent(XEvent& event)
{
    const bool isPointer = event.type == ButtonPress || event.type == ButtonRelease || event.type == MotionNotify;
    int px = 0;
    int py = 0;
    if (event.type == MotionNotify) {
        px = event.xmotion.x;
        py = event.xmotion.y;
    } else if (event.type == ButtonPress || event.type == ButtonRelease) {
        px = event.xbutton.x;
        py = event.xbutton.y;
    }

    if (isPointer && contains(px, py)) {
        const auto rect = bounds();
        const int coordinate = vertical_ ? py - rect.y : px - rect.x;
        const int track = std::max(1, vertical_ ? rect.height : rect.width);
        const int maxValue = std::max(1, total_ - page_);
        if (event.type == ButtonPress && event.xbutton.button == Button1) {
            dragging_ = true;
            setRange(total_, page_, coordinate * maxValue / track);
            return;
        }
        if (event.type == MotionNotify && dragging_) {
            setRange(total_, page_, coordinate * maxValue / track);
            return;
        }
    }

    if (event.type == ButtonRelease) {
        dragging_ = false;
    }
    Neu_Control::handleXEvent(event);
}

} // namespace neutrino
