#include "Neutrino/Neutrino.hpp"

namespace neutrino {

void Neu_PopWindowMenu::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(display, drawable, gc, theme);
    const auto rect = bounds();
    const int sidebarWidth = std::min(180, rect.width / 3);

    XSetForeground(display, gc, Neu_Pixel(display, theme.hover));
    Neu_DrawRoundedRect(display,
                      drawable,
                      gc,
                      rect.x + 4,
                      rect.y + 4,
                      sidebarWidth - 8,
                      rect.height - 8,
                      theme.radius,
                      true);

    int y = rect.y + 25;
    for (int index = 0; index < static_cast<int>(categories_.size()); ++index) {
        const auto& category = categories_[static_cast<size_t>(index)];
        if (index == selectedCategory_) {
            XSetForeground(display, gc, Neu_Pixel(display, theme.pressed));
            XFillRectangle(display, drawable, gc, rect.x + 8, y - 16, std::max(1, sidebarWidth - 16), 20);
        }
        drawText(display, drawable, gc, theme, category, rect.x + 14, y);
        y += 22;
    }

    if (categories_.empty()) {
        return;
    }

    selectedCategory_ = std::max(0, std::min(selectedCategory_, static_cast<int>(categories_.size()) - 1));
    const auto itemIterator = items_.find(categories_[static_cast<size_t>(selectedCategory_)]);
    if (itemIterator == items_.end()) {
        return;
    }

    y = rect.y + 25;
    for (const auto& item : itemIterator->second) {
        drawText(display, drawable, gc, theme, item, rect.x + sidebarWidth + 18, y);
        y += 22;
    }
}

void Neu_PopWindowMenu::handleXEvent(XEvent& event)
{
    if (!visible_) {
        return;
    }

    const auto rect = bounds();
    const int sidebarWidth = std::min(180, rect.width / 3);

    if (event.type == ButtonRelease && contains(event.xbutton.x, event.xbutton.y)) {
        if (event.xbutton.x >= rect.x && event.xbutton.x < rect.x + sidebarWidth) {
            const int row = (event.xbutton.y - (rect.y + 9)) / 22;
            if (row >= 0 && row < static_cast<int>(categories_.size())) {
                selectedCategory_ = row;
                requestRedraw();
                return;
            }
        }
    }

    Neu_Placement::handleXEvent(event);
}

} // namespace neutrino
