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
    for (const auto& category : categories_) {
        drawText(display, drawable, gc, theme, category, rect.x + 14, y);
        y += 22;
    }

    if (categories_.empty()) {
        return;
    }

    const int safeIndex = std::min(selectedCategory_, static_cast<int>(categories_.size()) - 1);
    const auto itemIterator = items_.find(categories_[static_cast<size_t>(safeIndex)]);
    if (itemIterator == items_.end()) {
        return;
    }

    y = rect.y + 25;
    for (const auto& item : itemIterator->second) {
        drawText(display, drawable, gc, theme, item, rect.x + sidebarWidth + 18, y);
        y += 22;
    }
}

} // namespace neutrino
