#include "Neutrino/Neutrino.hpp"

namespace neutrino {

void Neu_Listbox::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(display, drawable, gc, theme);
    const auto rect = bounds();
    const int rowHeight = 18;
    setVirtualSize(rect.width, std::max(rect.height, static_cast<int>(items_.size()) * rowHeight + 8));
    int y = rect.y + 18 - scrollY();

    for (size_t index = 0; index < items_.size(); ++index, y += rowHeight) {
        if (y < rect.y + 8) {
            continue;
        }
        if (y >= rect.y + rect.height) {
            break;
        }
        if (static_cast<int>(index) == selected_) {
            XSetForeground(display, gc, Neu_Pixel(display, theme.hover));
            XFillRectangle(display, drawable, gc, rect.x + 4, y - 14, rect.width - 8, 18);
        }

        drawText(display, drawable, gc, theme, items_[index], rect.x + 8, y);
    }
    drawScrollbars(display, drawable, gc, theme);
}

void Neu_Listbox::handleXEvent(XEvent& event)
{
    if (event.type == ButtonRelease && contains(event.xbutton.x, event.xbutton.y)) {
        const auto rect = bounds();
        const int row = (event.xbutton.y - rect.y + scrollY()) / 18;

        if (row >= 0 && row < static_cast<int>(items_.size())) {
            selected_ = row;

            if (callbacks_.onSelectionChanged) {
                callbacks_.onSelectionChanged(this, row, 0, items_[static_cast<size_t>(row)].c_str(), callbacks_.userData);
            }

            requestRedraw();
        }
    }

    Neu_Control::handleXEvent(event);
}

} // namespace neutrino
