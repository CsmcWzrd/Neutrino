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
    const int lineHeight = 18;
    std::stringstream stream(text());
    std::string line;
    int lineNo = 1;
    int y = rect.y + 20 - scrollY();
    int maxWidth = rect.width;

    XSetForeground(display, gc, Neu_Pixel(display, Neu_Color{225, 232, 242, 255}));
    XFillRectangle(display, drawable, gc, rect.x + 1, rect.y + 1, 46, rect.height - 2);
    drawText(display, drawable, gc, theme, languageName_, rect.x + rect.width - 90, rect.y + 16);

    while (std::getline(stream, line)) {
        maxWidth = std::max(maxWidth, static_cast<int>(line.size()) * 8 + 70);
        if (y >= rect.y + 16 && y < rect.y + rect.height - 4) {
            drawText(display, drawable, gc, theme, std::to_string(lineNo), rect.x + 8, y);
            if (startsWithKeyword(line, "#include") || startsWithKeyword(line, "class") || startsWithKeyword(line, "int") || startsWithKeyword(line, "void") || startsWithKeyword(line, "auto")) {
                XSetForeground(display, gc, Neu_Pixel(display, Neu_Color{230, 240, 255, 255}));
                XFillRectangle(display, drawable, gc, rect.x + 50, y - 14, rect.width - 62, lineHeight);
            }
            drawText(display, drawable, gc, theme, line, rect.x + 56 - scrollX(), y);
        }
        y += lineHeight;
        ++lineNo;
    }

    setAutoScroll(true);
    setVirtualSize(maxWidth, std::max(rect.height, (lineNo + 1) * lineHeight));
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
