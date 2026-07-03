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

    if (open_) {
        Neu_Listbox::draw(display, drawable, gc, theme);
    }
}

void Neu_ComboBox::handleXEvent(XEvent& event)
{
    if (event.type == ButtonRelease && contains(event.xbutton.x, event.xbutton.y)) {
        open_ = !open_;
    }

    if (open_) {
        Neu_Listbox::handleXEvent(event);
    }

    requestRedraw();
}

} // namespace neutrino
