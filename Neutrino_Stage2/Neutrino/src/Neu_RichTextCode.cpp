#include "Neutrino/Neutrino.hpp"
#include <X11/keysym.h>

namespace neutrino {

namespace {

static bool startsWithKeyword(const std::string& line, const std::string& keyword)
{
    size_t first = line.find_first_not_of(" \t");
    return first != std::string::npos && line.compare(first, keyword.size(), keyword) == 0;
}

static int richLineHeight(const Neu_TextFragment& f)
{
    if (f.headingLevel > 0) {
        return std::max(22, 36 - f.headingLevel * 2);
    }
    return 18;
}

static std::vector<std::string> splitPreserveLines(const std::string& text)
{
    std::vector<std::string> lines;
    std::string current;
    for (char ch : text) {
        if (ch == '\r') {
            continue;
        }
        if (ch == '\n') {
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

} // namespace

void Neu_RichTextCode::applyHeading(int level)
{
    Neu_TextFragment style;
    style.headingLevel = std::max(0, std::min(7, level));
    applyFragmentStyleToSelection(style);
}

void Neu_RichTextCode::applyFontColor(const Neu_Color& color)
{
    Neu_TextFragment style;
    style.useFontColor = true;
    style.fontColor = color;
    applyFragmentStyleToSelection(style);
}

void Neu_RichTextCode::applyBackgroundColor(const Neu_Color& color)
{
    Neu_TextFragment style;
    style.useBackgroundColor = true;
    style.backgroundColor = color;
    applyFragmentStyleToSelection(style);
}

void Neu_RichTextCode::applyHighlightColor(const Neu_Color& color)
{
    Neu_TextFragment style;
    style.useHighlightColor = true;
    style.highlightColor = color;
    applyFragmentStyleToSelection(style);
}

void Neu_RichTextCode::applyFragmentStyleToSelection(const Neu_TextFragment& style)
{
    const size_t a = hasSelection() ? selectionStart() : 0;
    const size_t b = hasSelection() ? selectionEnd() : text_.size();
    if (text_.empty() || a >= b || b > text_.size()) {
        return;
    }
    pushUndoSnapshot();
    std::vector<Neu_TextFragment> fragments;
    if (a > 0) {
        Neu_TextFragment before;
        before.text = text_.substr(0, a);
        before.useFontColor = true;
        before.fontColor = defaultFontColor_;
        fragments.push_back(before);
    }
    Neu_TextFragment selected = style;
    selected.text = text_.substr(a, b - a);
    if (!selected.useFontColor) {
        selected.useFontColor = true;
        selected.fontColor = defaultFontColor_;
    }
    if (!selected.fontName.empty()) {
        selected.monospace = selected.fontName.find("Mono") != std::string::npos || selected.fontName.find("mono") != std::string::npos;
    }
    fragments.push_back(selected);
    if (b < text_.size()) {
        Neu_TextFragment after;
        after.text = text_.substr(b);
        after.useFontColor = true;
        after.fontColor = defaultFontColor_;
        fragments.push_back(after);
    }
    richTextFragments_ = fragments;
    requestRedraw();
}

void Neu_RichTextCode::applyToolbarAction(int actionIndex)
{
    Neu_TextFragment style;
    switch (actionIndex) {
    case 0:
        style.bold = true;
        break;
    case 1:
        style.italic = true;
        break;
    case 2:
        style.underline = true;
        break;
    case 3:
        style.strikethrough = true;
        break;
    case 4:
        style.doubleStrikethrough = true;
        break;
    case 5:
        style.headingLevel = 1;
        break;
    case 6:
        style.headingLevel = 2;
        break;
    case 7:
        style.monospace = true;
        style.fontName = "Monospace";
        break;
    case 8:
        toolbarFontIndex_ = (toolbarFontIndex_ + 1) % static_cast<int>(toolbarFonts_.size());
        style.fontName = toolbarFonts_[toolbarFontIndex_];
        style.monospace = style.fontName == "Monospace";
        break;
    case 9:
        style.useFontColor = true;
        style.fontColor = Neu_Color{144, 202, 249, 255};
        break;
    case 10:
        style.useBackgroundColor = true;
        style.backgroundColor = Neu_Color{36, 46, 62, 255};
        break;
    case 11:
        style.useHighlightColor = true;
        style.highlightColor = sketchHighlightColor_;
        break;
    case 12:
        setTextAlignment(Neu_TextAlignment::Left);
        return;
    case 13:
        setTextAlignment(Neu_TextAlignment::Center);
        return;
    case 14:
        setTextAlignment(Neu_TextAlignment::Right);
        return;
    case 15:
        setWordWrap(!wordWrap_);
        return;
    default:
        return;
    }
    applyFragmentStyleToSelection(style);
}

void Neu_RichTextCode::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(display, drawable, gc, theme);
    const auto rect = bounds();
    const int toolbarHeight = toolbarVisible_ ? 34 : 0;
    const int lineHeight = 18;
    const int contentLeft = rect.x + 56;
    const int contentTop = rect.y + toolbarHeight + 10;
    const int contentWidth = std::max(1, rect.width - 74);
    int y = contentTop + 12 - scrollY();
    int maxWidth = rect.width;
    int naturalHeight = toolbarHeight + 20;

    if (toolbarVisible_) {
        XSetForeground(display, gc, Neu_Pixel(display, Neu_Color{224, 232, 244, 255}));
        XFillRectangle(display, drawable, gc, rect.x + 2, rect.y + 2, rect.width - 4, toolbarHeight - 3);
        const char* tools[] = {"𝐁", "𝐼", "U̲", "S̶", "S̶̶", "H₁", "H₂", "⌨", "𝑓", "A", "▣", "▧", "⇤", "↔", "⇥", "↩"};
        int tx = rect.x + 8;
        for (const char* tool : tools) {
            XSetForeground(display, gc, Neu_Pixel(display, theme.border));
            XDrawRectangle(display, drawable, gc, tx, rect.y + 7, 42, 20);
            drawText(display, drawable, gc, theme, tool, tx + 4, rect.y + 22);
            tx += 46;
            if (tx > rect.x + rect.width - 50) {
                break;
            }
        }
    }

    XSetForeground(display, gc, Neu_Pixel(display, Neu_Color{225, 232, 242, 255}));
    XFillRectangle(display, drawable, gc, rect.x + 2, rect.y + toolbarHeight + 2, 46, rect.height - toolbarHeight - 4);
    drawText(display, drawable, gc, theme, languageName_, rect.x + rect.width - 94, rect.y + toolbarHeight + 16);

    XRectangle clip{static_cast<short>(rect.x + 4),
                    static_cast<short>(rect.y + toolbarHeight + 4),
                    static_cast<unsigned short>(std::max(1, rect.width - 18)),
                    static_cast<unsigned short>(std::max(1, rect.height - toolbarHeight - 16))};
    XSetClipRectangles(display, gc, 0, 0, &clip, 1, Unsorted);

    if (!richTextFragments().empty()) {
        size_t fragmentBase = 0;
        for (const auto& f : richTextFragments()) {
            const int fh = richLineHeight(f);
            std::vector<std::string> logicalParts = splitPreserveLines(f.text);
            size_t localOffset = 0;
            for (const auto& logicalPart : logicalParts) {
                const auto parts = wordWrap_ ? wrapTextToWidth(display, drawable, gc, theme, logicalPart, contentWidth)
                                             : std::vector<std::string>{logicalPart};
                size_t visualOffset = 0;
                for (const auto& part : parts) {
                    const size_t lineStartOffset = fragmentBase + localOffset + visualOffset;
                    const size_t lineEndOffset = std::min(text_.size(), lineStartOffset + part.size());
                    const std::string visible = wordWrap_ ? part : truncateTextToWidth(display, drawable, gc, theme, part, contentWidth + scrollX());
                    const int w = measureTextWidth(display, drawable, gc, theme, visible, f.bold, f.italic, f.monospace, f.headingLevel);
                    maxWidth = std::max(maxWidth, w + 90);
                    if (y >= rect.y + toolbarHeight + 14 && y < rect.y + rect.height - 6) {
                        if (f.useBackgroundColor || f.useHighlightColor) {
                            XSetForeground(display, gc, Neu_Pixel(display, f.useHighlightColor ? f.highlightColor : f.backgroundColor));
                            XFillRectangle(display, drawable, gc, contentLeft - scrollX(), y - fh + 5, std::max(1, w), fh);
                        }
                        if (focused_ && hasSelection()) {
                            const size_t selA = selectionStart();
                            const size_t selB = selectionEnd();
                            if (selB > lineStartOffset && selA < lineEndOffset) {
                                const size_t localA = std::max(selA, lineStartOffset) - lineStartOffset;
                                const size_t localB = std::min(selB, lineEndOffset) - lineStartOffset;
                                const int sx = contentLeft - scrollX() + measureTextWidth(display, drawable, gc, theme, part.substr(0, std::min(localA, part.size())), f.bold, f.italic, f.monospace, f.headingLevel);
                                const int ex = contentLeft - scrollX() + measureTextWidth(display, drawable, gc, theme, part.substr(0, std::min(localB, part.size())), f.bold, f.italic, f.monospace, f.headingLevel);
                                XSetForeground(display, gc, Neu_Pixel(display, theme.highlight));
                                XFillRectangle(display, drawable, gc, std::min(sx, ex), y - fh + 5, std::max(1, std::abs(ex - sx)), fh);
                            }
                        }
                        drawTextColored(display,
                                        drawable,
                                        gc,
                                        theme,
                                        visible,
                                        alignedTextX(display, drawable, gc, theme, visible, contentLeft - scrollX(), contentWidth),
                                        y,
                                        f.useFontColor ? f.fontColor : defaultFontColor_,
                                        f.bold,
                                        f.italic,
                                        f.underline,
                                        f.strikethrough,
                                        f.doubleStrikethrough,
                                        f.monospace,
                                        f.headingLevel);
                    }
                    y += fh;
                    naturalHeight += fh;
                    visualOffset += part.size();
                }
                localOffset += logicalPart.size();
                if (localOffset < f.text.size()) {
                    if (f.text[localOffset] == '\r' && localOffset + 1 < f.text.size() && f.text[localOffset + 1] == '\n') {
                        localOffset += 2;
                    } else if (f.text[localOffset] == '\r' || f.text[localOffset] == '\n') {
                        ++localOffset;
                    }
                }
            }
            fragmentBase += f.text.size();
        }
    } else {
        const auto sourceLines = splitPreserveLines(text());
        int lineNo = 1;
        size_t logicalOffset = 0;
        for (const auto& line : sourceLines) {
            const size_t lineStartOffset = logicalOffset;
            const size_t lineEndOffset = std::min(text_.size(), lineStartOffset + line.size());
            auto lines = wordWrap_ ? wrapTextToWidth(display, drawable, gc, theme, line, contentWidth) : std::vector<std::string>{line};
            for (const auto& visualLine : lines) {
                maxWidth = std::max(maxWidth, measureTextWidth(display, drawable, gc, theme, visualLine, false, false, true) + 80);
                if (y >= rect.y + toolbarHeight + 14 && y < rect.y + rect.height - 6) {
                    drawText(display, drawable, gc, theme, std::to_string(lineNo), rect.x + 8, y);
                    if (startsWithKeyword(line, "#include") || startsWithKeyword(line, "class") || startsWithKeyword(line, "int") || startsWithKeyword(line, "void") || startsWithKeyword(line, "auto")) {
                        XSetForeground(display, gc, Neu_Pixel(display, sketchHighlightColor_));
                        XFillRectangle(display, drawable, gc, rect.x + 50, y - 14, rect.width - 64, lineHeight);
                    }
                    if (focused_ && hasSelection()) {
                        const size_t selA = selectionStart();
                        const size_t selB = selectionEnd();
                        if (selB > lineStartOffset && selA < lineEndOffset) {
                            const size_t localA = std::max(selA, lineStartOffset) - lineStartOffset;
                            const size_t localB = std::min(selB, lineEndOffset) - lineStartOffset;
                            const int sx = contentLeft - scrollX() + measureTextWidth(display, drawable, gc, theme, line.substr(0, localA), false, false, true);
                            const int ex = contentLeft - scrollX() + measureTextWidth(display, drawable, gc, theme, line.substr(0, localB), false, false, true);
                            XSetForeground(display, gc, Neu_Pixel(display, theme.highlight));
                            XFillRectangle(display, drawable, gc, std::min(sx, ex), y - lineHeight + 4, std::max(1, std::abs(ex - sx)), lineHeight);
                        }
                    }
                    drawTextColored(display,
                                    drawable,
                                    gc,
                                    theme,
                                    wordWrap_ ? visualLine : truncateTextToWidth(display, drawable, gc, theme, visualLine, contentWidth + scrollX()),
                                    contentLeft - scrollX(),
                                    y,
                                    defaultFontColor_,
                                    false,
                                    false,
                                    false,
                                    false,
                                    false,
                                    true);
                }
                y += lineHeight;
                naturalHeight += lineHeight;
            }
            logicalOffset = lineEndOffset;
            if (logicalOffset < text_.size()) {
                if (text_[logicalOffset] == '\r' && logicalOffset + 1 < text_.size() && text_[logicalOffset + 1] == '\n') {
                    logicalOffset += 2;
                } else if (text_[logicalOffset] == '\r' || text_[logicalOffset] == '\n') {
                    ++logicalOffset;
                }
            }
            ++lineNo;
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
            std::string prefix = text_.substr(lineStart, colBytes);
            if (!prefix.empty() && prefix.back() == '\r') {
                prefix.pop_back();
            }
            const int caretX = contentLeft - scrollX() + measureTextWidth(display, drawable, gc, theme, prefix, false, false, true);
            const int caretY = contentTop + static_cast<int>(lineIndex) * lineHeight - scrollY();
            XSetForeground(display, gc, Neu_Pixel(display, theme.accent));
            XDrawLine(display, drawable, gc, caretX, caretY, caretX, caretY + lineHeight - 3);
        }
    }


    if (!richTextFragments().empty() && focused_) {
        size_t localCursor = std::min(cursor_, text_.size());
        size_t lineIndex = 0;
        size_t lineStart = 0;
        for (size_t i = 0; i < localCursor; ++i) {
            if (text_[i] == '\r') {
                if (i + 1 < localCursor && text_[i + 1] == '\n') {
                    ++i;
                }
                ++lineIndex;
                lineStart = i + 1;
            } else if (text_[i] == '\n') {
                ++lineIndex;
                lineStart = i + 1;
            }
        }
        const size_t colBytes = localCursor >= lineStart ? localCursor - lineStart : 0;
        std::string prefix = text_.substr(lineStart, colBytes);
        while (!prefix.empty() && (prefix.back() == '\r' || prefix.back() == '\n')) {
            prefix.pop_back();
        }
        const int caretX = contentLeft - scrollX() + measureTextWidth(display, drawable, gc, theme, prefix, false, false, true);
        const int caretY = contentTop + static_cast<int>(lineIndex) * lineHeight - scrollY();
        XSetForeground(display, gc, Neu_Pixel(display, theme.accent));
        XDrawLine(display, drawable, gc, caretX, caretY, caretX, caretY + lineHeight - 3);
    }
    XSetClipMask(display, gc, None);
    setAutoScroll(true);
    setVirtualSize(std::max(rect.width, maxWidth), std::max(rect.height, naturalHeight + 20));
    drawScrollbars(display, drawable, gc, theme);
    drawHintPopup(display, drawable, gc, theme);
}

void Neu_RichTextCode::handleXEvent(XEvent& event)
{
    if (readOnly_ && event.type == KeyPress) {
        Neu_Control::handleXEvent(event);
        return;
    }

    auto cursorFromRichPoint = [&](int px, int py) -> size_t {
        const auto rect = bounds();
        const int toolbarHeight = toolbarVisible_ ? 34 : 0;
        constexpr int lineHeight = 18;
        const int contentLeft = rect.x + 56;
        const int contentTop = rect.y + toolbarHeight + 10;
        const int lineIndex = std::max(0, (py - contentTop + scrollY()) / lineHeight);
        const auto lines = splitPreserveLines(text_);
        const int clampedLine = std::min(lineIndex, std::max(0, static_cast<int>(lines.size()) - 1));
        size_t start = 0;
        for (int i = 0; i < clampedLine && start < text_.size(); ++i) {
            while (start < text_.size() && text_[start] != '\n' && text_[start] != '\r') {
                ++start;
            }
            if (start < text_.size() && text_[start] == '\r' && start + 1 < text_.size() && text_[start + 1] == '\n') {
                start += 2;
            } else if (start < text_.size()) {
                ++start;
            }
        }
        size_t end = start;
        while (end < text_.size() && text_[end] != '\n' && text_[end] != '\r') {
            ++end;
        }
        const int localX = px - contentLeft + (wordWrap_ ? 0 : scrollX());
        size_t newCursor = start;
        for (size_t i = start + 1; i <= end; ++i) {
            const std::string prefix = text_.substr(start, i - start);
            if (measureTextWidth(nullptr, 0, 0, Neu_Theme{}, prefix, false, false, true) <= localX) {
                newCursor = i;
            } else {
                break;
            }
        }
        return newCursor;
    };

    if (!readOnly_ && event.type == ButtonPress && contains(event.xbutton.x, event.xbutton.y)) {
        const auto rect = bounds();
        const int toolbarHeight = toolbarVisible_ ? 34 : 0;
        if (toolbarVisible_ && event.xbutton.y >= rect.y + 7 && event.xbutton.y <= rect.y + 27) {
            int tx = rect.x + 8;
            for (int i = 0; i < 16; ++i) {
                if (event.xbutton.x >= tx && event.xbutton.x <= tx + 42) {
                    applyToolbarAction(i);
                    return;
                }
                tx += 46;
                if (tx > rect.x + rect.width - 50) {
                    break;
                }
            }
        }
        const int contentTop = rect.y + toolbarHeight + 10;
        if (event.xbutton.y >= contentTop) {
            const size_t newCursor = cursorFromRichPoint(event.xbutton.x, event.xbutton.y);
            if (!(event.xbutton.state & ShiftMask) && hasSelection() && newCursor >= selectionStart() && newCursor <= selectionEnd()) {
                mouseDraggingSelectedText_ = true;
                mouseSelecting_ = false;
                dragSourceStart_ = selectionStart();
                dragSourceEnd_ = selectionEnd();
                dragText_ = selectedText();
                dragDropCursor_ = newCursor;
                cursor_ = newCursor;
                requestRedraw();
                return;
            }
            mouseDraggingSelectedText_ = false;
            mouseSelecting_ = true;
            mouseSelectAnchor_ = (event.xbutton.state & ShiftMask) ? selectionStart() : newCursor;
            moveCursorWithSelection(newCursor, (event.xbutton.state & ShiftMask) != 0);
            if (!(event.xbutton.state & ShiftMask)) {
                mouseSelectAnchor_ = newCursor;
            }
            Neu_Control::handleXEvent(event);
            return;
        }
    }

    if (!readOnly_ && event.type == MotionNotify && focused_) {
        const size_t newCursor = cursorFromRichPoint(event.xmotion.x, event.xmotion.y);
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

    if (!readOnly_ && event.type == ButtonRelease && mouseDraggingSelectedText_) {
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

    if (!readOnly_ && event.type == ButtonRelease && mouseSelecting_) {
        mouseSelecting_ = false;
        requestRedraw();
        return;
    }

    if (focused_ && event.type == KeyPress) {
        KeySym sym = XLookupKeysym(&event.xkey, 0);
        if (sym == XK_BackSpace) {
            if (hasSelection()) {
                deleteSelection();
                requestRedraw();
            } else if (cursor_ > 0 && !text_.empty()) {
                pushUndoSnapshot();
                cursor_ = std::min(cursor_, text_.size());
                size_t eraseAt = cursor_ - 1;
                size_t eraseCount = 1;
                if (cursor_ >= 2 && text_[cursor_ - 2] == '\r' && text_[cursor_ - 1] == '\n') {
                    eraseAt = cursor_ - 2;
                    eraseCount = 2;
                }
                text_.erase(eraseAt, eraseCount);
                cursor_ = eraseAt;
                invokeTextChanged();
                size_t lineStart = 0;
                const size_t searchFrom = cursor_ == 0 ? 0 : cursor_ - 1;
                const size_t prevLf = text_.rfind('\n', searchFrom);
                if (prevLf != std::string::npos && prevLf < cursor_) {
                    lineStart = prevLf + 1;
                }
                const size_t nextLf = text_.find('\n', lineStart);
                const size_t lineEnd = nextLf == std::string::npos ? text_.size() : nextLf;
                if (cursor_ > lineEnd) {
                    cursor_ = lineEnd;
                }
                std::string prefix = text_.substr(lineStart, cursor_ - lineStart);
                while (!prefix.empty() && (prefix.back() == '\r' || prefix.back() == '\n')) {
                    prefix.pop_back();
                }
                const auto rect = bounds();
                const int contentWidth = std::max(1, rect.width - 74);
                const int prefixWidth = measureTextWidth(nullptr, 0, 0, Neu_Theme{}, prefix, false, false, true);
                if (prefixWidth <= contentWidth - 12) {
                    scrollX_ = 0;
                } else if (prefixWidth - scrollX_ > contentWidth - 8) {
                    scrollX_ = std::max(0, prefixWidth - contentWidth + 8);
                } else if (prefixWidth < scrollX_ + 4) {
                    scrollX_ = std::max(0, prefixWidth - 8);
                }
                requestRedraw();
            }
            Neu_Control::handleXEvent(event);
            return;
        }
    }

    Neu_Multilinetextbox::handleXEvent(event);
}

} // namespace neutrino
