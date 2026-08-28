#include "Neutrino/Neutrino.hpp"
#include <X11/keysym.h>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cmath>

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

static bool richWordChar(unsigned char ch)
{
    return std::isalnum(ch) != 0 || ch == '_';
}

static std::pair<size_t, size_t> richToolbarTargetRange(const std::string& text,
                                                        size_t cursor,
                                                        bool hasSelection,
                                                        size_t selectionStart,
                                                        size_t selectionEnd)
{
    if (hasSelection && selectionStart != selectionEnd) {
        const size_t a = std::min(selectionStart, selectionEnd);
        const size_t b = std::max(selectionStart, selectionEnd);
        return {std::min(a, text.size()), std::min(b, text.size())};
    }

    if (text.empty()) {
        return {0, 0};
    }

    size_t pos = std::min(cursor, text.size());
    if (pos == text.size() || !richWordChar(static_cast<unsigned char>(text[pos]))) {
        if (pos > 0 && richWordChar(static_cast<unsigned char>(text[pos - 1]))) {
            --pos;
        }
    }

    if (pos >= text.size() || !richWordChar(static_cast<unsigned char>(text[pos]))) {
        return {cursor, cursor};
    }

    size_t a = pos;
    while (a > 0 && richWordChar(static_cast<unsigned char>(text[a - 1]))) {
        --a;
    }
    size_t b = pos;
    while (b < text.size() && richWordChar(static_cast<unsigned char>(text[b]))) {
        ++b;
    }
    return {a, b};
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

static bool styleEquivalent(const Neu_TextFragment& a, const Neu_TextFragment& b)
{
    return a.bold == b.bold &&
           a.italic == b.italic &&
           a.underline == b.underline &&
           a.strikethrough == b.strikethrough &&
           a.doubleStrikethrough == b.doubleStrikethrough &&
           a.monospace == b.monospace &&
           a.headingLevel == b.headingLevel &&
           a.fontName == b.fontName &&
           a.useFontColor == b.useFontColor &&
           (!a.useFontColor || (a.fontColor.r == b.fontColor.r && a.fontColor.g == b.fontColor.g && a.fontColor.b == b.fontColor.b && a.fontColor.a == b.fontColor.a)) &&
           a.useBackgroundColor == b.useBackgroundColor &&
           (!a.useBackgroundColor || (a.backgroundColor.r == b.backgroundColor.r && a.backgroundColor.g == b.backgroundColor.g && a.backgroundColor.b == b.backgroundColor.b && a.backgroundColor.a == b.backgroundColor.a)) &&
           a.useHighlightColor == b.useHighlightColor &&
           (!a.useHighlightColor || (a.highlightColor.r == b.highlightColor.r && a.highlightColor.g == b.highlightColor.g && a.highlightColor.b == b.highlightColor.b && a.highlightColor.a == b.highlightColor.a));
}

static Neu_TextFragment defaultTextFragment(const std::string& text, const Neu_Color& defaultColor)
{
    Neu_TextFragment f;
    f.text = text;
    f.useFontColor = true;
    f.fontColor = defaultColor;
    return f;
}

static std::vector<Neu_TextFragment> normalizedFragments(const std::string& text,
                                                         const std::vector<Neu_TextFragment>& fragments,
                                                         const Neu_Color& defaultColor)
{
    if (fragments.empty()) {
        return {defaultTextFragment(text, defaultColor)};
    }
    size_t total = 0;
    for (const auto& f : fragments) {
        total += f.text.size();
    }
    if (total != text.size()) {
        return {defaultTextFragment(text, defaultColor)};
    }
    return fragments;
}

static void appendMergedFragment(std::vector<Neu_TextFragment>& out, const Neu_TextFragment& fragment)
{
    if (fragment.text.empty()) {
        return;
    }
    if (!out.empty() && styleEquivalent(out.back(), fragment)) {
        out.back().text += fragment.text;
    } else {
        out.push_back(fragment);
    }
}

static bool sameColor(const Neu_Color& a, const Neu_Color& b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

static Neu_TextFragment toggledStyle(Neu_TextFragment current, const Neu_TextFragment& requested)
{
    if (requested.bold) {
        current.bold = !current.bold;
    }
    if (requested.italic) {
        current.italic = !current.italic;
    }
    if (requested.underline) {
        current.underline = !current.underline;
    }
    if (requested.strikethrough) {
        current.strikethrough = !current.strikethrough;
    }
    if (requested.doubleStrikethrough) {
        current.doubleStrikethrough = !current.doubleStrikethrough;
    }
    if (requested.monospace || !requested.fontName.empty()) {
        const bool sameFont = current.fontName == requested.fontName && current.monospace == requested.monospace;
        if (sameFont) {
            current.fontName.clear();
            current.monospace = false;
        } else {
            current.fontName = requested.fontName;
            current.monospace = requested.monospace;
        }
    }
    if (requested.headingLevel > 0) {
        current.headingLevel = current.headingLevel == requested.headingLevel ? 0 : requested.headingLevel;
    }
    if (requested.useFontColor) {
        if (current.useFontColor && sameColor(current.fontColor, requested.fontColor)) {
            current.useFontColor = false;
        } else {
            current.useFontColor = true;
            current.fontColor = requested.fontColor;
        }
    }
    if (requested.useBackgroundColor) {
        if (current.useBackgroundColor && sameColor(current.backgroundColor, requested.backgroundColor)) {
            current.useBackgroundColor = false;
        } else {
            current.useBackgroundColor = true;
            current.backgroundColor = requested.backgroundColor;
        }
    }
    if (requested.useHighlightColor) {
        if (current.useHighlightColor && sameColor(current.highlightColor, requested.highlightColor)) {
            current.useHighlightColor = false;
        } else {
            current.useHighlightColor = true;
            current.highlightColor = requested.highlightColor;
        }
    }
    return current;
}

struct StyledRun {
    size_t start{0};
    size_t end{0};
    std::string text;
    Neu_TextFragment style;
};

struct StyledLine {
    std::vector<StyledRun> runs;
    size_t start{0};
    size_t end{0};
    int height{18};
};

static std::vector<StyledLine> buildStyledLines(const std::string& text,
                                                const std::vector<Neu_TextFragment>& fragments,
                                                const Neu_Color& defaultColor)
{
    std::vector<StyledLine> lines;
    StyledLine current;
    current.start = 0;
    current.end = 0;
    current.height = 18;

    size_t global = 0;
    const auto normalized = normalizedFragments(text, fragments, defaultColor);
    for (auto fragment : normalized) {
        if (!fragment.useFontColor) {
            fragment.useFontColor = true;
            fragment.fontColor = defaultColor;
        }
        size_t segmentStart = 0;
        for (size_t i = 0; i < fragment.text.size(); ++i) {
            const char ch = fragment.text[i];
            const bool isNewline = ch == '\n' || ch == '\r';
            if (!isNewline) {
                continue;
            }
            if (i > segmentStart) {
                Neu_TextFragment styled = fragment;
                styled.text = fragment.text.substr(segmentStart, i - segmentStart);
                StyledRun run;
                run.start = global + segmentStart;
                run.end = global + i;
                run.text = styled.text;
                run.style = styled;
                current.runs.push_back(run);
                current.end = run.end;
                current.height = std::max(current.height, richLineHeight(styled));
            }
            lines.push_back(current);
            current = StyledLine{};
            size_t newlineAdvance = 1;
            if (ch == '\r' && i + 1 < fragment.text.size() && fragment.text[i + 1] == '\n') {
                newlineAdvance = 2;
                ++i;
            }
            current.start = global + i + 1;
            current.end = current.start;
            current.height = 18;
            segmentStart = i + 1;
            (void)newlineAdvance;
        }
        if (segmentStart < fragment.text.size()) {
            Neu_TextFragment styled = fragment;
            styled.text = fragment.text.substr(segmentStart);
            StyledRun run;
            run.start = global + segmentStart;
            run.end = global + fragment.text.size();
            run.text = styled.text;
            run.style = styled;
            current.runs.push_back(run);
            current.end = run.end;
            current.height = std::max(current.height, richLineHeight(styled));
        }
        global += fragment.text.size();
    }
    lines.push_back(current);
    if (lines.empty()) {
        lines.push_back(StyledLine{});
    }
    return lines;
}

static int runTextWidth(Display* display,
                        Drawable,
                        GC gc,
                        const Neu_Theme&,
                        const StyledRun& run,
                        const std::string& text)
{
    int base = run.style.monospace ? 8 : 7;
    if (run.style.headingLevel > 0) {
        base += std::max(1, 8 - run.style.headingLevel);
    }
    if (run.style.bold) {
        base += 1;
    }
    int width = static_cast<int>(text.size()) * base;
    if (display && gc) {
        XFontStruct* font = XQueryFont(display, XGContextFromGC(gc));
        if (font) {
            width = XTextWidth(font, text.c_str(), static_cast<int>(text.size()));
            XFreeFontInfo(nullptr, font, 1);
        }
    }
    return std::max(0, width);
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
    const auto target = richToolbarTargetRange(text_, cursor_, hasSelection(), selectionStart_, selectionEnd_);
    const size_t a = target.first;
    const size_t b = target.second;
    if (text_.empty() || a >= b || b > text_.size()) {
        return;
    }

    pushUndoSnapshot();
    std::vector<Neu_TextFragment> result;
    size_t base = 0;
    const auto fragments = normalizedFragments(text_, richTextFragments_, defaultFontColor_);

    for (auto fragment : fragments) {
        if (!fragment.useFontColor) {
            fragment.useFontColor = true;
            fragment.fontColor = defaultFontColor_;
        }
        const size_t fragStart = base;
        const size_t fragEnd = base + fragment.text.size();
        if (fragEnd <= a || fragStart >= b) {
            appendMergedFragment(result, fragment);
            base = fragEnd;
            continue;
        }

        const size_t localA = a > fragStart ? a - fragStart : 0;
        const size_t localB = std::min(b, fragEnd) - fragStart;
        if (localA > 0) {
            Neu_TextFragment before = fragment;
            before.text = fragment.text.substr(0, localA);
            appendMergedFragment(result, before);
        }
        if (localB > localA) {
            Neu_TextFragment middle = toggledStyle(fragment, style);
            middle.text = fragment.text.substr(localA, localB - localA);
            appendMergedFragment(result, middle);
        }
        if (localB < fragment.text.size()) {
            Neu_TextFragment after = fragment;
            after.text = fragment.text.substr(localB);
            appendMergedFragment(result, after);
        }
        base = fragEnd;
    }

    richTextFragments_ = result;
    clearSelection();
    cursor_ = b;
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
    const int contentLeft = rect.x + 56;
    const int contentTop = rect.y + toolbarHeight + 10;
    const int contentWidth = std::max(1, rect.width - 74);
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
            drawTextColored(display,
                            drawable,
                            gc,
                            theme,
                            tool,
                            tx + 4,
                            rect.y + 22,
                            Neu_Color{0, 0, 0, 255},
                            false,
                            false,
                            false,
                            false,
                            false,
                            false);
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

    int yTop = contentTop - scrollY();
    const size_t selA = selectionStart();
    const size_t selB = selectionEnd();

    if (!richTextFragments().empty()) {
        const auto lines = buildStyledLines(text_, richTextFragments_, defaultFontColor_);
        for (const auto& line : lines) {
            const int lineHeight = std::max(18, line.height);
            const int baseline = yTop + lineHeight - 5;
            int x = contentLeft - scrollX();
            if (baseline >= rect.y + toolbarHeight + 10 && yTop < rect.y + rect.height - 6) {
                for (const auto& run : line.runs) {
                    const int w = runTextWidth(display, drawable, gc, theme, run, run.text);
                    maxWidth = std::max(maxWidth, x - contentLeft + scrollX() + w + 90);
                    if (run.style.useBackgroundColor || run.style.useHighlightColor) {
                        XSetForeground(display, gc, Neu_Pixel(display, run.style.useHighlightColor ? run.style.highlightColor : run.style.backgroundColor));
                        XFillRectangle(display, drawable, gc, x, yTop + 1, std::max(1, w), std::max(1, lineHeight - 2));
                    }
                    if (focused_ && hasSelection() && selB > run.start && selA < run.end) {
                        const size_t localA = std::max(selA, run.start) - run.start;
                        const size_t localB = std::min(selB, run.end) - run.start;
                        const int sx = x + runTextWidth(display, drawable, gc, theme, run, run.text.substr(0, std::min(localA, run.text.size())));
                        const int ex = x + runTextWidth(display, drawable, gc, theme, run, run.text.substr(0, std::min(localB, run.text.size())));
                        XSetForeground(display, gc, Neu_Pixel(display, theme.highlight));
                        XFillRectangle(display, drawable, gc, std::min(sx, ex), yTop + 1, std::max(1, std::abs(ex - sx)), std::max(1, lineHeight - 2));
                    }
                    drawTextColored(display,
                                    drawable,
                                    gc,
                                    theme,
                                    run.text,
                                    x,
                                    baseline,
                                    run.style.useFontColor ? run.style.fontColor : defaultFontColor_,
                                    run.style.bold,
                                    run.style.italic,
                                    run.style.underline,
                                    run.style.strikethrough,
                                    run.style.doubleStrikethrough,
                                    run.style.monospace,
                                    run.style.headingLevel);
                    x += w;
                }
            }
            yTop += lineHeight;
            naturalHeight += lineHeight;
        }

        if (focused_) {
            int caretY = contentTop - scrollY();
            int caretX = contentLeft - scrollX();
            const size_t c = std::min(cursor_, text_.size());
            for (const auto& line : lines) {
                const int lineHeight = std::max(18, line.height);
                if (c >= line.start && c <= line.end) {
                    int x = contentLeft - scrollX();
                    for (const auto& run : line.runs) {
                        if (c >= run.start && c <= run.end) {
                            caretX = x + runTextWidth(display, drawable, gc, theme, run, run.text.substr(0, c - run.start));
                            break;
                        }
                        x += runTextWidth(display, drawable, gc, theme, run, run.text);
                        caretX = x;
                    }
                    XSetForeground(display, gc, Neu_Pixel(display, theme.accent));
                    XDrawLine(display, drawable, gc, caretX, caretY + 2, caretX, caretY + lineHeight - 3);
                    break;
                }
                caretY += lineHeight;
            }
        }
    } else {
        const int lineHeight = 18;
        const auto sourceLines = splitPreserveLines(text());
        int lineNo = 1;
        size_t logicalOffset = 0;
        int y = contentTop + 12 - scrollY();
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
                        if (selB > lineStartOffset && selA < lineEndOffset) {
                            const size_t localA = std::max(selA, lineStartOffset) - lineStartOffset;
                            const size_t localB = std::min(selB, lineEndOffset) - lineStartOffset;
                            const int sx = contentLeft - scrollX() + measureTextWidth(display, drawable, gc, theme, line.substr(0, localA), false, false, true);
                            const int ex = contentLeft - scrollX() + measureTextWidth(display, drawable, gc, theme, line.substr(0, localB), false, false, true);
                            XSetForeground(display, gc, Neu_Pixel(display, theme.highlight));
                            XFillRectangle(display, drawable, gc, std::min(sx, ex), y - lineHeight + 6, std::max(1, std::abs(ex - sx)), std::max(1, lineHeight - 2));
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
        const int contentLeft = rect.x + 56;
        const int contentTop = rect.y + toolbarHeight + 10;
        const int localY = py - contentTop + scrollY();
        const auto styledLines = buildStyledLines(text_, richTextFragments_, defaultFontColor_);
        int accumulated = 0;
        for (const auto& line : styledLines) {
            const int lineHeight = std::max(18, line.height);
            if (localY < accumulated + lineHeight) {
                const int localX = px - contentLeft + (wordWrap_ ? 0 : scrollX());
                int x = 0;
                for (const auto& run : line.runs) {
                    const int w = runTextWidth(nullptr, 0, 0, Neu_Theme{}, run, run.text);
                    if (localX <= x + w) {
                        size_t cursor = run.start;
                        for (size_t i = 1; i <= run.text.size(); ++i) {
                            const int prefixWidth = runTextWidth(nullptr, 0, 0, Neu_Theme{}, run, run.text.substr(0, i));
                            if (x + prefixWidth <= localX) {
                                cursor = run.start + i;
                            } else {
                                break;
                            }
                        }
                        return cursor;
                    }
                    x += w;
                }
                return line.end;
            }
            accumulated += lineHeight;
        }
        return text_.size();
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
                richTextFragments_.clear();
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
