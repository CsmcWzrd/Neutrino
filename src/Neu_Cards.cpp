#include "Neutrino/Neutrino.hpp"

#include <algorithm>
#include <climits>
#include <sstream>
#include <utility>

namespace neutrino {

namespace {

static std::string utf8FromWideString(const std::wstring& value)
{
    std::string out;
    if (value.empty()) {
        return out;
    }

    auto appendCodePoint = [&](uint32_t cp) {
        if (cp <= 0x7fU) {
            out.push_back(static_cast<char>(cp));
        } else if (cp <= 0x7ffU) {
            out.push_back(static_cast<char>(0xc0U | ((cp >> 6) & 0x1fU)));
            out.push_back(static_cast<char>(0x80U | (cp & 0x3fU)));
        } else if (cp <= 0xffffU) {
            out.push_back(static_cast<char>(0xe0U | ((cp >> 12) & 0x0fU)));
            out.push_back(static_cast<char>(0x80U | ((cp >> 6) & 0x3fU)));
            out.push_back(static_cast<char>(0x80U | (cp & 0x3fU)));
        } else if (cp <= 0x10ffffU) {
            out.push_back(static_cast<char>(0xf0U | ((cp >> 18) & 0x07U)));
            out.push_back(static_cast<char>(0x80U | ((cp >> 12) & 0x3fU)));
            out.push_back(static_cast<char>(0x80U | ((cp >> 6) & 0x3fU)));
            out.push_back(static_cast<char>(0x80U | (cp & 0x3fU)));
        } else {
            out.push_back('?');
        }
    };

    out.reserve(value.size() * 3);
    for (size_t i = 0; i < value.size(); ++i) {
        uint32_t cp = static_cast<uint32_t>(value[i]);
#if WCHAR_MAX <= 0xffff
        if (cp >= 0xd800U && cp <= 0xdbffU && i + 1 < value.size()) {
            const uint32_t low = static_cast<uint32_t>(value[i + 1]);
            if (low >= 0xdc00U && low <= 0xdfffU) {
                cp = 0x10000U + (((cp - 0xd800U) << 10) | (low - 0xdc00U));
                ++i;
            }
        }
#endif
        appendCodePoint(cp);
    }
    return out;
}

static std::wstring wideFromUtf8String(const std::string& value)
{
    std::wstring out;
    if (value.empty()) {
        return out;
    }

    auto appendCodePoint = [&](uint32_t cp) {
#if WCHAR_MAX <= 0xffff
        if (cp > 0xffffU) {
            cp -= 0x10000U;
            out.push_back(static_cast<wchar_t>(0xd800U + ((cp >> 10) & 0x3ffU)));
            out.push_back(static_cast<wchar_t>(0xdc00U + (cp & 0x3ffU)));
            return;
        }
#endif
        out.push_back(static_cast<wchar_t>(cp));
    };

    out.reserve(value.size());
    for (size_t i = 0; i < value.size();) {
        const unsigned char c0 = static_cast<unsigned char>(value[i]);
        uint32_t cp = 0;
        size_t needed = 0;
        if (c0 < 0x80U) {
            cp = c0;
            needed = 1;
        } else if ((c0 & 0xe0U) == 0xc0U) {
            cp = c0 & 0x1fU;
            needed = 2;
        } else if ((c0 & 0xf0U) == 0xe0U) {
            cp = c0 & 0x0fU;
            needed = 3;
        } else if ((c0 & 0xf8U) == 0xf0U) {
            cp = c0 & 0x07U;
            needed = 4;
        } else {
            appendCodePoint('?');
            ++i;
            continue;
        }

        if (i + needed > value.size()) {
            appendCodePoint('?');
            break;
        }
        bool valid = true;
        for (size_t j = 1; j < needed; ++j) {
            const unsigned char cx = static_cast<unsigned char>(value[i + j]);
            if ((cx & 0xc0U) != 0x80U) {
                valid = false;
                break;
            }
            cp = (cp << 6) | (cx & 0x3fU);
        }
        if (!valid || cp > 0x10ffffU || (cp >= 0xd800U && cp <= 0xdfffU)) {
            appendCodePoint('?');
            ++i;
            continue;
        }
        appendCodePoint(cp);
        i += needed;
    }
    return out;
}

static bool startsWithAt(const std::string& text, size_t pos, const char* marker)
{
    const size_t len = std::char_traits<char>::length(marker);
    return pos + len <= text.size() && text.compare(pos, len, marker) == 0;
}

static std::vector<Neu_TextFragment> parseSimpleRichText(const std::string& source, const Neu_Color& defaultColor)
{
    std::vector<Neu_TextFragment> out;
    Neu_TextFragment current;
    current.useFontColor = true;
    current.fontColor = defaultColor;

    auto flush = [&]() {
        if (!current.text.empty()) {
            out.push_back(current);
            current.text.clear();
        }
    };

    for (size_t i = 0; i < source.size();) {
        if (startsWithAt(source, i, "\\*")) {
            current.text.push_back('*');
            i += 2;
            continue;
        }
        if (startsWithAt(source, i, "\\_")) {
            current.text.push_back('_');
            i += 2;
            continue;
        }
        if (startsWithAt(source, i, "\\~")) {
            current.text.push_back('~');
            i += 2;
            continue;
        }
        if (startsWithAt(source, i, "\\`")) {
            current.text.push_back('`');
            i += 2;
            continue;
        }
        if (startsWithAt(source, i, "**")) {
            flush();
            current.bold = !current.bold;
            i += 2;
            continue;
        }
        if (startsWithAt(source, i, "//")) {
            flush();
            current.italic = !current.italic;
            i += 2;
            continue;
        }
        if (startsWithAt(source, i, "__")) {
            flush();
            current.underline = !current.underline;
            i += 2;
            continue;
        }
        if (startsWithAt(source, i, "~~")) {
            flush();
            current.strikethrough = !current.strikethrough;
            i += 2;
            continue;
        }
        if (source[i] == '`') {
            flush();
            current.monospace = !current.monospace;
            ++i;
            continue;
        }
        current.text.push_back(source[i]);
        ++i;
    }
    flush();

    if (out.empty()) {
        Neu_TextFragment plain;
        plain.text = source;
        plain.useFontColor = true;
        plain.fontColor = defaultColor;
        out.push_back(plain);
    }
    return out;
}

static Neu_Color cardFill(const Neu_Theme& theme, bool selected, bool hovered)
{
    if (selected) {
        return theme.pressed;
    }
    if (hovered) {
        return theme.hover;
    }
    return Neu_MixColor(theme.glass, theme.background, 0.18);
}

static void drawCardRect(Display* display,
                         Drawable drawable,
                         GC gc,
                         const Neu_Theme& theme,
                         int x,
                         int y,
                         int w,
                         int h,
                         bool selected,
                         bool hovered)
{
#ifdef _WIN32
    HDC hdc = drawable;
    HBRUSH fill = CreateSolidBrush(RGB(cardFill(theme, selected, hovered).r,
                                       cardFill(theme, selected, hovered).g,
                                       cardFill(theme, selected, hovered).b));
    HPEN pen = CreatePen(PS_SOLID, selected ? 2 : 1, RGB((selected ? theme.focus : theme.border).r,
                                                        (selected ? theme.focus : theme.border).g,
                                                        (selected ? theme.focus : theme.border).b));
    HGDIOBJ oldBrush = SelectObject(hdc, fill);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    RoundRect(hdc, x, y, x + w, y + h, theme.radius, theme.radius);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(pen);
    DeleteObject(fill);
    (void)display;
    (void)gc;
#else
    XSetForeground(display, gc, Neu_Pixel(display, cardFill(theme, selected, hovered)));
    Neu_DrawRoundedRect(display, drawable, gc, x, y, w, h, std::max(4, theme.radius), true);
    XSetForeground(display, gc, Neu_Pixel(display, selected ? theme.focus : theme.border));
    Neu_DrawRoundedRect(display, drawable, gc, x, y, w, h, std::max(4, theme.radius), false);
#endif
}

static int saveClip(Display* display, Drawable drawable, GC gc, int x, int y, int w, int h)
{
#ifdef _WIN32
    (void)display;
    (void)gc;
    HDC hdc = drawable;
    const int saved = SaveDC(hdc);
    IntersectClipRect(hdc, x, y, x + w, y + h);
    return saved;
#else
    XRectangle clip{static_cast<short>(x),
                    static_cast<short>(y),
                    static_cast<unsigned short>(std::max(1, w)),
                    static_cast<unsigned short>(std::max(1, h))};
    XSetClipRectangles(display, gc, 0, 0, &clip, 1, Unsorted);
    (void)drawable;
    return 0;
#endif
}

static void restoreClip(Display* display, Drawable drawable, GC gc, int saved)
{
#ifdef _WIN32
    RestoreDC(drawable, saved);
    (void)display;
    (void)gc;
#else
    XSetClipMask(display, gc, None);
    (void)drawable;
    (void)saved;
#endif
}

static void drawCachedIcon(Display* display,
                           Drawable drawable,
                           GC gc,
                           const Neu_IconBmp& icon,
                           int x,
                           int y,
                           int maxSize)
{
    if (icon.pixels().empty() || icon.width() <= 0 || icon.height() <= 0 || maxSize <= 0) {
        return;
    }
    const int drawWidth = std::min(icon.width(), maxSize);
    const int drawHeight = std::min(icon.height(), maxSize);
    for (int iy = 0; iy < drawHeight; ++iy) {
        for (int ix = 0; ix < drawWidth; ++ix) {
            const uint32_t pixel = icon.pixels()[static_cast<size_t>(iy * icon.width() + ix)];
            const unsigned char alpha = static_cast<unsigned char>((pixel >> 24) & 0xffU);
            if (alpha < 8) {
                continue;
            }
            const uint8_t r = static_cast<uint8_t>((pixel >> 16) & 0xffU);
            const uint8_t g = static_cast<uint8_t>((pixel >> 8) & 0xffU);
            const uint8_t b = static_cast<uint8_t>(pixel & 0xffU);
#ifdef _WIN32
            SetPixel(drawable, x + ix, y + iy, RGB(r, g, b));
            (void)display;
            (void)gc;
#else
            XSetForeground(display, gc, Neu_Pixel(display, Neu_Color{r, g, b, alpha}));
            XDrawPoint(display, drawable, gc, x + ix, y + iy);
#endif
        }
    }
}

} // namespace

Neu_CardItem::Neu_CardItem(std::wstring value)
    : text(std::move(value))
{
}

Neu_CardItem::Neu_CardItem(std::wstring value, std::string icon, Neu_CardIconPosition position)
    : text(std::move(value)), iconPath(std::move(icon)), iconPosition(position)
{
}

Neu_Card::Neu_Card(std::vector<Neu_CardItem> cardItems, int level)
    : items(std::move(cardItems)), levelIndex(level)
{
}

Neu_Cards::Neu_Cards(Neu_Layout layout)
    : Neu_Control(layout)
{
    autoScroll_ = true;
    setHintText("Neu_Cards: list-like cards with level indentation, card items, icons and simple rich text.");
}

void Neu_Cards::setCards(const std::vector<Neu_Card>& cards)
{
    cards_ = cards;
    if (selectedIndex_ >= static_cast<int>(cards_.size())) {
        selectedIndex_ = -1;
    }
    hoveredIndex_ = -1;
    requestRedraw();
}

void Neu_Cards::addCard(const Neu_Card& card)
{
    cards_.push_back(card);
    requestRedraw();
}

void Neu_Cards::clearCards()
{
    cards_.clear();
    selectedIndex_ = -1;
    hoveredIndex_ = -1;
    requestRedraw();
}

void Neu_Cards::setCardLevelOffset(int pixels)
{
    cardLevelOffset_ = std::max(0, pixels);
    requestRedraw();
}

void Neu_Cards::setCardMinHeight(int pixels)
{
    cardMinHeight_ = std::max(28, pixels);
    requestRedraw();
}

void Neu_Cards::setCardPadding(int left, int top, int right, int bottom)
{
    cardPaddingLeft_ = std::max(0, left);
    cardPaddingTop_ = std::max(0, top);
    cardPaddingRight_ = std::max(0, right);
    cardPaddingBottom_ = std::max(0, bottom);
    requestRedraw();
}

void Neu_Cards::setCardSpacing(int pixels)
{
    cardSpacing_ = std::max(0, pixels);
    requestRedraw();
}

void Neu_Cards::setItemSpacing(int pixels)
{
    itemSpacing_ = std::max(0, pixels);
    requestRedraw();
}

void Neu_Cards::setIconSize(int pixels)
{
    iconSize_ = std::max(0, pixels);
    requestRedraw();
}

void Neu_Cards::setSelectable(bool selectable)
{
    if (selectable_ != selectable) {
        selectable_ = selectable;
        if (!selectable_) {
            selectedIndex_ = -1;
        }
        requestRedraw();
    }
}

void Neu_Cards::setSelectedIndex(int index)
{
    const int next = (index >= 0 && index < static_cast<int>(cards_.size())) ? index : -1;
    if (selectedIndex_ != next) {
        selectedIndex_ = next;
        requestRedraw();
    }
}

Neu_IconBmp* Neu_Cards::iconForPath(const std::string& path) const
{
    if (path.empty()) {
        return nullptr;
    }
    auto found = iconCache_.find(path);
    if (found == iconCache_.end()) {
        Neu_IconBmp icon;
        icon.load(path);
        found = iconCache_.emplace(path, std::move(icon)).first;
    }
    if (found->second.pixels().empty()) {
        return nullptr;
    }
    return &found->second;
}

int Neu_Cards::cardHeight(const Neu_Card& card) const
{
    const int count = std::max(1, static_cast<int>(card.items.size()));
    const int body = count * itemLineHeight_ + std::max(0, count - 1) * itemSpacing_;
    return std::max(cardMinHeight_, cardPaddingTop_ + body + cardPaddingBottom_);
}

int Neu_Cards::cardAt(int px, int py) const
{
    const auto rect = bounds();
    if (px < rect.x || px >= rect.x + rect.width || py < rect.y || py >= rect.y + rect.height) {
        return -1;
    }
    int y = rect.y + outerPadding_ - scrollY();
    for (size_t i = 0; i < cards_.size(); ++i) {
        const int h = cardHeight(cards_[i]);
        if (py >= y && py < y + h) {
            return static_cast<int>(i);
        }
        y += h + cardSpacing_;
    }
    return -1;
}

std::string Neu_Cards::cardPrimaryTextUtf8(int index) const
{
    if (index < 0 || index >= static_cast<int>(cards_.size()) || cards_[static_cast<size_t>(index)].items.empty()) {
        return {};
    }
    return utf8FromWideString(cards_[static_cast<size_t>(index)].items.front().text);
}

void Neu_Cards::drawStyledTextLine(Display* display,
                                   Drawable drawable,
                                   GC gc,
                                   const Neu_Theme& theme,
                                   const std::string& text,
                                   int x,
                                   int textY,
                                   int maxWidth,
                                   bool rich)
{
    if (maxWidth <= 0 || text.empty()) {
        return;
    }

    if (!rich) {
        const std::string visible = truncateText_ ? truncateTextToWidth(display, drawable, gc, theme, text, maxWidth) : text;
        drawText(display, drawable, gc, theme, visible, x, textY);
        return;
    }

    int cursorX = x;
    const auto fragments = parseSimpleRichText(text, theme.text);
    for (const auto& fragment : fragments) {
        if (cursorX >= x + maxWidth) {
            break;
        }
        const int remaining = std::max(1, x + maxWidth - cursorX);
        const std::string visible = truncateText_ ? truncateTextToWidth(display,
                                                                        drawable,
                                                                        gc,
                                                                        theme,
                                                                        fragment.text,
                                                                        remaining) : fragment.text;
        const Neu_Color color = fragment.useFontColor ? fragment.fontColor : theme.text;
        drawTextColored(display,
                        drawable,
                        gc,
                        theme,
                        visible,
                        cursorX,
                        textY,
                        color,
                        fragment.bold,
                        fragment.italic,
                        fragment.underline,
                        fragment.strikethrough,
                        fragment.doubleStrikethrough,
                        fragment.monospace,
                        fragment.headingLevel);
        cursorX += measureTextWidth(display,
                                    drawable,
                                    gc,
                                    theme,
                                    visible,
                                    fragment.bold,
                                    fragment.italic,
                                    fragment.monospace,
                                    fragment.headingLevel) + 2;
    }
}

void Neu_Cards::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    const auto rect = bounds();

#ifdef _WIN32
    HBRUSH bg = CreateSolidBrush(RGB(theme.background.r, theme.background.g, theme.background.b));
    RECT bgRect{rect.x, rect.y, rect.x + rect.width, rect.y + rect.height};
    FillRect(drawable, &bgRect, bg);
    DeleteObject(bg);
#else
    XSetForeground(display, gc, Neu_Pixel(display, theme.background));
    XFillRectangle(display, drawable, gc, rect.x, rect.y, rect.width, rect.height);
#endif

