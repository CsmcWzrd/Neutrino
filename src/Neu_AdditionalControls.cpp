#include "Neutrino/Neutrino.hpp"
#include <X11/keysym.h>
#include <algorithm>
#include <cmath>
#include <iterator>

namespace neutrino {

namespace {

static int eventX(const XEvent& ev)
{
    if (ev.type == MotionNotify) {
        return ev.xmotion.x;
    }
    if (ev.type == ButtonPress || ev.type == ButtonRelease) {
        return ev.xbutton.x;
    }
    return 0;
}

static int eventY(const XEvent& ev)
{
    if (ev.type == MotionNotify) {
        return ev.xmotion.y;
    }
    if (ev.type == ButtonPress || ev.type == ButtonRelease) {
        return ev.xbutton.y;
    }
    return 0;
}

static std::string approximateTruncate(const std::string& text, int width)
{
    const int maxChars = std::max(0, width / 7);
    if (static_cast<int>(text.size()) <= maxChars) {
        return text;
    }
    if (maxChars <= 3) {
        return text.substr(0, static_cast<size_t>(maxChars));
    }
    return text.substr(0, static_cast<size_t>(maxChars - 3)) + "...";
}

static void drawClippedText(Display* display,
                            Drawable drawable,
                            GC gc,
                            const Neu_Theme& theme,
                            const std::string& text,
                            int left,
                            int top,
                            int width,
                            int height,
                            int baseline,
                            const Neu_Color* color = nullptr,
                            bool underline = false)
{
    XRectangle clip{static_cast<short>(left),
                    static_cast<short>(top),
                    static_cast<unsigned short>(std::max(1, width)),
                    static_cast<unsigned short>(std::max(1, height))};
    XSetClipRectangles(display, gc, 0, 0, &clip, 1, Unsorted);
    const std::string visible = approximateTruncate(text, width);
    const Neu_Color useColor = color ? *color : theme.text;
    XSetForeground(display, gc, Neu_Pixel(display, useColor));
    XDrawString(display, drawable, gc, left, baseline, visible.c_str(), static_cast<int>(visible.size()));
    if (underline) {
        XDrawLine(display, drawable, gc, left, baseline + 2, left + std::min(width, static_cast<int>(visible.size()) * 7), baseline + 2);
    }
    XSetClipMask(display, gc, None);
}


static void drawTriangle(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme, int cx, int cy, bool up)
{
    XSetForeground(display, gc, Neu_Pixel(display, theme.text));
    const int halfWidth = 4;
    const int halfHeight = 3;
    if (up) {
        XPoint pts[3]{{static_cast<short>(cx - halfWidth), static_cast<short>(cy + halfHeight)},
                      {static_cast<short>(cx + halfWidth), static_cast<short>(cy + halfHeight)},
                      {static_cast<short>(cx), static_cast<short>(cy - halfHeight)}};
        XFillPolygon(display, drawable, gc, pts, 3, Convex, CoordModeOrigin);
    } else {
        XPoint pts[3]{{static_cast<short>(cx - halfWidth), static_cast<short>(cy - halfHeight)},
                      {static_cast<short>(cx + halfWidth), static_cast<short>(cy - halfHeight)},
                      {static_cast<short>(cx), static_cast<short>(cy + halfHeight)}};
        XFillPolygon(display, drawable, gc, pts, 3, Convex, CoordModeOrigin);
    }
}


static void drawSmallTriangle(Display* display,
                              Drawable drawable,
                              GC gc,
                              const Neu_Color& color,
                              int centerX,
                              int centerY,
                              bool up)
{
    XPoint points[3];
    if (up) {
        points[0] = XPoint{static_cast<short>(centerX), static_cast<short>(centerY - 4)};
        points[1] = XPoint{static_cast<short>(centerX - 5), static_cast<short>(centerY + 3)};
        points[2] = XPoint{static_cast<short>(centerX + 5), static_cast<short>(centerY + 3)};
    } else {
        points[0] = XPoint{static_cast<short>(centerX - 5), static_cast<short>(centerY - 3)};
        points[1] = XPoint{static_cast<short>(centerX + 5), static_cast<short>(centerY - 3)};
        points[2] = XPoint{static_cast<short>(centerX), static_cast<short>(centerY + 4)};
    }
    XSetForeground(display, gc, Neu_Pixel(display, color));
    XFillPolygon(display, drawable, gc, points, 3, Convex, CoordModeOrigin);
}

static bool primaryButtonPress(const XEvent& ev)
{
    return ev.type == ButtonPress && ev.xbutton.button == Button1;
}

static bool primaryButtonRelease(const XEvent& ev)
{
    return ev.type == ButtonRelease && ev.xbutton.button == Button1;
}

static int clampSplitterPosition(const Neu_Rect& rect, bool vertical, int requested, int minimumPaneSize, int sashSize)
{
    const int extent = std::max(1, vertical ? rect.width : rect.height);
    const int halfSash = std::max(1, sashSize / 2);
    const int minPane = std::max(0, minimumPaneSize);
    const int low = 4 + minPane + halfSash;
    const int high = extent - 4 - minPane - halfSash;
    if (high < low) {
        return std::max(4 + halfSash, extent / 2);
    }
    return std::max(low, std::min(requested, high));
}

static XRectangle splitterPaneRect(const Neu_Rect& rect, bool vertical, bool firstPane, int split, int sashSize)
{
    const int halfSash = std::max(1, sashSize / 2);
    if (vertical) {
        const int x = firstPane ? rect.x + 4 : rect.x + split + halfSash;
        const int right = firstPane ? rect.x + split - halfSash : rect.x + rect.width - 4;
        return XRectangle{static_cast<short>(x),
                          static_cast<short>(rect.y + 4),
                          static_cast<unsigned short>(std::max(0, right - x)),
                          static_cast<unsigned short>(std::max(0, rect.height - 8))};
    }
    const int y = firstPane ? rect.y + 4 : rect.y + split + halfSash;
    const int bottom = firstPane ? rect.y + split - halfSash : rect.y + rect.height - 4;
    return XRectangle{static_cast<short>(rect.x + 4),
                      static_cast<short>(y),
                      static_cast<unsigned short>(std::max(0, rect.width - 8)),
                      static_cast<unsigned short>(std::max(0, bottom - y))};
}

static bool splitterChildInFirstPane(const Neu_Rect& rect,
                                     const Neu_Rect& childRect,
                                     bool vertical,
                                     size_t childIndex,
                                     size_t childCount,
                                     int split)
{
    if (childCount <= 1U) {
        return true;
    }
    if (childIndex == 0U) {
        return true;
    }
    if (childIndex == 1U) {
        return false;
    }
    const int splitAbs = vertical ? rect.x + split : rect.y + split;
    return vertical ? childRect.x < splitAbs : childRect.y < splitAbs;
}

static bool pointInPaneRect(const XRectangle& pane, int x, int y)
{
    return pane.width > 0 && pane.height > 0 &&
           x >= pane.x && x < pane.x + static_cast<int>(pane.width) &&
           y >= pane.y && y < pane.y + static_cast<int>(pane.height);
}

} // namespace

void Neu_CheckBox::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    const auto rect = bounds();
    const int box = std::max(12, std::min(18, rect.height - 8));
    const int boxX = rect.x + 4;
    const int boxY = rect.y + (rect.height - box) / 2;

