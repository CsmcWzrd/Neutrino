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
    const int track = vertical_ ? rect.height : rect.width;
    const int thumb = std::max(18, track * page_ / std::max(1, total_));
    const int maxValue = std::max(1, total_ - page_);
    const int pos = (track - thumb) * value_ / maxValue;
    XSetForeground(display, gc, Neu_Pixel(display, theme.accent));
    if (vertical_) {
        Neu_DrawRoundedRect(display, drawable, gc, rect.x + 2, rect.y + pos, rect.width - 4, thumb, 5, true);
    } else {
        Neu_DrawRoundedRect(display, drawable, gc, rect.x + pos, rect.y + 2, thumb, rect.height - 4, 5, true);
    }
}

void Neu_ScrollBar::handleXEvent(XEvent& event)
{
    if (event.type == ButtonPress && contains(event.xbutton.x, event.xbutton.y)) {
        const auto rect = bounds();
        int coordinate = vertical_ ? event.xbutton.y - rect.y : event.xbutton.x - rect.x;
        int track = vertical_ ? rect.height : rect.width;
        int maxValue = std::max(1, total_ - page_);
        setRange(total_, page_, coordinate * maxValue / std::max(1, track));
    }
    Neu_Control::handleXEvent(event);
}

} // namespace neutrino
