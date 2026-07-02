#include "Neutrino/Neutrino.hpp"

namespace neutrino {

void Neu_ImageView::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(display, drawable, gc, theme);
    const auto rect = bounds();
    const int maxSize = std::min(rect.width - 12, rect.height - 12);
    drawIconBmp(display, drawable, gc, rect.x + 6, rect.y + 6, std::max(1, maxSize));
    drawHintPopup(display, drawable, gc, theme);
}

} // namespace neutrino