    XSetForeground(display, gc, Neu_Pixel(display, hover_ ? theme.hover : theme.glass));
    XFillRectangle(display, drawable, gc, rect.x, rect.y, static_cast<unsigned int>(rect.width), static_cast<unsigned int>(rect.height));
    Neu_DrawSmoothRoundedRect(display, drawable, gc, theme.glass, theme.background, boxX, boxY, box, box, 4, true);
    Neu_DrawSmoothRoundedRect(display, drawable, gc, focused_ ? theme.accent : theme.border, theme.background, boxX, boxY, box, box, 4, false);
    if (checked_) {
        XSetForeground(display, gc, Neu_Pixel(display, theme.accent));
        XDrawLine(display, drawable, gc, boxX + 3, boxY + box / 2, boxX + box / 2, boxY + box - 3);
        XDrawLine(display, drawable, gc, boxX + box / 2, boxY + box - 3, boxX + box - 3, boxY + 3);
        XDrawLine(display, drawable, gc, boxX + 4, boxY + box / 2, boxX + box / 2 + 1, boxY + box - 3);
    }

    const int textLeft = boxX + box + 7 + textOffset_.left;
    const int textTop = rect.y + textOffset_.top;
    const int textRight = rect.x + rect.width - textOffset_.right;
    drawClippedText(display, drawable, gc, theme, text_, textLeft, textTop,
                    std::max(1, textRight - textLeft), std::max(1, rect.height - textOffset_.top - textOffset_.bottom),
                    rect.y + rect.height / 2 + 5);
    drawHintPopup(display, drawable, gc, theme);
}

