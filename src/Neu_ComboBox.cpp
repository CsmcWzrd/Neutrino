#include "Neutrino/Neutrino.hpp"

namespace neutrino {

namespace {

static void drawTriangle(Display* display,
                         Drawable drawable,
                         GC gc,
                         const Neu_Color& color,
                         int centerX,
                         int centerY,
                         bool up)
{
    const int halfWidth = 4;
    const int halfHeight = 3;
    XPoint pts[3];
    if (up) {
        pts[0] = XPoint{static_cast<short>(centerX), static_cast<short>(centerY - halfHeight)};
        pts[1] = XPoint{static_cast<short>(centerX - halfWidth), static_cast<short>(centerY + halfHeight)};
        pts[2] = XPoint{static_cast<short>(centerX + halfWidth), static_cast<short>(centerY + halfHeight)};
    } else {
        pts[0] = XPoint{static_cast<short>(centerX - halfWidth), static_cast<short>(centerY - halfHeight)};
        pts[1] = XPoint{static_cast<short>(centerX + halfWidth), static_cast<short>(centerY - halfHeight)};
        pts[2] = XPoint{static_cast<short>(centerX), static_cast<short>(centerY + halfHeight)};
    }
    XSetForeground(display, gc, Neu_Pixel(display, color));
    XFillPolygon(display, drawable, gc, pts, 3, Convex, CoordModeOrigin);
}

} // namespace

void Neu_ComboBox::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(display, drawable, gc, theme);
    const auto rect = bounds();
    const int buttonW = 24;
    const std::string selectedText = selected_ >= 0 && selected_ < static_cast<int>(items_.size())
                                     ? items_[static_cast<size_t>(selected_)]
                                     : std::string{};

    const int textLeft = rect.x + textOffset_.left + 4;
    const int textRight = rect.x + rect.width - buttonW - textOffset_.right;
    XRectangle textClip{static_cast<short>(textLeft),
                        static_cast<short>(rect.y + 4),
                        static_cast<unsigned short>(std::max(1, textRight - textLeft)),
                        static_cast<unsigned short>(std::max(1, rect.height - 8))};
    XSetClipRectangles(display, gc, 0, 0, &textClip, 1, Unsorted);
    const std::string visible = truncateTextToWidth(display, drawable, gc, theme, selectedText, std::max(1, textRight - textLeft - 4));
    drawText(display, drawable, gc, theme, visible, textLeft, rect.y + rect.height / 2 + 5);
    XSetClipMask(display, gc, None);

    XSetForeground(display, gc, Neu_Pixel(display, theme.border));
    const int buttonLeft = rect.x + rect.width - buttonW;
    XDrawLine(display, drawable, gc, buttonLeft, rect.y + 4, buttonLeft, rect.y + rect.height - 4);
    const int arrowCenterX = buttonLeft + buttonW / 2;
    const int arrowCenterY = rect.y + rect.height / 2;
    drawTriangle(display, drawable, gc, theme.text, arrowCenterX, arrowCenterY, false);

