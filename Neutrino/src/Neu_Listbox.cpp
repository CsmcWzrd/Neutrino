#include "Neutrino/Neutrino.hpp"

namespace neutrino {

void Neu_Listbox::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(display, drawable, gc, theme);
    const auto rect = bounds();
    constexpr int rowHeight = 22;
    const int contentRight = rect.x + rect.width - 16;
    int y = rect.y + 20 - scrollY();
    autoScroll_ = autoScroll_ || static_cast<int>(items_.size()) * rowHeight > rect.height;
    setVirtualSize(rect.width, std::max(rect.height, static_cast<int>(items_.size()) * rowHeight + 10));

    XRectangle clip{};
    clip.x = static_cast<short>(rect.x + 2);
    clip.y = static_cast<short>(rect.y + 2);
    clip.width = static_cast<unsigned short>(std::max(0, rect.width - 4));
    clip.height = static_cast<unsigned short>(std::max(0, rect.height - 4));
    XSetClipRectangles(display, gc, 0, 0, &clip, 1, Unsorted);

    for (size_t index = 0; index < items_.size(); ++index, y += rowHeight) {
        if (y < rect.y + 8) {
            continue;
        }
        if (y >= rect.y + rect.height - 4) {
            break;
        }
        if (static_cast<int>(index) == selected_ || static_cast<int>(index) == hoverIndex_) {
            XSetForeground(display, gc, Neu_Pixel(display, static_cast<int>(index) == selected_ ? theme.pressed : theme.hover));
            XFillRectangle(display, drawable, gc, rect.x + 4, y - 15, rect.width - 18, rowHeight);
        }
        drawTextInRect(display,
                       drawable,
                       gc,
                       theme,
                       items_[index],
                       Neu_Rect{rect.x + 10, y - 15, contentRight - rect.x - 14, rowHeight},
                       Neu_TextLayoutOptions{false, true, Neu_TextAlign::Left, rowHeight, 0});
    }
    XSetClipMask(display, gc, None);
    drawScrollbars(display, drawable, gc, theme);
}

void Neu_Listbox::handleXEvent(XEvent& event)
{
    const auto rect = bounds();
    if (event.type == MotionNotify) {
        int hover = -1;
        if (contains(event.xmotion.x, event.xmotion.y)) {
            hover = (event.xmotion.y - rect.y + scrollY()) / 22;
            if (hover < 0 || hover >= static_cast<int>(items_.size())) {
                hover = -1;
            }
        }
        if (hover != hoverIndex_) {
            hoverIndex_ = hover;
            requestRedraw();
        }
    }

    if (event.type == LeaveNotify && hoverIndex_ != -1) {
        hoverIndex_ = -1;
        requestRedraw();
    }

    if (event.type == ButtonRelease && contains(event.xbutton.x, event.xbutton.y)) {
        const int index = (event.xbutton.y - rect.y + scrollY()) / 22;
        if (index >= 0 && index < static_cast<int>(items_.size())) {
            selected_ = index;
            if (callbacks_.onSelectionChanged) {
                callbacks_.onSelectionChanged(this, index, 0, items_[static_cast<size_t>(index)].c_str(), callbacks_.userData);
            }
            requestRedraw();
        }
    }

    Neu_Control::handleXEvent(event);
}

} // namespace neutrino
