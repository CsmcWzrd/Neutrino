#include "Neutrino/Neutrino.hpp"

namespace neutrino {

Neu_MultilineLabel::Neu_MultilineLabel(Neu_Layout layout)
    : Neu_Label(layout)
{
    wordWrap_ = true;
    truncateText_ = false;
    autoScroll_ = true;
}

void Neu_Label::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    const auto rect = bounds();
    int textX = rect.x;
    int textWidth = rect.width;
    if (!icon().pixels().empty()) {
        drawIconBmp(display, drawable, gc, rect.x, rect.y + std::max(0, (rect.height - 16) / 2), 16);
        textX += 22;
        textWidth -= 22;
    }

    Neu_Rect textRect{textX, rect.y, std::max(1, textWidth), rect.height};
    if (!richTextFragments().empty()) {
        drawRichTextFragments(display,
                              drawable,
                              gc,
                              theme,
                              richTextFragments(),
                              textRect,
                              Neu_TextLayoutOptions{wordWrap_, truncateText_, textAlign_, 18, 0});
    } else {
        drawTextInRect(display,
                       drawable,
                       gc,
                       theme,
                       text(),
                       textRect,
                       Neu_TextLayoutOptions{wordWrap_, truncateText_, textAlign_, 18, 0});
    }
    drawHintPopup(display, drawable, gc, theme);
}

void Neu_MultilineLabel::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    const auto rect = bounds();
    int textX = rect.x;
    int textWidth = rect.width - 4;
    if (!icon().pixels().empty()) {
        drawIconBmp(display, drawable, gc, rect.x, rect.y + 4, 20);
        textX += 26;
        textWidth -= 26;
    }

    const auto lines = wordWrap_ ? wrapTextToWidth(text(), std::max(1, textWidth - 8)) : std::vector<std::string>{truncateTextToWidth(text(), textWidth - 8)};
    setVirtualSize(rect.width, std::max(rect.height, static_cast<int>(lines.size()) * 18 + 8));
    Neu_Rect textRect{textX + 2 - scrollX(), rect.y + 4 - scrollY(), std::max(1, textWidth), rect.height - 8 + scrollY()};
    if (!richTextFragments().empty()) {
        drawRichTextFragments(display,
                              drawable,
                              gc,
                              theme,
                              richTextFragments(),
                              textRect,
                              Neu_TextLayoutOptions{wordWrap_, truncateText_, textAlign_, 18, 0});
    } else {
        drawTextInRect(display,
                       drawable,
                       gc,
                       theme,
                       text(),
                       textRect,
                       Neu_TextLayoutOptions{wordWrap_, truncateText_, textAlign_, 18, 0});
    }
    drawScrollbars(display, drawable, gc, theme);
    drawHintPopup(display, drawable, gc, theme);
}

} // namespace neutrino
