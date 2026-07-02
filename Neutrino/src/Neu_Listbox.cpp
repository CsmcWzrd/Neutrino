#include "Neutrino/Neutrino.hpp"

namespace neutrino {

namespace {

static void applyRowSelection(std::set<int>& rows, int& selected, int& anchor, int row, bool ctrl, bool shift, bool multi)
{
    if (!multi) {
        rows.clear();
        rows.insert(row);
        selected = row;
        anchor = row;
        return;
    }
    if (shift && anchor >= 0) {
        rows.clear();
        const int lo = std::min(anchor, row);
        const int hi = std::max(anchor, row);
        for (int i = lo; i <= hi; ++i) {
            rows.insert(i);
        }
        selected = row;
        return;
    }
    if (ctrl) {
        if (rows.count(row)) {
            rows.erase(row);
            if (selected == row) {
                selected = rows.empty() ? -1 : *rows.rbegin();
            }
        } else {
            rows.insert(row);
            selected = row;
        }
        anchor = row;
        return;
    }
    rows.clear();
    rows.insert(row);
    selected = row;
    anchor = row;
}

} // namespace

void Neu_Listbox::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(display, drawable, gc, theme);
    const auto rect = bounds();
    const int rowHeight = 20;
    setAutoScroll(true);
    setVirtualSize(rect.width, std::max(rect.height, static_cast<int>(items_.size()) * rowHeight + 8));

    XRectangle clip{static_cast<short>(rect.x + 4),
                    static_cast<short>(rect.y + 4),
                    static_cast<unsigned short>(std::max(1, rect.width - 16)),
                    static_cast<unsigned short>(std::max(1, rect.height - 16))};
    XSetClipRectangles(display, gc, 0, 0, &clip, 1, Unsorted);

    int y = rect.y + 18 - scrollY();
    for (size_t index = 0; index < items_.size(); ++index, y += rowHeight) {
        if (y < rect.y + 8) {
            continue;
        }
        if (y >= rect.y + rect.height - 6) {
            break;
        }
        const int row = static_cast<int>(index);
        if (selectedIndices_.count(row) != 0U || row == selected_ || row == hoveredIndex_) {
            XSetForeground(display, gc, Neu_Pixel(display, (selectedIndices_.count(row) != 0U || row == selected_) ? theme.pressed : theme.hover));
            XFillRectangle(display, drawable, gc, rect.x + 4, y - 14, rect.width - 18, rowHeight);
        }
        drawText(display,
                 drawable,
                 gc,
                 theme,
                 truncateTextToWidth(display, drawable, gc, theme, items_[index], rect.width - 26),
                 rect.x + 8,
                 y);
    }
    XSetClipMask(display, gc, None);
    drawScrollbars(display, drawable, gc, theme);
    drawHintPopup(display, drawable, gc, theme);
}

void Neu_Listbox::handleXEvent(XEvent& event)
{
    if (event.type == MotionNotify && contains(event.xmotion.x, event.xmotion.y)) {
        const auto rect = bounds();
        const int row = (event.xmotion.y - rect.y + scrollY()) / 20;
        const int newHover = (row >= 0 && row < static_cast<int>(items_.size())) ? row : -1;
        if (newHover != hoveredIndex_) {
            hoveredIndex_ = newHover;
            requestRedraw();
        }
    }

    if (event.type == LeaveNotify && hoveredIndex_ != -1) {
        hoveredIndex_ = -1;
        requestRedraw();
    }

    if (event.type == ButtonRelease && contains(event.xbutton.x, event.xbutton.y)) {
        const auto rect = bounds();
        const int row = (event.xbutton.y - rect.y + scrollY()) / 20;
        if (row >= 0 && row < static_cast<int>(items_.size())) {
            const bool ctrl = (event.xbutton.state & ControlMask) != 0U;
            const bool shift = (event.xbutton.state & ShiftMask) != 0U;
            applyRowSelection(selectedIndices_, selected_, anchorIndex_, row, ctrl, shift, true);
            if (callbacks_.onSelectionChanged) {
                callbacks_.onSelectionChanged(this, row, 0, items_[static_cast<size_t>(row)].c_str(), callbacks_.userData);
            }
            requestRedraw();
        }
    }
    Neu_Control::handleXEvent(event);
}

} // namespace neutrino
