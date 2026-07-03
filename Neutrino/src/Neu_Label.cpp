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
    const auto off = textOffset_;
    if (borderVisible_) {
        XSetForeground(display, gc, Neu_Pixel(display, theme.border));
        XDrawRectangle(display, drawable, gc, rect.x, rect.y, std::max(1, rect.width - 1), std::max(1, rect.height - 1));
    }

    const int iconSize = !icon().pixels().empty() ? 16 : 0;
    const int iconSpace = iconSize > 0 ? iconSize + 6 : 0;
    const int contentLeft = rect.x + off.left;
    const int contentTop = rect.y + off.top;
    const int contentRight = rect.x + rect.width - off.right;
    const int contentBottom = rect.y + rect.height - off.bottom;
    const int textLeft = contentLeft + iconSpace;
    const int textWidth = std::max(1, contentRight - textLeft);
    const int contentHeight = std::max(1, contentBottom - contentTop);

    if (iconSize > 0) {
        drawIconBmp(display, drawable, gc, contentLeft, contentTop + std::max(0, (contentHeight - iconSize) / 2), iconSize);
    }

    XRectangle clip{static_cast<short>(std::max(rect.x, textLeft)),
                    static_cast<short>(std::max(rect.y, contentTop)),
                    static_cast<unsigned short>(std::max(1, std::min(rect.x + rect.width, contentRight) - std::max(rect.x, textLeft))),
                    static_cast<unsigned short>(std::max(1, std::min(rect.y + rect.height, contentBottom) - std::max(rect.y, contentTop)))};
    XSetClipRectangles(display, gc, 0, 0, &clip, 1, Unsorted);

    if (!richTextFragments().empty()) {
        int x = textLeft;
        const int y = contentTop + std::max(14, contentHeight / 2 + 5);
        for (const auto& f : richTextFragments()) {
            if (x >= contentRight) {
                break;
            }
            const int remaining = std::max(1, contentRight - x);
            const std::string visible = truncateText_ ? truncateTextToWidth(display, drawable, gc, theme, f.text, remaining) : f.text;
            const Neu_Color color = f.useFontColor ? f.fontColor : theme.text;
            if (f.useHighlightColor || f.useBackgroundColor) {
                const int w = measureTextWidth(display, drawable, gc, theme, visible, f.bold, f.italic, f.monospace, f.headingLevel);
                XSetForeground(display, gc, Neu_Pixel(display, f.useHighlightColor ? f.highlightColor : f.backgroundColor));
                XFillRectangle(display, drawable, gc, x, y - 14, std::max(1, std::min(w, remaining)), 18);
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
        }
    } else if (wordWrap_) {
        auto lines = wrapTextToWidth(display, drawable, gc, theme, text(), textWidth);
        int y = contentTop + 14;
        for (const auto& line : lines) {
            if (y >= contentBottom) {
                break;
            }
            const std::string visible = truncateText_ ? truncateTextToWidth(display, drawable, gc, theme, line, textWidth) : line;
            drawText(display, drawable, gc, theme, visible, alignedTextX(display, drawable, gc, theme, visible, textLeft, textWidth), y);
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
                 contentTop + std::max(14, contentHeight / 2 + 5));
    }

    XSetClipMask(display, gc, None);
    drawHintPopup(display, drawable, gc, theme);
}

void Neu_MultilineLabel::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    const auto rect = bounds();
    const auto off = textOffset_;
    if (borderVisible_) {
        XSetForeground(display, gc, Neu_Pixel(display, theme.border));
        XDrawRectangle(display, drawable, gc, rect.x, rect.y, std::max(1, rect.width - 1), std::max(1, rect.height - 1));
    }

    const int iconSize = !icon().pixels().empty() ? 20 : 0;
    const int iconSpace = iconSize > 0 ? iconSize + 6 : 0;
    const int contentLeft = rect.x + off.left;
    const int contentTop = rect.y + off.top;
    const int contentRight = rect.x + rect.width - off.right - 10;
    const int contentBottom = rect.y + rect.height - off.bottom;
    const int textLeft = contentLeft + iconSpace;
    const int contentWidth = std::max(1, contentRight - textLeft);

    if (iconSize > 0) {
        drawIconBmp(display, drawable, gc, contentLeft, contentTop + 2, iconSize);
    }

    XRectangle clip{static_cast<short>(std::max(rect.x, textLeft)),
                    static_cast<short>(std::max(rect.y, contentTop)),
                    static_cast<unsigned short>(std::max(1, std::min(rect.x + rect.width, contentRight) - std::max(rect.x, textLeft))),
                    static_cast<unsigned short>(std::max(1, std::min(rect.y + rect.height, contentBottom) - std::max(rect.y, contentTop)))};
    XSetClipRectangles(display, gc, 0, 0, &clip, 1, Unsorted);

    int y = contentTop + 14 - scrollY();
    int naturalHeight = 12;

    if (!richTextFragments().empty()) {
        for (const auto& f : richTextFragments()) {
            const int lineHeight = fragmentLineHeight(f);
            const auto parts = wrapTextToWidth(display, drawable, gc, theme, f.text, contentWidth);
            for (const auto& part : parts) {
                if (y >= contentTop + 12 && y < contentBottom) {
                    const Neu_Color color = f.useFontColor ? f.fontColor : theme.text;
                    const std::string visible = truncateText_ ? truncateTextToWidth(display, drawable, gc, theme, part, contentWidth) : part;
                    drawTextColored(display,
                                    drawable,
                                    gc,
                                    theme,
                                    visible,
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
        const auto lines = wordWrap_ ? wrapTextToWidth(display, drawable, gc, theme, text(), contentWidth)
                                     : std::vector<std::string>{text()};
        for (const auto& line : lines) {
            if (y >= contentTop + 12 && y < contentBottom) {
                const std::string visible = truncateText_ ? truncateTextToWidth(display, drawable, gc, theme, line, contentWidth) : line;
                drawText(display,
                         drawable,
                         gc,
                         theme,
                         visible,
                         alignedTextX(display, drawable, gc, theme, visible, textLeft, contentWidth),
                         y);
            }
            y += 18;
            naturalHeight += 18;
        }
    }

    XSetClipMask(display, gc, None);
    const_cast<Neu_MultilineLabel*>(this)->setAutoScroll(true);
    const_cast<Neu_MultilineLabel*>(this)->setVirtualSize(rect.width, std::max(rect.height, naturalHeight + off.top + off.bottom));
    drawScrollbars(display, drawable, gc, theme);
    drawHintPopup(display, drawable, gc, theme);
}

} // namespace neutrino
