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
        const char* tools[] = {"B", "I", "U", "S", "DS", "H1", "H2", "Mono", "Font", "Text", "BG", "HL", "Left", "Center", "Right", "Wrap"};
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
        for (const auto& f : richTextFragments()) {
            const int fh = richLineHeight(f);
            std::vector<std::string> logicalParts = splitPreserveLines(f.text);
            for (const auto& logicalPart : logicalParts) {
                const auto parts = wordWrap_ ? wrapTextToWidth(display, drawable, gc, theme, logicalPart, contentWidth)
                                             : std::vector<std::string>{logicalPart};
                for (const auto& part : parts) {
                    const std::string visible = wordWrap_ ? part : truncateTextToWidth(display, drawable, gc, theme, part, contentWidth + scrollX());
                    const int w = measureTextWidth(display, drawable, gc, theme, visible, f.bold, f.italic, f.monospace, f.headingLevel);
                    maxWidth = std::max(maxWidth, w + 90);
                    if (y >= rect.y + toolbarHeight + 14 && y < rect.y + rect.height - 6) {
                        if (f.useBackgroundColor || f.useHighlightColor) {
                            XSetForeground(display, gc, Neu_Pixel(display, f.useHighlightColor ? f.highlightColor : f.backgroundColor));
                            XFillRectangle(display, drawable, gc, contentLeft - scrollX(), y - fh + 5, std::max(1, w), fh);
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
                }
            }
        }
    } else {
        const auto sourceLines = splitPreserveLines(text());
        int lineNo = 1;
        for (const auto& line : sourceLines) {
            auto lines = wordWrap_ ? wrapTextToWidth(display, drawable, gc, theme, line, contentWidth) : std::vector<std::string>{line};
            for (const auto& visualLine : lines) {
                maxWidth = std::max(maxWidth, measureTextWidth(display, drawable, gc, theme, visualLine, false, false, true) + 80);
                if (y >= rect.y + toolbarHeight + 14 && y < rect.y + rect.height - 6) {
                    drawText(display, drawable, gc, theme, std::to_string(lineNo), rect.x + 8, y);
                    if (startsWithKeyword(line, "#include") || startsWithKeyword(line, "class") || startsWithKeyword(line, "int") || startsWithKeyword(line, "void") || startsWithKeyword(line, "auto")) {
                        XSetForeground(display, gc, Neu_Pixel(display, sketchHighlightColor_));
                        XFillRectangle(display, drawable, gc, rect.x + 50, y - 14, rect.width - 64, lineHeight);
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
    Neu_Multilinetextbox::handleXEvent(event);
}

} // namespace neutrino