void Neu_CheckBox::handleXEvent(XEvent& ev)
{
    Neu_Control::handleXEvent(ev);
    if (primaryButtonRelease(ev) && contains(ev.xbutton.x, ev.xbutton.y)) {
        checked_ = !checked_;
        invokeClick();
        requestRedraw();
    }
}

void Neu_RadioButton::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    const auto rect = bounds();
    const int size = std::max(12, std::min(18, rect.height - 8));
    const int cx = rect.x + 4 + size / 2;
    const int cy = rect.y + rect.height / 2;

    XSetForeground(display, gc, Neu_Pixel(display, hover_ ? theme.hover : theme.glass));
    XFillRectangle(display, drawable, gc, rect.x, rect.y, static_cast<unsigned int>(rect.width), static_cast<unsigned int>(rect.height));
    Neu_DrawSmoothRoundedRect(display, drawable, gc, theme.glass, theme.background, cx - size / 2, cy - size / 2, size, size, size / 2, true);
    Neu_DrawSmoothRoundedRect(display, drawable, gc, focused_ ? theme.accent : theme.border, theme.background, cx - size / 2, cy - size / 2, size, size, size / 2, false);
    if (checked_) {
        Neu_DrawSmoothRoundedRect(display, drawable, gc, theme.accent, theme.background, cx - size / 4, cy - size / 4, size / 2, size / 2, size / 4, true);
    }

    const int textLeft = rect.x + 4 + size + 7 + textOffset_.left;
    const int textRight = rect.x + rect.width - textOffset_.right;
    drawClippedText(display, drawable, gc, theme, text_, textLeft, rect.y + textOffset_.top,
                    std::max(1, textRight - textLeft), std::max(1, rect.height - textOffset_.top - textOffset_.bottom),
                    rect.y + rect.height / 2 + 5);
    drawHintPopup(display, drawable, gc, theme);
}

void Neu_ToggleButton::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    const auto rect = bounds();
    XSetForeground(display, gc, Neu_Pixel(display, checked_ ? theme.pressed : (hover_ ? theme.hover : theme.glass)));
    Neu_DrawRoundedRect(display, drawable, gc, rect.x, rect.y, rect.width, rect.height, theme.radius, true);
    XSetForeground(display, gc, Neu_Pixel(display, focused_ ? theme.accent : theme.border));
    Neu_DrawRoundedRect(display, drawable, gc, rect.x, rect.y, rect.width, rect.height, theme.radius, false);
    const int textLeft = rect.x + textOffset_.left;
    const int textRight = rect.x + rect.width - textOffset_.right;
    drawClippedText(display, drawable, gc, theme, text_, textLeft, rect.y + textOffset_.top,
                    std::max(1, textRight - textLeft), std::max(1, rect.height - textOffset_.top - textOffset_.bottom),
                    rect.y + rect.height / 2 + 5);
    drawHintPopup(display, drawable, gc, theme);
}