    if (open_ && !items_.empty()) {
        const int itemH = 22;
        const int visibleRows = std::min<int>(8, static_cast<int>(items_.size()));
        const int dropH = std::max(itemH, visibleRows * itemH + 4);
        const int dropX = rect.x;
        const int dropY = rect.y + rect.height + 2;
        const int dropW = rect.width;
        const int listRight = dropX + dropW - (static_cast<int>(items_.size()) > visibleRows ? 12 : 0);
        Neu_DrawSmoothRoundedRect(display, drawable, gc,
                                  theme.glass,
                                  theme.background,
                                  dropX,
                                  dropY,
                                  dropW,
                                  dropH,
                                  std::max(2, theme.radius - 2),
                                  true,
                                  theme.antiAliasSamples);
        XRectangle dropClip{static_cast<short>(dropX + 2),
                            static_cast<short>(dropY + 2),
                            static_cast<unsigned short>(std::max(1, listRight - dropX - 4)),
                            static_cast<unsigned short>(std::max(1, dropH - 4))};
        XSetClipRectangles(display, gc, 0, 0, &dropClip, 1, Unsorted);
        const int maxScroll = std::max(0, static_cast<int>(items_.size()) * itemH - (dropH - 4));
        if (scrollY_ > maxScroll) {
            scrollY_ = maxScroll;
        }
        int y = dropY + 4 - scrollY_;
        for (size_t i = 0; i < items_.size(); ++i, y += itemH) {
            if (y + itemH < dropY + 2) {
                continue;
            }
            if (y > dropY + dropH - 2) {
                break;
            }
            const bool selected = selectedIndices_.count(static_cast<int>(i)) != 0U || selected_ == static_cast<int>(i);
            const bool hover = hoveredIndex_ == static_cast<int>(i);
            if (selected || hover) {
                Neu_DrawSmoothRoundedRect(display, drawable, gc,
                                          selected ? theme.pressed : theme.hover,
                                          theme.background,
                                          dropX + 3,
                                          y,
                                          std::max(1, listRight - dropX - 6),
                                          itemH,
                                          std::max(2, theme.radius - 4),
                                          true,
                                          theme.antiAliasSamples);
            }
            drawText(display,
                     drawable,
                     gc,
                     theme,
                     truncateTextToWidth(display, drawable, gc, theme, items_[i], std::max(1, listRight - dropX - 10)),
                     dropX + 6,
                     y + 16);
        }
        XSetClipMask(display, gc, None);
        if (static_cast<int>(items_.size()) > visibleRows) {
            const int trackX = dropX + dropW - 10;
            const int trackY = dropY + 4;
            const int trackH = std::max(1, dropH - 8);
            XSetForeground(display, gc, Neu_Pixel(display, Neu_MixColor(theme.glass, theme.border, 0.35)));
            XFillRectangle(display, drawable, gc, trackX, trackY, 6, static_cast<unsigned int>(trackH));
            const int thumbH = std::max(16, trackH * visibleRows / static_cast<int>(items_.size()));
            const int thumbY = trackY + (maxScroll > 0 ? (trackH - thumbH) * scrollY_ / maxScroll : 0);
            Neu_DrawSmoothRoundedRect(display, drawable, gc,
                                      theme.accent,
                                      theme.background,
                                      trackX,
                                      thumbY,
                                      6,
                                      thumbH,
                                      3,
                                      true,
                                      theme.antiAliasSamples);
        }
    }
}

void Neu_ComboBox::handleXEvent(XEvent& event)
{
    const auto rect = bounds();
    const int itemH = 22;
    const int visibleRows = std::min<int>(8, static_cast<int>(items_.size()));
    const int dropH = std::max(itemH, visibleRows * itemH + 4);
    const int dropX = rect.x;
    const int dropY = rect.y + rect.height + 2;
    auto point = [&](int& x, int& y) {
        if (event.type == MotionNotify) { x = event.xmotion.x; y = event.xmotion.y; }
        else if (event.type == ButtonPress || event.type == ButtonRelease) { x = event.xbutton.x; y = event.xbutton.y; }
        else { x = 0; y = 0; }
    };
    int x = 0;
    int y = 0;
    point(x, y);
    const bool inBase = contains(x, y);
    const bool inDrop = open_ && x >= dropX && x <= dropX + rect.width && y >= dropY && y <= dropY + dropH;

    if (event.type == ButtonPress && inDrop && event.xbutton.button == Button4) {
        scrollY_ = std::max(0, scrollY_ - itemH);
        requestRedraw();
        return;
    }
    if (event.type == ButtonPress && inDrop && event.xbutton.button == Button5) {
        const int maxScroll = std::max(0, static_cast<int>(items_.size()) * itemH - (dropH - 4));
        scrollY_ = std::min(maxScroll, scrollY_ + itemH);
        requestRedraw();
        return;
    }
    if (event.type == MotionNotify) {
        hoveredIndex_ = inDrop ? std::max(0, std::min(static_cast<int>(items_.size()) - 1, (y - (dropY + 4) + scrollY_) / itemH)) : -1;
        requestRedraw();
        return;
    }
    if (event.type == ButtonRelease && inBase) {
        open_ = !open_;
        requestRedraw();
        return;
    }
    if (event.type == ButtonRelease && inDrop && !items_.empty()) {
        const int idx = std::max(0, std::min(static_cast<int>(items_.size()) - 1, (y - (dropY + 4) + scrollY_) / itemH));
        selected_ = idx;
        selectedIndices_.clear();
        selectedIndices_.insert(idx);
        anchorIndex_ = idx;
        open_ = false;
        if (callbacks_.onSelectionChanged) {
            callbacks_.onSelectionChanged(this, selected_, 0, items_[static_cast<size_t>(selected_)].c_str(), callbacks_.userData);
        }
        requestRedraw();
        return;
    }
    if (event.type == ButtonPress && open_ && !inBase && !inDrop) {
        open_ = false;
        requestRedraw();
    }
}

} // namespace neutrino
