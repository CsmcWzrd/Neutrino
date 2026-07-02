#include "Neutrino/Neutrino.hpp"
#include <X11/keysym.h>

namespace neutrino {

namespace {

static std::vector<std::string> splitVisualLines(const std::string& text)
{
    std::vector<std::string> lines;
    std::stringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }
    if (lines.empty()) {
        lines.push_back({});
    }
    return lines;
}

} // namespace

void Neu_Multilinetextbox::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(display, drawable, gc, theme);
    const auto rect = bounds();
    const int lineHeight = 18;
    const int contentLeft = rect.x + 8;
    const int contentTop = rect.y + 8;
    const int contentWidth = std::max(1, rect.width - 24);

    std::vector<std::string> lines;
    if (wordWrap_) {
        lines = wrapTextToWidth(display, drawable, gc, theme, text_, contentWidth);
    } else {
        lines = splitVisualLines(text_);
    }

    int maxLineWidth = rect.width;
    for (const auto& line : lines) {
        maxLineWidth = std::max(maxLineWidth, measureTextWidth(display, drawable, gc, theme, line) + 24);
    }
    setAutoScroll(true);
    setVirtualSize(std::max(rect.width, maxLineWidth), std::max(rect.height, static_cast<int>(lines.size()) * lineHeight + 16));

    XRectangle clip{static_cast<short>(rect.x + 4),
                    static_cast<short>(rect.y + 4),
                    static_cast<unsigned short>(std::max(1, rect.width - 16)),
                    static_cast<unsigned short>(std::max(1, rect.height - 16))};
    XSetClipRectangles(display, gc, 0, 0, &clip, 1, Unsorted);

    int y = contentTop + 12 - scrollY();
    for (const auto& line : lines) {
        if (y >= rect.y + 12 && y < rect.y + rect.height - 6) {
            const std::string visible = wordWrap_ ? line : truncateTextToWidth(display, drawable, gc, theme, line, contentWidth + scrollX());
            drawText(display, drawable, gc, theme, visible, contentLeft - scrollX(), y);
        }
        y += lineHeight;
    }

    if (focused_) {
        size_t localCursor = std::min(cursor_, text_.size());
        size_t lineIndex = 0;
        size_t lineStart = 0;
        for (size_t i = 0; i < localCursor; ++i) {
            if (text_[i] == '\n') {
                ++lineIndex;
                lineStart = i + 1;
            }
        }
        const size_t colBytes = localCursor >= lineStart ? localCursor - lineStart : 0;
        const std::string prefix = text_.substr(lineStart, colBytes);
        const int caretX = contentLeft - scrollX() + measureTextWidth(display, drawable, gc, theme, prefix);
        const int caretY = contentTop + static_cast<int>(lineIndex) * lineHeight - scrollY();
        XSetForeground(display, gc, Neu_Pixel(display, theme.accent));
        XDrawLine(display, drawable, gc, caretX, caretY, caretX, caretY + lineHeight - 3);
    }

    XSetClipMask(display, gc, None);
    drawScrollbars(display, drawable, gc, theme);
    drawHintPopup(display, drawable, gc, theme);
}

void Neu_Multilinetextbox::handleXEvent(XEvent& event)
{
    if (focused_ && event.type == KeyPress && XLookupKeysym(&event.xkey, 0) == XK_Return) {
        const size_t insertAt = std::min(cursor_, text_.size());
        text_.insert(insertAt, 1, '\n');
        cursor_ = insertAt + 1;
        invokeTextChanged();
        requestRedraw();
        Neu_Control::handleXEvent(event);
        return;
    }

    Neu_Textbox::handleXEvent(event);
}

} // namespace neutrino
