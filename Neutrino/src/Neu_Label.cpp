#include "Neutrino/Neutrino.hpp"

namespace neutrino {

void Neu_Label::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    const auto rect = bounds();
    if (!icon().pixels().empty()) {
        drawIconBmp(display, drawable, gc, rect.x, rect.y + std::max(0, (rect.height - 16) / 2), 16);
        drawText(display, drawable, gc, theme, text(), rect.x + 22, rect.y + rect.height / 2 + 5);
    } else {
        drawText(display, drawable, gc, theme, text(), rect.x, rect.y + rect.height / 2 + 5);
    }
    drawHintPopup(display, drawable, gc, theme);
}

void Neu_MultilineLabel::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    const auto rect = bounds();
    int y = rect.y + 16 - scrollY();
    int textX = rect.x;
    if (!icon().pixels().empty()) {
        drawIconBmp(display, drawable, gc, rect.x, rect.y + 4, 20);
        textX += 26;
    }
    std::stringstream stream(text());
    std::string line;
    int lineCount = 0;
    while (std::getline(stream, line)) {
        if (y >= rect.y + 12 && y < rect.y + rect.height) {
            drawText(display, drawable, gc, theme, line, textX, y);
        }
        y += 18;
        ++lineCount;
    }
    const_cast<Neu_MultilineLabel*>(this)->setVirtualSize(rect.width, std::max(rect.height, lineCount * 18 + 12));
    drawScrollbars(display, drawable, gc, theme);
    drawHintPopup(display, drawable, gc, theme);
}

} // namespace neutrino