void Neu_ProgressBar::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(display, drawable, gc, theme);
    const auto rect = bounds();
    const int innerX = rect.x + 5;
    const int innerY = rect.y + 5;
    const int innerW = std::max(1, rect.width - 10);
    const int innerH = std::max(1, rect.height - 10);
    const int filledW = static_cast<int>(innerW * progress_);
    if (filledW > 0) {
        Neu_DrawSmoothRoundedRect(display, drawable, gc, theme.focus, theme.background,
                                  innerX, innerY, filledW, innerH,
                                  std::max(2, theme.radius - 4),
                                  true,
                                  theme.antiAliasSamples);
    }
    drawClippedText(display, drawable, gc, theme,
                    text_.empty() ? std::to_string(static_cast<int>(progress_ * 100.0f)) + "%" : text_,
                    innerX, innerY, innerW, innerH, rect.y + rect.height / 2 + 5);
    drawHintPopup(display, drawable, gc, theme);
}

void Neu_Slider::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    const auto rect = bounds();
    Neu_Control::draw(display, drawable, gc, theme);
    const float t = (max_ == min_) ? 0.0f : static_cast<float>(value_ - min_) / static_cast<float>(max_ - min_);
    XSetForeground(display, gc, Neu_Pixel(display, theme.border));
    if (vertical_) {
        const int trackX = rect.x + rect.width / 2;
        XDrawLine(display, drawable, gc, trackX, rect.y + 8, trackX, rect.y + rect.height - 8);
        const int knobY = rect.y + 8 + static_cast<int>((rect.height - 16) * (1.0f - t));
        XSetForeground(display, gc, Neu_Pixel(display, theme.accent));
        XFillArc(display, drawable, gc, trackX - 7, knobY - 7, 14, 14, 0, 360 * 64);
    } else {
        const int trackY = rect.y + rect.height / 2;
        XDrawLine(display, drawable, gc, rect.x + 8, trackY, rect.x + rect.width - 8, trackY);
        const int knobX = rect.x + 8 + static_cast<int>((rect.width - 16) * t);
        XSetForeground(display, gc, Neu_Pixel(display, theme.accent));
        XFillArc(display, drawable, gc, knobX - 7, trackY - 7, 14, 14, 0, 360 * 64);
    }
    drawHintPopup(display, drawable, gc, theme);
}

void Neu_Slider::handleXEvent(XEvent& ev)
{
    Neu_Control::handleXEvent(ev);
    if (primaryButtonPress(ev) && contains(ev.xbutton.x, ev.xbutton.y)) {
        dragging_ = true;
    }
    if (primaryButtonRelease(ev)) {
        dragging_ = false;
    }
    if ((dragging_ && ev.type == MotionNotify) || (primaryButtonPress(ev) && contains(ev.xbutton.x, ev.xbutton.y))) {
        const auto rect = bounds();
        float t = 0.0f;
        if (vertical_) {
            t = 1.0f - static_cast<float>(eventY(ev) - rect.y - 8) / static_cast<float>(std::max(1, rect.height - 16));
        } else {
            t = static_cast<float>(eventX(ev) - rect.x - 8) / static_cast<float>(std::max(1, rect.width - 16));
        }
        t = std::max(0.0f, std::min(1.0f, t));
        setValue(min_ + static_cast<int>((max_ - min_) * t + 0.5f));
    }
}

