#include "Neutrino/Neutrino.hpp"
#include <X11/keysym.h>

namespace neutrino {

static bool startsWithKeyword(const std::string& line, const std::string& keyword)
{
    size_t first = line.find_first_not_of(" \t");
    return first != std::string::npos && line.compare(first, keyword.size(), keyword) == 0;
}

void Neu_RichTextCode::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(display, drawable, gc, theme);
    const auto rect = bounds();
    const int toolbarHeight = toolbarVisible_ ? 30 : 0;
    const int lineHeight = 18;

    if (toolbarVisible_) {
        XSetForeground(display, gc, Neu_Pixel(display, Neu_Color{225, 232, 242, 255}));
        XFillRectangle(display, drawable, gc, rect.x + 1, rect.y + 1, rect.width - 2, toolbarHeight - 1);
        const std::string toolbar = "B I U S SS H1 H2 H3 H4 H5 H6 H7 Normal Mono Font Fg Bg Highlight Left Center Right Wrap";
        drawTextInRect(display,
                       drawable,
                       gc,
                       theme,
                       toolbar,
                       Neu_Rect{rect.x + 8, rect.y + 6, rect.width - 16, toolbarHeight - 8},
                       Neu_TextLayoutOptions{false, true, Neu_TextAlign::Left, lineHeight, 0});
    }

    const int contentTop = rect.y + toolbarHeight;
    Neu_Rect content{rect.x + 1, contentTop + 1, rect.width - 14, rect.height - toolbarHeight - 14};
    if (content.width <= 0 || content.height <= 0) {
        return;
    }

    XRectangle clip{};
    clip.x = static_cast<short>(content.x);
    clip.y = static_cast<short>(content.y);
    clip.width = static_cast<unsigned short>(std::max(0, content.width));
    clip.height = static_cast<unsigned short>(std::max(0, content.height));
    XSetClipRectangles(display, gc, 0, 0, &clip, 1, Unsorted);

    XSetForeground(display, gc, Neu_Pixel(display, Neu_Color{225, 232, 242, 255}));
    XFillRectangle(display, drawable, gc, rect.x + 1, contentTop + 1, 48, rect.height - toolbarHeight - 2);
    drawTextInRect(display,
                   drawable,
                   gc,
                   theme,
                   languageName_,
                   Neu_Rect{rect.x + rect.width - 110, rect.y + toolbarHeight + 2, 100, 22},
                   Neu_TextLayoutOptions{false, true, Neu_TextAlign::Right, lineHeight, 0});

    if (!richTextFragments().empty()) {
        drawRichTextFragments(display,
                              drawable,
                              gc,
                              theme,
                              richTextFragments(),
                              Neu_Rect{rect.x + 56 - scrollX(), contentTop + 8 - scrollY(), content.width - 58 + scrollX(), content.height + scrollY()},
                              Neu_TextLayoutOptions{wordWrap_, truncateText_, textAlign_, lineHeight, 0});
        XSetClipMask(display, gc, None);
        setAutoScroll(true);
        setVirtualSize(std::max(rect.width, 1200), std::max(rect.height, 120));
        drawScrollbars(display, drawable, gc, theme);
        return;
    }

    std::stringstream stream(text());
    std::string line;
    int lineNo = 1;
    int y = contentTop + 22 - scrollY();
    int maxWidth = rect.width;
    int renderedLines = 0;

    while (std::getline(stream, line)) {
        maxWidth = std::max(maxWidth, approximateTextWidth(line, Neu_TextStyle_Monospaced) + 80);
        if (y >= contentTop + 16 && y < rect.y + rect.height - 10) {
            drawTextInRect(display,
                           drawable,
                           gc,
                           theme,
                           std::to_string(lineNo),
                           Neu_Rect{rect.x + 6, y - 15, 38, lineHeight},
                           Neu_TextLayoutOptions{false, true, Neu_TextAlign::Right, lineHeight, 0});
            if (startsWithKeyword(line, "#include") || startsWithKeyword(line, "class") || startsWithKeyword(line, "int") || startsWithKeyword(line, "void") || startsWithKeyword(line, "auto")) {
                XSetForeground(display, gc, Neu_Pixel(display, Neu_Color{230, 240, 255, 255}));
                XFillRectangle(display, drawable, gc, rect.x + 50, y - 15, rect.width - 62, lineHeight);
            }
            drawTextInRect(display,
                           drawable,
                           gc,
                           theme,
                           line,
                           Neu_Rect{rect.x + 56 - scrollX(), y - 15, rect.width - 70 + scrollX(), lineHeight},
                           Neu_TextLayoutOptions{wordWrap_, truncateText_, Neu_TextAlign::Left, lineHeight, 0});
        }
        y += lineHeight;
        ++lineNo;
        ++renderedLines;
    }

    XSetClipMask(display, gc, None);
    setAutoScroll(true);
    if (wordWrap_) {
        maxWidth = rect.width;
    }
    setVirtualSize(std::max(rect.width, maxWidth), std::max(rect.height, (renderedLines + 2) * lineHeight + toolbarHeight));
    drawScrollbars(display, drawable, gc, theme);
    drawHintPopup(display, drawable, gc, theme);
}

void Neu_RichTextCode::handleXEvent(XEvent& event)
{
    if (readOnly_ && event.type == KeyPress) {
        Neu_Control::handleXEvent(event);
        return;
    }
    Neu_Multilinetextbox::handleXEvent(event);
}

} // namespace neutrino
