#include "Neutrino/Neutrino.hpp"

namespace neutrino {

namespace {

static Neu_Color darkerListHeader(const Neu_Color& c)
{
    return Neu_Color{static_cast<uint8_t>(std::max(0, static_cast<int>(c.r) - 28)),
                     static_cast<uint8_t>(std::max(0, static_cast<int>(c.g) - 28)),
                     static_cast<uint8_t>(std::max(0, static_cast<int>(c.b) - 28)),
                     c.a};
}

constexpr int kRowHeight = 26;
constexpr int kMinColumnWidth = 48;
constexpr int kHeaderGrip = 6;

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

static size_t maxColumns(const Neu_StringTable* model)
{
    size_t cols = 0;
    if (model) {
        for (const auto& row : *model) {
            cols = std::max(cols, row.size());
        }
    }
    return cols;
}

static int defaultColumnWidth(int controlWidth)
{
    return std::max(kMinColumnWidth, controlWidth > 0 ? std::max(120, std::min(360, (controlWidth * 60) / 100)) : 160);
}

static Neu_Color darkerHeader(const Neu_Color& c)
{
    return Neu_Color{static_cast<uint8_t>(std::max(0, static_cast<int>(c.r) - 38)),
                     static_cast<uint8_t>(std::max(0, static_cast<int>(c.g) - 38)),
                     static_cast<uint8_t>(std::max(0, static_cast<int>(c.b) - 38)),
                     c.a};
}

static int externalColumnWidth(const Neu_ListView& view, size_t column, int controlWidth)
{
    const int explicitWidth = view.columnWidth(column);
    return explicitWidth > 0 ? explicitWidth : defaultColumnWidth(controlWidth);
}

static int columnAtX(const Neu_ListView& view, int localX, size_t columnCount, int controlWidth)
{
    int x = 0;
    for (size_t column = 0; column < columnCount; ++column) {
        const int width = externalColumnWidth(view, column, controlWidth);
        if (localX >= x && localX < x + width) {
            return static_cast<int>(column);
        }
        x += width;
    }
    return -1;
}

static int hitResizeColumn(const Neu_ListView& view, int localX, size_t columnCount, int controlWidth)
{
    int x = 0;
    for (size_t column = 0; column < columnCount; ++column) {
        x += externalColumnWidth(view, column, controlWidth);
        if (std::abs(localX - x) <= kHeaderGrip) {
            return static_cast<int>(column);
        }
    }
    return -1;
}

} // namespace

void Neu_ListView::setColumnWidths(const std::vector<int>& widths)
{
    columnWidths_.clear();
    columnWidths_.reserve(widths.size());
    for (int width : widths) {
        columnWidths_.push_back(std::max(kMinColumnWidth, width));
    }
    requestRedraw();
}

void Neu_ListView::setColumnWidth(size_t column, int width)
{
    if (columnWidths_.size() <= column) {
        columnWidths_.resize(column + 1, 0);
    }
    columnWidths_[column] = std::max(kMinColumnWidth, width);
    requestRedraw();
}

int Neu_ListView::columnWidth(size_t column) const
{
    if (column < columnWidths_.size() && columnWidths_[column] > 0) {
        return columnWidths_[column];
    }
    return 0;
}

int Neu_ListView::effectiveColumnWidth(size_t column, int controlWidth) const
{
    if (column < columnWidths_.size() && columnWidths_[column] > 0) {
        return std::max(kMinColumnWidth, columnWidths_[column]);
    }
    const int base = controlWidth > 0 ? std::max(120, std::min(360, (controlWidth * 60) / 100)) : 160;
    return std::max(kMinColumnWidth, base);
}

int Neu_ListView::totalColumnWidth(size_t columnCount, int controlWidth) const
{
    int total = 0;
    for (size_t i = 0; i < columnCount; ++i) {
        total += effectiveColumnWidth(i, controlWidth);
    }
    return total;
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
    const size_t cols = maxColumns(model_);
    const int headerH = std::max(18, headerHeight_);
    const int viewportLeft = rect.x + 4;
    const int viewportRight = rect.x + rect.width - 14;
    const int viewportWidth = std::max(1, viewportRight - viewportLeft);
    const int totalWidth = totalColumnWidth(cols, rect.width) + 36;
    const int virtualWidth = totalWidth > viewportWidth ? std::max(rect.width + 1, totalWidth) : rect.width;
    const int dataRows = std::max(0, static_cast<int>(model_->size()) - 1);
    setAutoScroll(true);
    setVirtualSize(virtualWidth, std::max(rect.height, headerH + dataRows * kRowHeight + 16));

    const int viewportTop = rect.y + 4;
    const int viewportBottom = rect.y + rect.height - 14;

    XRectangle fullClip{static_cast<short>(viewportLeft),
                        static_cast<short>(viewportTop),
                        static_cast<unsigned short>(std::max(1, viewportRight - viewportLeft)),
                        static_cast<unsigned short>(std::max(1, viewportBottom - viewportTop))};
    XSetClipRectangles(display, gc, 0, 0, &fullClip, 1, Unsorted);

    const int headerTop = viewportTop;
    const int headerBottom = std::min(viewportBottom, headerTop + headerH);
    Neu_DrawSmoothRoundedRect(display, drawable, gc,
                              darkerHeader(theme.glass),
                              theme.background,
                              viewportLeft, headerTop,
                              std::max(1, viewportRight - viewportLeft),
                              std::max(1, headerBottom - headerTop),
                              std::max(2, theme.radius - 2),
                              true,
                              theme.antiAliasSamples);

    int x = rect.x + 8 - scrollX();
    for (size_t column = 0; column < cols; ++column) {
        const int cw = effectiveColumnWidth(column, rect.width);
        const int cellLeft = x - 4;
        const int cellRight = x + cw - 4;
        if (cellRight >= viewportLeft && cellLeft < viewportRight) {
            const int drawLeft = std::max(cellLeft, viewportLeft);
            const int drawRight = std::min(cellRight, viewportRight);
            XSetForeground(display, gc, Neu_Pixel(display, theme.border));
            XDrawRectangle(display, drawable, gc, drawLeft, headerTop,
                           static_cast<unsigned int>(std::max(1, drawRight - drawLeft)),
                           static_cast<unsigned int>(std::max(1, headerH - 1)));
            if (!model_->empty() && column < (*model_)[0].size()) {
                XRectangle cellClip{static_cast<short>(drawLeft + 2),
                                    static_cast<short>(headerTop + 1),
                                    static_cast<unsigned short>(std::max(1, drawRight - drawLeft - 4)),
                                    static_cast<unsigned short>(std::max(1, headerH - 2))};
                XSetClipRectangles(display, gc, 0, 0, &cellClip, 1, Unsorted);
                const int visibleWidth = std::max(1, drawRight - drawLeft - 6);
                const std::string headerText = truncateTextToWidth(display, drawable, gc, theme, (*model_)[0][column], visibleWidth);
                drawTextColored(display, drawable, gc, theme, headerText, drawLeft + 4, headerTop + headerH - 7, theme.text, true);
                XSetClipRectangles(display, gc, 0, 0, &fullClip, 1, Unsorted);
            }
            XSetForeground(display, gc, Neu_Pixel(display, theme.accent));
            XDrawLine(display, drawable, gc, drawRight - 1, headerTop + 3, drawRight - 1, headerBottom - 3);
        }
        x += cw;
    }

    int y = rect.y + headerH + kRowHeight - scrollY();
    for (size_t row = 1; row < model_->size(); ++row, y += kRowHeight) {
        if (y < headerBottom + 2) {
            continue;
        }
        if (y >= viewportBottom + kRowHeight) {
            break;
        }

        const int rowIndex = static_cast<int>(row);
        if (selectedRows_.count(rowIndex) != 0U || rowIndex == selectedRow_ || rowIndex == hoveredRow_) {
            XSetForeground(display, gc, Neu_Pixel(display, (selectedRows_.count(rowIndex) != 0U || rowIndex == selectedRow_) ? theme.pressed : theme.hover));
            Neu_DrawSmoothRoundedRect(display, drawable, gc,
                                      (selectedRows_.count(rowIndex) != 0U || rowIndex == selectedRow_) ? theme.pressed : theme.hover,
                                      theme.background,
                                      viewportLeft, y - 16,
                                      std::max(1, viewportRight - viewportLeft),
                                      kRowHeight,
                                      std::max(2, theme.radius - 3),
                                      true,
                                      theme.antiAliasSamples);
        }

        x = rect.x + 8 - scrollX();
        for (size_t column = 0; column < (*model_)[row].size(); ++column) {
            const int cw = effectiveColumnWidth(column, rect.width);
            const int cellLeft = x - 4;
            const int cellRight = x + cw - 4;
            if (cellRight < viewportLeft) {
                x += cw;
                continue;
            }
            if (cellLeft >= viewportRight) {
                break;
            }

            const int drawLeft = std::max(cellLeft, viewportLeft);
            const int drawRight = std::min(cellRight, viewportRight);
            if (rowIndex == selectedRow_ && static_cast<int>(column) == selectedCol_) {
                XSetForeground(display, gc, Neu_Pixel(display, theme.hover));
                Neu_DrawSmoothRoundedRect(display, drawable, gc, theme.hover, theme.background,
                                          drawLeft, y - 16,
                                          std::max(1, drawRight - drawLeft),
                                          kRowHeight,
                                          std::max(2, theme.radius - 3),
                                          true,
                                          theme.antiAliasSamples);
            }

            XSetForeground(display, gc, Neu_Pixel(display, theme.border));
            XDrawRectangle(display, drawable, gc, drawLeft, y - 18,
                           static_cast<unsigned int>(std::max(1, drawRight - drawLeft)),
                           static_cast<unsigned int>(kRowHeight));

            const int clipLeft = std::max(x, viewportLeft + 2);
            const int clipRight = std::min(x + cw - 10, viewportRight - 2);
            if (clipRight > clipLeft) {
                XRectangle cellClip{static_cast<short>(clipLeft),
                                    static_cast<short>(std::max(y - 18, headerBottom)),
                                    static_cast<unsigned short>(std::max(1, clipRight - clipLeft)),
                                    static_cast<unsigned short>(std::max(1, std::min(y + 4, viewportBottom) - std::max(y - 18, headerBottom)))};
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
            x += cw;
        }
    }
    XSetClipMask(display, gc, None);
    drawScrollbars(display, drawable, gc, theme);
    drawHintPopup(display, drawable, gc, theme);
}

void Neu_ListView::handleXEvent(XEvent& event)
{
    if (event.type == MotionNotify && resizingColumn_ >= 0) {
        setColumnWidth(static_cast<size_t>(resizingColumn_), resizeStartWidth_ + event.xmotion.x - resizeStartX_);
        return;
    }
    if (event.type == ButtonRelease && resizingColumn_ >= 0) {
        resizingColumn_ = -1;
        return;
    }

    Neu_Control::handleXEvent(event);
    if (activeScrollDrag_ != 0 || !model_) {
        return;
    }

    const auto rect = bounds();
    const size_t cols = maxColumns(model_);
    const int headerH = std::max(18, headerHeight_);
    const int viewportLeft = rect.x + 4;
    const int viewportRight = rect.x + rect.width - 14;
    const int viewportTop = rect.y + 4;
    const int viewportBottom = rect.y + rect.height - 14;

    auto mouseX = [&]() { return event.type == MotionNotify ? event.xmotion.x : event.xbutton.x; };
    auto mouseY = [&]() { return event.type == MotionNotify ? event.xmotion.y : event.xbutton.y; };

    if ((event.type == ButtonPress || event.type == ButtonRelease || event.type == MotionNotify) && !contains(mouseX(), mouseY())) {
        if (event.type == LeaveNotify || event.type == MotionNotify) {
            hoveredRow_ = -1;
            hoveredCol_ = -1;
            requestRedraw();
        }
        return;
    }

    if ((event.type == ButtonPress || event.type == ButtonRelease || event.type == MotionNotify)
        && (mouseX() >= rect.x + rect.width - 14 || mouseY() >= rect.y + rect.height - 14)) {
        return;
    }

    if (event.type == ButtonPress && event.xbutton.button == Button1 && headerResizable_) {
        if (event.xbutton.y >= viewportTop && event.xbutton.y < viewportTop + headerH) {
            const int localX = event.xbutton.x - (rect.x + 8) + scrollX();
            const int resizeCol = hitResizeColumn(*this, localX, cols, rect.width);
            if (resizeCol >= 0) {
                resizingColumn_ = resizeCol;
                resizeStartX_ = event.xbutton.x;
                resizeStartWidth_ = effectiveColumnWidth(static_cast<size_t>(resizeCol), rect.width);
                return;
            }
        }
    }

    if (event.type == MotionNotify && model_) {
        const int mx = event.xmotion.x;
        const int my = event.xmotion.y;
        if (mx >= viewportLeft && mx < viewportRight && my >= viewportTop + headerH && my < viewportBottom) {
            const int row = (my - (rect.y + headerH) + scrollY()) / kRowHeight + 1;
            const int column = columnAtX(*this, mx - (rect.x + 8) + scrollX(), cols, rect.width);
            const int validRow = (row >= 1 && row < static_cast<int>(model_->size())) ? row : -1;
            if (validRow != hoveredRow_ || column != hoveredCol_) {
                hoveredRow_ = validRow;
                hoveredCol_ = column;
                requestRedraw();
            }
        } else if (hoveredRow_ != -1 || hoveredCol_ != -1) {
            hoveredRow_ = -1;
            hoveredCol_ = -1;
            requestRedraw();
        }
        return;
    }

    if (event.type == LeaveNotify) {
        if (hoveredRow_ != -1 || hoveredCol_ != -1) {
            hoveredRow_ = -1;
            hoveredCol_ = -1;
            requestRedraw();
        }
        return;
    }

    if (event.type == ButtonRelease && event.xbutton.button == Button1) {
        const int row = (event.xbutton.y - (rect.y + headerH) + scrollY()) / kRowHeight + 1;
        const int column = columnAtX(*this, event.xbutton.x - (rect.x + 8) + scrollX(), cols, rect.width);

        if (row >= 1
            && row < static_cast<int>(model_->size())
            && column >= 0
            && column < static_cast<int>((*model_)[static_cast<size_t>(row)].size())) {
            const bool ctrl = (event.xbutton.state & ControlMask) != 0U;
            const bool shift = (event.xbutton.state & ShiftMask) != 0U;
            applyRowSelection(selectedRows_, selectedRow_, anchorRow_, row, ctrl, shift, multiSelect_);
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
}

} // namespace neutrino