    int maxIndent = 0;
    int naturalHeight = outerPadding_ * 2;
    for (const auto& card : cards_) {
        maxIndent = std::max(maxIndent, std::max(0, card.levelIndex) * cardLevelOffset_);
        naturalHeight += cardHeight(card) + cardSpacing_;
    }
    if (!cards_.empty()) {
        naturalHeight -= cardSpacing_;
    }
    setVirtualSize(rect.width, std::max(rect.height, naturalHeight));

    const int saved = saveClip(display, drawable, gc, rect.x, rect.y, rect.width, rect.height);
    int y = rect.y + outerPadding_ - scrollY();
    for (size_t i = 0; i < cards_.size(); ++i) {
        const auto& card = cards_[i];
        const int h = cardHeight(card);
        if (y + h < rect.y) {
            y += h + cardSpacing_;
            continue;
        }
        if (y > rect.y + rect.height) {
            break;
        }

        const int indent = std::max(0, card.levelIndex) * cardLevelOffset_;
        const int cardX = rect.x + outerPadding_ + indent - scrollX();
        const int cardW = std::max(48, rect.width - 2 * outerPadding_ - indent - 12);
        const bool selected = selectable_ && selectedIndex_ == static_cast<int>(i);
        const bool hovered = hoveredIndex_ == static_cast<int>(i);
        drawCardRect(display, drawable, gc, theme, cardX, y, cardW, h, selected, hovered);

        const int contentLeft = cardX + cardPaddingLeft_;
        const int contentTop = y + cardPaddingTop_;
        const int contentRight = cardX + cardW - cardPaddingRight_;
        const int contentBottom = y + h - cardPaddingBottom_;
        const int contentWidth = std::max(1, contentRight - contentLeft);
        const int contentHeight = std::max(1, contentBottom - contentTop);
        const int savedCardClip = saveClip(display, drawable, gc, contentLeft, contentTop, contentWidth, contentHeight);

        int lineTop = contentTop;
        for (size_t itemIndex = 0; itemIndex < card.items.size(); ++itemIndex) {
            if (lineTop + itemLineHeight_ > contentBottom) {
                break;
            }

            const auto& item = card.items[itemIndex];
            const bool primary = itemIndex == 0;
            const bool allowIcon = primary && !item.iconPath.empty() && iconSize_ > 0;
            int textLeft = contentLeft;
            int textRight = contentRight;

            if (allowIcon) {
                if (Neu_IconBmp* icon = iconForPath(item.iconPath)) {
                    const int iconY = lineTop + std::max(0, (itemLineHeight_ - iconSize_) / 2);
                    if (item.iconPosition == Neu_CardIconPosition::End) {
                        const int iconX = textRight - iconSize_;
                        drawCachedIcon(display, drawable, gc, *icon, iconX, iconY, iconSize_);
                        textRight -= iconSize_ + iconSpacing_;
                    } else {
                        drawCachedIcon(display, drawable, gc, *icon, textLeft, iconY, iconSize_);
                        textLeft += iconSize_ + iconSpacing_;
                    }
                }
            }

#ifdef _WIN32
            const int textY = lineTop + std::max(0, (itemLineHeight_ - 17) / 2);
#else
            const int textY = lineTop + std::max(14, itemLineHeight_ - 7);
#endif
            const int textWidth = std::max(1, textRight - textLeft);
            const std::string text = utf8FromWideString(item.text);
            drawStyledTextLine(display, drawable, gc, theme, text, textLeft, textY, textWidth, primary && item.simpleRichText);
            lineTop += itemLineHeight_ + itemSpacing_;
        }

        restoreClip(display, drawable, gc, savedCardClip);
#ifndef _WIN32
        saveClip(display, drawable, gc, rect.x, rect.y, rect.width, rect.height);
#endif

        y += h + cardSpacing_;
    }
    restoreClip(display, drawable, gc, saved);

