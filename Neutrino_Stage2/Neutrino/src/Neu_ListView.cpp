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
    constexpr int rowHeight = 22;
    constexpr int columnWidth = 140;
    size_t maxCols = 0;
    for (const auto& rowData : *model_) {
        maxCols = std::max(maxCols, rowData.size());
    }
    setAutoScroll(true);
    setVirtualSize(std::max(rect.width, static_cast<int>(maxCols) * columnWidth + 20),
                   std::max(rect.height, static_cast<int>(model_->size()) * rowHeight + 12));

    XRectangle fullClip{static_cast<short>(rect.x + 4),
                        static_cast<short>(rect.y + 4),
                        static_cast<unsigned short>(std::max(1, rect.width - 16)),
                        static_cast<unsigned short>(std::max(1, rect.height - 16))};
    XSetClipRectangles(display, gc, 0, 0, &fullClip, 1, Unsorted);

    int y = rect.y + 20 - scrollY();
    for (size_t row = 0; row < model_->size(); ++row, y += rowHeight) {
        if (y < rect.y + 8) {
            continue;
        }
        if (y >= rect.y + rect.height - 6) {
            break;
        }

        const int rowIndex = static_cast<int>(row);
        if (selectedRows_.count(rowIndex) != 0U || rowIndex == selectedRow_ || rowIndex == hoveredRow_) {
            XSetForeground(display, gc, Neu_Pixel(display, (selectedRows_.count(rowIndex) != 0U || rowIndex == selectedRow_) ? theme.pressed : theme.hover));
            XFillRectangle(display, drawable, gc, rect.x + 4, y - 16, rect.width - 18, rowHeight);
        }

        int x = rect.x + 8 - scrollX();
        for (size_t column = 0; column < (*model_)[row].size(); ++column, x += columnWidth) {
            const int cellLeft = x - 4;
            const int cellRight = x + columnWidth - 4;
            if (cellRight < rect.x + 6) {
                continue;
            }
            if (cellLeft >= rect.x + rect.width - 12) {
                break;
            }

            if (rowIndex == selectedRow_ && static_cast<int>(column) == selectedCol_) {
                XSetForeground(display, gc, Neu_Pixel(display, theme.hover));
                XFillRectangle(display, drawable, gc, std::max(cellLeft, rect.x + 4), y - 16, std::max(1, std::min(cellRight, rect.x + rect.width - 14) - std::max(cellLeft, rect.x + 4)), rowHeight);
            }

            XSetForeground(display, gc, Neu_Pixel(display, theme.border));
            XDrawRectangle(display, drawable, gc, cellLeft, y - 18, columnWidth, rowHeight);

            const int clipLeft = std::max(x, rect.x + 6);
            const int clipRight = std::min(x + columnWidth - 10, rect.x + rect.width - 14);
            if (clipRight <= clipLeft) {
                continue;
            }
            XRectangle cellClip{static_cast<short>(clipLeft),
                                static_cast<short>(rect.y + 4),
                                static_cast<unsigned short>(std::max(1, clipRight - clipLeft)),
                                static_cast<unsigned short>(std::max(1, rect.height - 14))};
            XSetClipRectangles(display, gc, 0, 0, &cellClip, 1, Unsorted);
            const int drawX = std::max(x, clipLeft + 2);
            const int visibleWidth = std::max(1, clipRight - drawX - 2);
            const std::string cell = truncateTextToWidth(display,
                                                         drawable,
                                                         gc,
                                                         theme,
                                                         (*model_)[row][column],
                                                         visibleWidth);
            drawText(display, drawable, gc, theme, cell, drawX, y);
            XSetClipRectangles(display, gc, 0, 0, &fullClip, 1, Unsorted);
        }
    }
    XSetClipMask(display, gc, None);
    drawScrollbars(display, drawable, gc, theme);
    drawHintPopup(display, drawable, gc, theme);
}

void Neu_ListView::handleXEvent(XEvent& event)
{
    if (event.type == MotionNotify && contains(event.xmotion.x, event.xmotion.y) && model_) {
        const auto rect = bounds();
        const int row = (event.xmotion.y - rect.y + scrollY()) / 22;
        const int column = (event.xmotion.x - (rect.x + 8) + scrollX()) / 140;
        const int validRow = (row >= 0 && row < static_cast<int>(model_->size())) ? row : -1;
        if (validRow != hoveredRow_ || column != hoveredCol_) {
            hoveredRow_ = validRow;
            hoveredCol_ = column;
            requestRedraw();
        }
    }

    if (event.type == LeaveNotify) {
        if (hoveredRow_ != -1 || hoveredCol_ != -1) {
            hoveredRow_ = -1;
            hoveredCol_ = -1;
            requestRedraw();
        }
    }

    if (event.type == ButtonRelease && contains(event.xbutton.x, event.xbutton.y) && model_) {
        const auto rect = bounds();
        const int row = (event.xbutton.y - rect.y + scrollY()) / 22;
        const int column = (event.xbutton.x - (rect.x + 8) + scrollX()) / 140;

        if (row >= 0
            && row < static_cast<int>(model_->size())
            && column >= 0
            && column < static_cast<int>((*model_)[static_cast<size_t>(row)].size())) {
            const bool ctrl = (event.xbutton.state & ControlMask) != 0U;
            const bool shift = (event.xbutton.state & ShiftMask) != 0U;
            applyRowSelection(selectedRows_, selectedRow_, anchorRow_, row, ctrl, shift, true);
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