void Neu_Spinner::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(display, drawable, gc, theme);
    const auto rect = bounds();
    const int buttonW = 22;
    const int textLeft = rect.x + textOffset_.left + 8;
    const int textRight = rect.x + rect.width - buttonW - textOffset_.right;
    drawClippedText(display, drawable, gc, theme, text_, textLeft, rect.y + textOffset_.top,
                    std::max(1, textRight - textLeft), std::max(1, rect.height - textOffset_.top - textOffset_.bottom),
                    rect.y + rect.height / 2 + 5);
    XSetForeground(display, gc, Neu_Pixel(display, theme.border));
    XDrawLine(display, drawable, gc, rect.x + rect.width - buttonW, rect.y + 2, rect.x + rect.width - buttonW, rect.y + rect.height - 2);
    const int arrowX = rect.x + rect.width - buttonW / 2;
    const int topButtonTop = rect.y + 2;
    const int bottomButtonBottom = rect.y + rect.height - 2;
    drawTriangle(display, drawable, gc, theme, arrowX, topButtonTop + std::max(7, (rect.height - 4) / 4), true);
    drawTriangle(display, drawable, gc, theme, arrowX, bottomButtonBottom - std::max(7, (rect.height - 4) / 4), false);
    drawHintPopup(display, drawable, gc, theme);
}

void Neu_Spinner::handleXEvent(XEvent& ev)
{
    Neu_Control::handleXEvent(ev);
    if (primaryButtonRelease(ev) && contains(ev.xbutton.x, ev.xbutton.y)) {
        const auto rect = bounds();
        if (ev.xbutton.x >= rect.x + rect.width - 24) {
            setValue(value_ + (ev.xbutton.y < rect.y + rect.height / 2 ? 1 : -1));
            invokeTextChanged();
            invokeClick();
        }
    }
}

void Neu_GroupBox::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    const auto rect = bounds();
    XSetForeground(display, gc, Neu_Pixel(display, theme.border));
    XDrawRectangle(display, drawable, gc, rect.x, rect.y + 8, std::max(1, rect.width - 1), std::max(1, rect.height - 9));
    if (!text_.empty()) {
        drawText(display, drawable, gc, theme, text_, rect.x + 10 + textOffset_.left, rect.y + 14 + textOffset_.top);
    }
    for (auto& child : children()) {
        if (child && child->visible()) {
            child->draw(display, drawable, gc, theme);
        }
    }
    drawHintPopup(display, drawable, gc, theme);
}

void Neu_Separator::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    const auto rect = bounds();
    XSetForeground(display, gc, Neu_Pixel(display, theme.border));
    if (vertical_) {
        const int x = rect.x + rect.width / 2;
        XDrawLine(display, drawable, gc, x, rect.y, x, rect.y + rect.height);
    } else {
        const int y = rect.y + rect.height / 2;
        XDrawLine(display, drawable, gc, rect.x, y, rect.x + rect.width, y);
    }
}

void Neu_LinkLabel::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    const auto rect = bounds();
    const int left = rect.x + textOffset_.left;
    const int right = rect.x + rect.width - textOffset_.right;
    drawClippedText(display, drawable, gc, theme, text_, left, rect.y + textOffset_.top,
                    std::max(1, right - left), std::max(1, rect.height - textOffset_.top - textOffset_.bottom),
                    rect.y + rect.height / 2 + 5, &theme.accent, true);
    drawHintPopup(display, drawable, gc, theme);
}

void Neu_LinkLabel::handleXEvent(XEvent& ev)
{
    Neu_Control::handleXEvent(ev);
    if (primaryButtonRelease(ev) && contains(ev.xbutton.x, ev.xbutton.y)) {
        invokeClick();
    }
}

void Neu_ToolBar::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(display, drawable, gc, theme);
    for (auto& child : children()) {
        if (child && child->visible()) {
            child->draw(display, drawable, gc, theme);
        }
    }
    drawHintPopup(display, drawable, gc, theme);
}

int Neu_TabView::addTab(const std::string& title, std::shared_ptr<Neu_Placement> page)
{
    titles_.push_back(title);
    pages_.push_back(page);
    if (page) {
        page->setParent(parent_);
    }
    return static_cast<int>(titles_.size()) - 1;
}

