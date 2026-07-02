#include "Neutrino/Neutrino.hpp"
#include <X11/keysym.h>

namespace neutrino {

void Neu_Multilinetextbox::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(display, drawable, gc, theme);
    const auto rect = bounds();
    Neu_Rect content{rect.x + 8, rect.y + 6, rect.width - 20, rect.height - 18};
    drawTextInRect(display,
                   drawable,
                   gc,
                   theme,
                   text_,
                   Neu_Rect{content.x - scrollX_, content.y - scrollY_, content.width + scrollX_, content.height + scrollY_},
                   Neu_TextLayoutOptions{wordWrap_, truncateText_, textAlign_, 18, 0});

    int lineCount = 1;
    int maxLineWidth = rect.width;
    std::stringstream stream(text_);
    std::string line;
    while (std::getline(stream, line)) {
        ++lineCount;
        maxLineWidth = std::max(maxLineWidth, approximateTextWidth(line) + 32);
    }
    if (wordWrap_) {
        auto wrapped = wrapTextToWidth(text_, rect.width - 24);
        lineCount = static_cast<int>(wrapped.size());
        maxLineWidth = rect.width;
    }
    setAutoScroll(true);
    setVirtualSize(std::max(rect.width, maxLineWidth), std::max(rect.height, lineCount * 18 + 20));
    drawScrollbars(display, drawable, gc, theme);
}

void Neu_Multilinetextbox::handleXEvent(XEvent& event)
{
    if (event.type == KeyPress && focused_ && XLookupKeysym(&event.xkey, 0) == XK_Return) {
        text_.insert(cursor_, "\n");
        ++cursor_;
        invokeTextChanged();
        requestRedraw();
        return;
    }

    Neu_Textbox::handleXEvent(event);
}

} // namespace neutrino
