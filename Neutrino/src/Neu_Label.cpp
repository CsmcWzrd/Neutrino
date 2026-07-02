#include "Neutrino/Neutrino.hpp"

namespace neutrino {

namespace {

static int fragmentLineHeight(const Neu_TextFragment& f)
{
    if (f.headingLevel > 0) {
        return std::max(22, 36 - f.headingLevel * 2);
    }
    return 18;
}

} // namespace

void Neu_Label::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    const auto rect = bounds();
    if (borderVisible_) {
        XSetForeground(display, gc, Neu_Pixel(display, theme.border));
        XDrawRectangle(display, drawable, gc, rect.x, rect.y, std::max(1, rect.width - 1), std::max(1, rect.height - 1));
    }
    const int iconSpace = !icon().pixels().empty() ? 24 : 0;
    const int textLeft = rect.x + iconSpace + 2;
    const int textWidth = std::max(1, rect.width - iconSpace - 4);

    if (!icon().pixels().empty()) {
        drawIconBmp(display, drawable, gc, rect.x, rect.y + std::max(0, (rect.height - 16) / 2), 16);
    }

    XRectangle clip{static_cast<short>(textLeft),
                    static_cast<short>(rect.y),
                    static_cast<unsigned short>(std::max(1, textWidth)),
                    static_cast<unsigned short>(std::max(1, rect.height))};
    XSetClipRectangles(display, gc, 0, 0, &clip, 1, Unsorted);

    if (!richTextFragments().empty()) {
        int x = textLeft;
        const int y = rect.y + rect.height / 2 + 5;
        for (const auto& f : richTextFragments()) {
            const std::string visible = truncateText_ ? truncateTextToWidth(display, drawable, gc, theme, f.text, textLeft + textWidth - x) : f.text;
            const Neu_Color color = f.useFontColor ? f.fontColor : theme.text;
            if (f.useHighlightColor || f.useBackgroundColor) {
                const int w = measureTextWidth(display, drawable, gc, theme, visible, f.bold, f.italic, f.monospace, f.headingLevel);
                XSetForeground(display, gc, Neu_Pixel(display, f.useHighlightColor ? f.highlightColor : f.backgroundColor));
                XFillRectangle(display, drawable, gc, x, y - 14, std::max(1, w), 18);
            }
            drawTextColored(display,
                            drawable,
                            gc,
                            theme,
                            visible,
                            x,
                            y,
                            color,
                            f.bold,
                            f.italic,
                            f.underline,
                            f.strikethrough,
                            f.doubleStrikethrough,
                            f.monospace,
                            f.headingLevel);
            x += measureTextWidth(display, drawable, gc, theme, visible, f.bold, f.italic, f.monospace, f.headingLevel) + 2;
            if (x >= textLeft + textWidth) {
                break;
            }
        }
    } else if (wordWrap_) {
        auto lines = wrapTextToWidth(display, drawable, gc, theme, text(), textWidth);
        int y = rect.y + 16;
        for (const auto& line : lines) {
            if (y >= rect.y + rect.height) {
                break;
            }
            drawText(display, drawable, gc, theme, line, alignedTextX(display, drawable, gc, theme, line, textLeft, textWidth), y);
            y += 18;
        }
    } else {
        std::string visible = truncateText_ ? truncateTextToWidth(display, drawable, gc, theme, text(), textWidth) : text();
        drawText(display,
                 drawable,
                 gc,
                 theme,
                 visible,
                 alignedTextX(display, drawable, gc, theme, visible, textLeft, textWidth),
                 rect.y + rect.height / 2 + 5);
    }

    XSetClipMask(display, gc, None);
    drawHintPopup(display, drawable, gc, theme);
}

void Neu_MultilineLabel::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    const auto rect = bounds();
    if (borderVisible_) {
        XSetForeground(display, gc, Neu_Pixel(display, theme.border));
        XDrawRectangle(display, drawable, gc, rect.x, rect.y, std::max(1, rect.width - 1), std::max(1, rect.height - 1));
    }
    const int iconSpace = !icon().pixels().empty() ? 26 : 0;
    const int textLeft = rect.x + iconSpace + 2;
    const int contentWidth = std::max(1, rect.width - iconSpace - 14);

    if (!icon().pixels().empty()) {
        drawIconBmp(display, drawable, gc, rect.x, rect.y + 4, 20);
    }

    XRectangle clip{static_cast<short>(textLeft),
                    static_cast<short>(rect.y),
                    static_cast<unsigned short>(std::max(1, contentWidth)),
                    static_cast<unsigned short>(std::max(1, rect.height))};
    XSetClipRectangles(display, gc, 0, 0, &clip, 1, Unsorted);

    int y = rect.y + 16 - scrollY();
    int naturalHeight = 12;

    if (!richTextFragments().empty()) {
        for (const auto& f : richTextFragments()) {
            const int lineHeight = fragmentLineHeight(f);
            const auto parts = wrapTextToWidth(display, drawable, gc, theme, f.text, contentWidth);
            for (const auto& part : parts) {
                if (y >= rect.y + 12 && y < rect.y + rect.height) {
                    const Neu_Color color = f.useFontColor ? f.fontColor : theme.text;
                    drawTextColored(display,
                                    drawable,
                                    gc,
                                    theme,
                                    truncateText_ ? truncateTextToWidth(display, drawable, gc, theme, part, contentWidth) : part,
                                    textLeft,
                                    y,
                                    color,
                                    f.bold,
                                    f.italic,
                                    f.underline,
                                    f.strikethrough,
                                    f.doubleStrikethrough,
                                    f.monospace,
                                    f.headingLevel);
                }
                y += lineHeight;
                naturalHeight += lineHeight;
            }
        }
    } else {
        const auto lines = wrapTextToWidth(display, drawable, gc, theme, text(), contentWidth);
        for (const auto& line : lines) {
            if (y >= rect.y + 12 && y < rect.y + rect.height) {
                drawText(display,
                         drawable,
                         gc,
                         theme,
                         truncateText_ ? truncateTextToWidth(display, drawable, gc, theme, line, contentWidth) : line,
                         alignedTextX(display, drawable, gc, theme, line, textLeft, contentWidth),
                         y);
            }
            y += 18;
            naturalHeight += 18;
        }
    }

    XSetClipMask(display, gc, None);
    const_cast<Neu_MultilineLabel*>(this)->setAutoScroll(true);
    const_cast<Neu_MultilineLabel*>(this)->setVirtualSize(rect.width, std::max(rect.height, naturalHeight));
    drawScrollbars(display, drawable, gc, theme);
    drawHintPopup(display, drawable, gc, theme);
}

} // namespace neutrino
