#include "Neutrino/Neutrino.hpp"

namespace neutrino {

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
    constexpr int rowHeight = 20;
    constexpr int columnWidth = 120;
    size_t maxCols = 0;
    for (const auto& rowData : *model_) {
        maxCols = std::max(maxCols, rowData.size());
    }
    setVirtualSize(std::max(rect.width, static_cast<int>(maxCols) * columnWidth + 20),
                   std::max(rect.height, static_cast<int>(model_->size()) * rowHeight + 10));
    int y = rect.y + 18 - scrollY();

    for (size_t row = 0; row < model_->size(); ++row, y += rowHeight) {
        if (y < rect.y + 8) {
            continue;
        }
        if (y >= rect.y + rect.height) {
            break;
        }
        int x = rect.x + 8 - scrollX();

        for (size_t column = 0; column < (*model_)[row].size() && x < rect.x + rect.width; ++column, x += columnWidth) {
            if (static_cast<int>(row) == selectedRow_ && static_cast<int>(column) == selectedCol_) {
                XSetForeground(display, gc, Neu_Pixel(display, theme.hover));
                XFillRectangle(display, drawable, gc, x - 2, y - 14, columnWidth - 4, rowHeight);
            }

            drawText(display, drawable, gc, theme, (*model_)[row][column], x, y);
        }
    }
    drawScrollbars(display, drawable, gc, theme);
}

void Neu_ListView::handleXEvent(XEvent& event)
{
    if (event.type == ButtonRelease && contains(event.xbutton.x, event.xbutton.y) && model_) {
        const auto rect = bounds();
        const int row = (event.xbutton.y - rect.y + scrollY()) / 20;
        const int column = (event.xbutton.x - rect.x + scrollX()) / 120;

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