    drawScrollbars(display, drawable, gc, theme);
    drawHintPopup(display, drawable, gc, theme);
}

void Neu_Cards::handleXEvent(XEvent& event)
{
#ifdef _WIN32
    if (event.message == WM_MOUSEMOVE) {
        const int next = cardAt(event.x, event.y);
        if (next != hoveredIndex_) {
            hoveredIndex_ = next;
            requestRedraw();
        }
    }

    if (event.message == WM_MOUSELEAVE && hoveredIndex_ != -1) {
        hoveredIndex_ = -1;
        requestRedraw();
    }

    if (event.message == WM_LBUTTONUP && contains(event.x, event.y)) {
        const int index = cardAt(event.x, event.y);
        if (index >= 0) {
            if (selectable_) {
                selectedIndex_ = index;
            }
            const std::string value = cardPrimaryTextUtf8(index);
            if (callbacks_.onSelectionChanged) {
                callbacks_.onSelectionChanged(this, index, 0, value.c_str(), callbacks_.userData);
            }
            invokeClick();
            requestRedraw();
        }
    }
#else
    if (event.type == MotionNotify) {
        const int next = cardAt(event.xmotion.x, event.xmotion.y);
        if (next != hoveredIndex_) {
            hoveredIndex_ = next;
            requestRedraw();
        }
    }

    if (event.type == LeaveNotify && hoveredIndex_ != -1) {
        hoveredIndex_ = -1;
        requestRedraw();
    }

    if (event.type == ButtonRelease && contains(event.xbutton.x, event.xbutton.y)) {
        const int index = cardAt(event.xbutton.x, event.xbutton.y);
        if (index >= 0) {
            if (selectable_) {
                selectedIndex_ = index;
            }
            const std::string value = cardPrimaryTextUtf8(index);
            if (callbacks_.onSelectionChanged) {
                callbacks_.onSelectionChanged(this, index, 0, value.c_str(), callbacks_.userData);
            }
            invokeClick();
            requestRedraw();
        }
    }
#endif

    Neu_Control::handleXEvent(event);
}

std::wstring Neu_CardItem::toWide(const std::string& utf8)
{
    return wideFromUtf8String(utf8);
}

std::string Neu_CardItem::toUtf8(const std::wstring& wide)
{
    return utf8FromWideString(wide);
}

} // namespace neutrino