void Neu_TabView::setParent(Neu_Window* parent)
{
    Neu_Control::setParent(parent);
    for (auto& page : pages_) {
        if (page) {
            page->setParent(parent);
        }
    }
}

void Neu_TabView::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(display, drawable, gc, theme);
    const auto rect = bounds();
    const int tabHeight = 26;
    int x = rect.x + 6;
    for (size_t i = 0; i < titles_.size(); ++i) {
        const int w = std::max(72, measureTextWidth(display, drawable, gc, theme, titles_[i]) + 18);
        Neu_DrawSmoothRoundedRect(display, drawable, gc,
                                  static_cast<int>(i) == selectedTab_ ? theme.pressed : theme.hover,
                                  theme.background,
                                  x, rect.y + 4, w, tabHeight,
                                  std::max(2, theme.radius - 3),
                                  true,
                                  theme.antiAliasSamples);
        XSetForeground(display, gc, Neu_Pixel(display, theme.border));
        Neu_DrawRoundedRect(display, drawable, gc, x, rect.y + 4, w, tabHeight, std::max(2, theme.radius - 3), false);
        drawClippedText(display, drawable, gc, theme, titles_[i], x + 8, rect.y + 4, w - 12, tabHeight, rect.y + 22);
        x += w + 2;
    }
    if (selectedTab_ >= 0 && selectedTab_ < static_cast<int>(pages_.size()) && pages_[static_cast<size_t>(selectedTab_)]) {
        pages_[static_cast<size_t>(selectedTab_)]->draw(display, drawable, gc, theme);
    }
    drawHintPopup(display, drawable, gc, theme);
}

void Neu_TabView::handleXEvent(XEvent& ev)
{
    const auto rect = bounds();
    if ((primaryButtonRelease(ev) || primaryButtonPress(ev)) && contains(eventX(ev), eventY(ev))) {
        int x = rect.x + 6;
        for (size_t i = 0; i < titles_.size(); ++i) {
            const int w = std::max(72, static_cast<int>(titles_[i].size()) * 8 + 18);
            if (eventY(ev) >= rect.y + 4 && eventY(ev) <= rect.y + 30 && eventX(ev) >= x && eventX(ev) <= x + w) {
                selectedTab_ = static_cast<int>(i);
                requestRedraw();
                return;
            }
            x += w + 2;
        }
    }
    if (selectedTab_ >= 0 && selectedTab_ < static_cast<int>(pages_.size()) && pages_[static_cast<size_t>(selectedTab_)]) {
        pages_[static_cast<size_t>(selectedTab_)]->handleXEvent(ev);
        return;
    }
    Neu_Control::handleXEvent(ev);
}

