#include "Neutrino/Neutrino.hpp"
#include <X11/keysym.h>

namespace neutrino {

namespace {

static std::vector<std::string> splitVisualLines(const std::string& text)
{
    std::vector<std::string> lines;
    std::string current;
    for (size_t i = 0; i < text.size(); ++i) {
        char ch = text[i];
        if (ch == '\r') {
            if (i + 1 < text.size() && text[i + 1] == '\n') {
                ++i;
            }
            lines.push_back(current);
            current.clear();
        } else if (ch == '\n') {
            lines.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    lines.push_back(current);
    if (lines.empty()) {
        lines.push_back({});
    }
    return lines;
}

static size_t lineStartOffset(const std::string& text, int targetLine)
{
    int line = 0;
    size_t start = 0;
    for (size_t i = 0; i < text.size() && line < targetLine; ++i) {
        if (text[i] == '\r') {
            if (i + 1 < text.size() && text[i + 1] == '\n') {
                ++i;
            }
            start = i + 1;
            ++line;
        } else if (text[i] == '\n') {
            start = i + 1;
            ++line;
        }
    }
    return std::min(start, text.size());
}

static size_t lineEndOffset(const std::string& text, size_t start)
{
    size_t end = start;
    while (end < text.size() && text[end] != '\n' && text[end] != '\r') {
        ++end;
    }
    return end;
}

static int lineIndexForOffset(const std::string& text, size_t offset)
{
    offset = std::min(offset, text.size());
    int line = 0;
    for (size_t i = 0; i < offset && i < text.size(); ++i) {
        if (text[i] == '\r') {
            if (i + 1 < offset && i + 1 < text.size() && text[i + 1] == '\n') {
                ++i;
            }
            ++line;
        } else if (text[i] == '\n') {
            ++line;
        }
    }
    return line;
}

static size_t byteOffsetFromX(const std::string& text, size_t start, size_t end, int x)
{
    size_t cursor = start;
    int width = 0;
    for (size_t i = start; i < end; ++i) {
        width += (text[i] == '\t') ? 32 : 8;
        if (width <= x) {
            cursor = i + 1;
        } else {
            break;
        }
    }
    return cursor;
}

static int estimatedTextWidth(const std::string& s)
{
    int width = 0;
    for (char ch : s) {
        width += (ch == '\t') ? 32 : 8;
    }
    return width;
}

static size_t moveVerticallyByLines(const std::string& text, size_t cursor, int deltaLines)
{
    if (text.empty()) {
        return 0;
    }
    cursor = std::min(cursor, text.size());
    const int currentLine = lineIndexForOffset(text, cursor);
    const size_t currentStart = lineStartOffset(text, currentLine);
    const std::string currentPrefix = text.substr(currentStart, cursor - currentStart);
    const int preferredX = estimatedTextWidth(currentPrefix);
    const auto lines = splitVisualLines(text);
    const int lastLine = std::max(0, static_cast<int>(lines.size()) - 1);
    const int targetLine = std::max(0, std::min(lastLine, currentLine + deltaLines));
    const size_t targetStart = lineStartOffset(text, targetLine);
    const size_t targetEnd = lineEndOffset(text, targetStart);
    return byteOffsetFromX(text, targetStart, targetEnd, preferredX);
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
        const auto logical = splitVisualLines(text_);
        for (const auto& base : logical) {
            const auto wrapped = wrapTextToWidth(display, drawable, gc, theme, base, contentWidth);
            lines.insert(lines.end(), wrapped.begin(), wrapped.end());
        }
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
    size_t lineOffset = 0;
    for (const auto& line : lines) {
        if (y >= rect.y + 12 && y < rect.y + rect.height - 6) {
            if (focused_ && hasSelection()) {
                const size_t a = selectionStart();
                const size_t b = selectionEnd();
                const size_t lineStart = lineOffset;
                const size_t lineEnd = lineOffset + line.size();
                if (b > lineStart && a < lineEnd) {
                    const size_t selA = std::max(a, lineStart) - lineStart;
                    const size_t selB = std::min(b, lineEnd) - lineStart;
                    const int sx = contentLeft - scrollX() + measureTextWidth(display, drawable, gc, theme, line.substr(0, selA));
                    const int ex = contentLeft - scrollX() + measureTextWidth(display, drawable, gc, theme, line.substr(0, selB));
                    XSetForeground(display, gc, Neu_Pixel(display, theme.highlight));
                    XFillRectangle(display, drawable, gc, std::min(sx, ex), y - lineHeight + 4, std::max(1, std::abs(ex - sx)), lineHeight);
                }
            }
            const std::string visible = wordWrap_ ? line : truncateTextToWidth(display, drawable, gc, theme, line, contentWidth + scrollX());
            drawText(display, drawable, gc, theme, visible, contentLeft - (wordWrap_ ? 0 : scrollX()), y);
        }
        lineOffset += line.size();
        if (lineOffset < text_.size()) {
            if (text_[lineOffset] == '\r' && lineOffset + 1 < text_.size() && text_[lineOffset + 1] == '\n') {
                lineOffset += 2;
            } else if (text_[lineOffset] == '\r' || text_[lineOffset] == '\n') {
                ++lineOffset;
            }
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
            } else if (text_[i] == '\r') {
                if (i + 1 < localCursor && text_[i + 1] == '\n') {
                    ++i;
                }
                ++lineIndex;
                lineStart = i + 1;
            }
        }
        const size_t colBytes = localCursor >= lineStart ? localCursor - lineStart : 0;
        const std::string prefix = text_.substr(lineStart, colBytes);
        const int caretX = contentLeft - (wordWrap_ ? 0 : scrollX()) + measureTextWidth(display, drawable, gc, theme, prefix);
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
    auto cursorFromMouse = [&](int px, int py) -> size_t {
        const auto rect = bounds();
        constexpr int lineHeight = 18;
        const int contentLeft = rect.x + 8;
        const int contentTop = rect.y + 8;
        const int lineIndex = std::max(0, (py - contentTop + scrollY()) / lineHeight);
        const auto lines = splitVisualLines(text_);
        const int clampedLine = std::min(lineIndex, std::max(0, static_cast<int>(lines.size()) - 1));
        const size_t start = lineStartOffset(text_, clampedLine);
        const size_t end = lineEndOffset(text_, start);
        const int localX = px - contentLeft + (wordWrap_ ? 0 : scrollX());
        size_t newCursor = start;
        for (size_t i = start + 1; i <= end; ++i) {
            const std::string prefix = text_.substr(start, i - start);
            if (measureTextWidth(nullptr, 0, 0, Neu_Theme{}, prefix) <= localX) {
                newCursor = i;
            } else {
                break;
            }
        }
        return newCursor;
    };

    if (event.type == ButtonPress && contains(event.xbutton.x, event.xbutton.y)) {
        const size_t newCursor = cursorFromMouse(event.xbutton.x, event.xbutton.y);
        if (!(event.xbutton.state & ShiftMask) && hasSelection() && newCursor >= selectionStart() && newCursor <= selectionEnd()) {
            mouseDraggingSelectedText_ = true;
            mouseSelecting_ = false;
            dragSourceStart_ = selectionStart();
            dragSourceEnd_ = selectionEnd();
            dragText_ = selectedText();
            dragDropCursor_ = newCursor;
            cursor_ = newCursor;
            Neu_Control::handleXEvent(event);
            requestRedraw();
            return;
        }
        mouseDraggingSelectedText_ = false;
        mouseSelecting_ = true;
        mouseSelectAnchor_ = (event.xbutton.state & ShiftMask) ? selectionStart() : newCursor;
        if (event.xbutton.state & ShiftMask) {
            moveCursorWithSelection(newCursor, true);
        } else {
            moveCursorWithSelection(newCursor, false);
            mouseSelectAnchor_ = newCursor;
        }
        Neu_Control::handleXEvent(event);
        return;
    }

    if (event.type == MotionNotify && focused_) {
        const size_t newCursor = cursorFromMouse(event.xmotion.x, event.xmotion.y);
        if (mouseDraggingSelectedText_) {
            dragDropCursor_ = newCursor;
            cursor_ = newCursor;
            requestRedraw();
            return;
        }
        if (mouseSelecting_) {
            selectionStart_ = mouseSelectAnchor_;
            selectionEnd_ = newCursor;
            cursor_ = newCursor;
            requestRedraw();
            return;
        }
    }

    if (event.type == ButtonRelease && mouseDraggingSelectedText_) {
        mouseDraggingSelectedText_ = false;
        if (!dragText_.empty() && dragSourceStart_ < dragSourceEnd_ && dragSourceEnd_ <= text_.size()) {
            size_t drop = std::min(dragDropCursor_, text_.size());
            if (drop < dragSourceStart_ || drop > dragSourceEnd_) {
                pushUndoSnapshot();
                text_.erase(dragSourceStart_, dragSourceEnd_ - dragSourceStart_);
                if (drop > dragSourceEnd_) {
                    drop -= (dragSourceEnd_ - dragSourceStart_);
                }
                drop = std::min(drop, text_.size());
                text_.insert(drop, dragText_);
                cursor_ = drop + dragText_.size();
                selectionStart_ = drop;
                selectionEnd_ = cursor_;
                invokeTextChanged();
            }
        }
        dragText_.clear();
        requestRedraw();
        return;
    }

    if (event.type == ButtonRelease && mouseSelecting_) {
        mouseSelecting_ = false;
        requestRedraw();
        return;
    }

    if (focused_ && event.type == KeyPress) {
        const KeySym key = XLookupKeysym(&event.xkey, 0);
        const bool shift = (event.xkey.state & ShiftMask) != 0;
        const int lineHeight = 18;
        const auto rect = bounds();
        const int pageLines = std::max(1, (rect.height - 16) / lineHeight);

        if (key == XK_Up || key == XK_Down || key == XK_Page_Up || key == XK_Page_Down) {
            int delta = 0;
            if (key == XK_Up) {
                delta = -1;
            } else if (key == XK_Down) {
                delta = 1;
            } else if (key == XK_Page_Up) {
                delta = -pageLines;
                setScrollOffset(scrollX(), scrollY() - pageLines * lineHeight);
            } else {
                delta = pageLines;
                setScrollOffset(scrollX(), scrollY() + pageLines * lineHeight);
            }
            moveCursorWithSelection(moveVerticallyByLines(text_, cursor_, delta), shift);
            return;
        }

        if (key == XK_Home || key == XK_End) {
            const int currentLine = lineIndexForOffset(text_, cursor_);
            const size_t start = lineStartOffset(text_, currentLine);
            const size_t end = lineEndOffset(text_, start);
            moveCursorWithSelection(key == XK_Home ? start : end, shift);
            return;
        }

        if (key == XK_Return) {
            replaceSelectionWith("\n");
            requestRedraw();
            Neu_Control::handleXEvent(event);
            return;
        }
    }

    Neu_Textbox::handleXEvent(event);
}

} // namespace neutrino
