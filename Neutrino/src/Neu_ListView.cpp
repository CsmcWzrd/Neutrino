#include "Neutrino/Neutrino.hpp"

namespace neutrino {

namespace {
constexpr int kRowHeight = 22;
constexpr int kColumnWidth = 150;
constexpr int kHeaderHeight = 8;
}

Neu_TypedValue Neu_ListView::cellValue(size_t row, size_t column) const
{
    if (!model_ || row >= model_->size() || column >= (*model_)[row].size()) {
        return {};
    }

    return Neu_TypeInterpreter::interpret((*model_)[row][column]);
}

void Neu_ListView::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(display, drawable, gc, theme);
    if (!model_) {
        return;
    }

    const auto rect = bounds();
    size_t maxCols = 0;
    for (const auto& rowData : *model_) {
        maxCols = std::max(maxCols, rowData.size());
    }
    autoScroll_ = autoScroll_ || static_cast<int>(model_->size()) * kRowHeight > rect.height || static_cast<int>(maxCols) * kColumnWidth > rect.width;
    setVirtualSize(std::max(rect.width, static_cast<int>(maxCols) * kColumnWidth + 20),
                   std::max(rect.height, static_cast<int>(model_->size()) * kRowHeight + 16));

    XRectangle clip{};
    clip.x = static_cast<short>(rect.x + 2);
    clip.y = static_cast<short>(rect.y + 2);
    clip.width = static_cast<unsigned short>(std::max(0, rect.width - 14));
    clip.height = static_cast<unsigned short>(std::max(0, rect.height - 14));
    XSetClipRectangles(display, gc, 0, 0, &clip, 1, Unsorted);

    int y = rect.y + kHeaderHeight + kRowHeight - scrollY();
    for (size_t row = 0; row < model_->size(); ++row, y += kRowHeight) {
        if (y < rect.y + kRowHeight) {
            continue;
        }
        if (y >= rect.y + rect.height - 6) {
            break;
        }
        int x = rect.x + 8 - scrollX();
        const bool rowSelected = static_cast<int>(row) == selectedRow_;
        const bool rowHovered = static_cast<int>(row) == hoverRow_;
        if (rowSelected || rowHovered) {
            XSetForeground(display, gc, Neu_Pixel(display, rowSelected ? theme.pressed : theme.hover));
            XFillRectangle(display, drawable, gc, rect.x + 4, y - 16, rect.width - 18, kRowHeight);
        }

        for (size_t column = 0; column < (*model_)[row].size(); ++column, x += kColumnWidth) {
            if (x >= rect.x + rect.width - 8) {
                break;
            }
            if (x + kColumnWidth < rect.x + 4) {
                continue;
            }
            const bool cellSelected = static_cast<int>(row) == selectedRow_ && static_cast<int>(column) == selectedCol_;
            const bool cellHovered = static_cast<int>(row) == hoverRow_ && static_cast<int>(column) == hoverCol_;
            if (cellSelected || cellHovered) {
                XSetForeground(display, gc, Neu_Pixel(display, cellSelected ? theme.pressed : theme.hover));
                XFillRectangle(display, drawable, gc, x, y - 16, kColumnWidth - 4, kRowHeight);
            }
            XSetForeground(display, gc, Neu_Pixel(display, theme.border));
            XDrawRectangle(display, drawable, gc, x - 1, y - 16, kColumnWidth - 3, kRowHeight);
            drawTextInRect(display,
                           drawable,
                           gc,
                           theme,
                           (*model_)[row][column],
                           Neu_Rect{x + 4, y - 15, kColumnWidth - 10, kRowHeight},
                           Neu_TextLayoutOptions{false, true, Neu_TextAlign::Left, kRowHeight, 0});
        }
    }
    XSetClipMask(display, gc, None);
    drawScrollbars(display, drawable, gc, theme);
}

void Neu_ListView::handleXEvent(XEvent& event)
{
    const auto rect = bounds();
    if (event.type == MotionNotify) {
        int row = -1;
        int col = -1;
        if (contains(event.xmotion.x, event.xmotion.y) && model_) {
            row = (event.xmotion.y - rect.y + scrollY() - kHeaderHeight) / kRowHeight;
            col = (event.xmotion.x - rect.x + scrollX() - 8) / kColumnWidth;
            if (row < 0 || row >= static_cast<int>(model_->size())) {
                row = -1;
                col = -1;
            } else if (col < 0 || col >= static_cast<int>((*model_)[static_cast<size_t>(row)].size())) {
                col = -1;
            }
        }
        if (row != hoverRow_ || col != hoverCol_) {
            hoverRow_ = row;
            hoverCol_ = col;
            requestRedraw();
        }
    }

    if (event.type == LeaveNotify && (hoverRow_ != -1 || hoverCol_ != -1)) {
        hoverRow_ = -1;
        hoverCol_ = -1;
        requestRedraw();
    }

    if (event.type == ButtonRelease && contains(event.xbutton.x, event.xbutton.y) && model_) {
        const int row = (event.xbutton.y - rect.y + scrollY() - kHeaderHeight) / kRowHeight;
        const int column = (event.xbutton.x - rect.x + scrollX() - 8) / kColumnWidth;

        if (row >= 0
            && row < static_cast<int>(model_->size())
            && column >= 0
            && column < static_cast<int>((*model_)[static_cast<size_t>(row)].size())) {
            selectedRow_ = row;
            selectedCol_ = column;

            if (callbacks_.onSelectionChanged) {
                callbacks_.onSelectionChanged(this,
                                              row,
                                              column,
                                              (*model_)[static_cast<size_t>(row)][static_cast<size_t>(column)].c_str(),
                                              callbacks_.userData);
            }

            requestRedraw();
        }
    }

    Neu_Control::handleXEvent(event);
}

} // namespace neutrino