void Neu_Splitter::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(display, drawable, gc, theme);
    const auto rect = bounds();
    const int sash = std::max(3, sashSize_);
    const int halfSash = std::max(1, sash / 2);
    const int split = clampSplitterPosition(rect, vertical_, splitPosition_, minimumPaneSize_, sash);
    if (split != splitPosition_) {
        splitPosition_ = split;
    }

    const XRectangle firstPane = splitterPaneRect(rect, vertical_, true, split, sash);
    const XRectangle secondPane = splitterPaneRect(rect, vertical_, false, split, sash);
    const int bufferWidth = std::max(rect.x + rect.width + 8, parent() ? parent()->width() : rect.x + rect.width + 8);
    const int bufferHeight = std::max(rect.y + rect.height + 8, parent() ? parent()->height() : rect.y + rect.height + 8);
    const int depth = DefaultDepth(display, DefaultScreen(display));
    const size_t childCount = children().size();

    for (size_t index = 0; index < childCount; ++index) {
        auto& child = children()[index];
        if (!child || !child->visible()) {
            continue;
        }
        const auto childRect = child->bounds();
        const bool inFirstPane = splitterChildInFirstPane(rect, childRect, vertical_, index, childCount, split);
        XRectangle pane = inFirstPane ? firstPane : secondPane;
        if (pane.width == 0 || pane.height == 0) {
            continue;
        }

        // X11 controls often reset the GC clip internally while drawing text or scrollbars.
        // Draw into a scratch pixmap and copy back only the pane rectangle so a child is
        // still visibly clipped when the sash crosses its start position or interior.
        Pixmap scratch = XCreatePixmap(display, drawable, static_cast<unsigned int>(bufferWidth), static_cast<unsigned int>(bufferHeight), depth);
        if (scratch) {
            XSetClipMask(display, gc, None);
            XCopyArea(display, drawable, scratch, gc, 0, 0, static_cast<unsigned int>(bufferWidth), static_cast<unsigned int>(bufferHeight), 0, 0);
            child->draw(display, scratch, gc, theme);
            XSetClipMask(display, gc, None);
            XCopyArea(display, scratch, drawable, gc, pane.x, pane.y, pane.width, pane.height, pane.x, pane.y);
            XFreePixmap(display, scratch);
        } else {
            XSetClipRectangles(display, gc, 0, 0, &pane, 1, Unsorted);
            child->draw(display, drawable, gc, theme);
            XSetClipMask(display, gc, None);
        }
    }

    XSetForeground(display, gc, Neu_Pixel(display, theme.accent));
    if (vertical_) {
        const int x = rect.x + split;
        XFillRectangle(display, drawable, gc, x - halfSash, rect.y + 4, static_cast<unsigned int>(sash), std::max(1, rect.height - 8));
    } else {
        const int y = rect.y + split;
        XFillRectangle(display, drawable, gc, rect.x + 4, y - halfSash, std::max(1, rect.width - 8), static_cast<unsigned int>(sash));
    }
    drawHintPopup(display, drawable, gc, theme);
}

void Neu_Splitter::handleXEvent(XEvent& ev)
{
    const auto rect = bounds();
    const int ex = eventX(ev);
    const int ey = eventY(ev);
    const int sash = std::max(3, sashSize_);
    const int halfSash = std::max(1, sash / 2);
    const int split = clampSplitterPosition(rect, vertical_, splitPosition_, minimumPaneSize_, sash);

    const bool onSash = vertical_
                        ? (ex >= rect.x + split - halfSash - 2 && ex <= rect.x + split + halfSash + 2 && ey >= rect.y && ey <= rect.y + rect.height)
                        : (ey >= rect.y + split - halfSash - 2 && ey <= rect.y + split + halfSash + 2 && ex >= rect.x && ex <= rect.x + rect.width);

    if (primaryButtonPress(ev) && onSash) {
        dragging_ = true;
        return;
    }
    if (primaryButtonRelease(ev)) {
        dragging_ = false;
    }
    if (dragging_ && ev.type == MotionNotify) {
        setSplitPosition(vertical_ ? ex - rect.x : ey - rect.y);
        return;
    }

    if (ev.type == MotionNotify || ev.type == ButtonPress || ev.type == ButtonRelease) {
        const size_t childCount = children().size();
        for (auto it = children().rbegin(); it != children().rend(); ++it) {
            const size_t reverseIndex = static_cast<size_t>(std::distance(children().rbegin(), it));
            const size_t childIndex = childCount - 1U - reverseIndex;
            auto& child = *it;
            if (!child || !child->visible() || !child->enabled()) {
                continue;
            }
            const auto childRect = child->bounds();
            const bool firstPane = splitterChildInFirstPane(rect, childRect, vertical_, childIndex, childCount, split);
            XRectangle pane = splitterPaneRect(rect, vertical_, firstPane, split, sash);
            if (pointInPaneRect(pane, ex, ey) && child->contains(ex, ey)) {
                child->handleXEvent(ev);
                return;
            }
        }
    }

    Neu_Control::handleXEvent(ev);
}

} // namespace neutrino
