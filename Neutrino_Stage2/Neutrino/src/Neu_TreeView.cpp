#include "Neutrino/Neutrino.hpp"
#include <map>
#include <set>

namespace neutrino {

namespace {

struct TreeRowInfo {
    size_t modelRow{0};
    int depth{0};
    std::string label;
    std::string path;
    bool hasChildren{false};
};

static std::vector<std::string> rowPath(const std::vector<std::string>& row)
{
    std::vector<std::string> path;
    for (const auto& cell : row) {
        if (!cell.empty()) {
            path.push_back(cell);
        }
    }
    return path;
}

static std::string joinPath(const std::vector<std::string>& path, size_t count)
{
    std::string result;
    for (size_t index = 0; index < count && index < path.size(); ++index) {
        if (!result.empty()) {
            result += '/';
        }
        result += path[index];
    }
    return result;
}

static bool hasCollapsedAncestor(const std::vector<std::string>& path, const std::set<std::string>& collapsedPaths)
{
    if (path.empty()) {
        return false;
    }
    for (size_t depth = 1; depth < path.size(); ++depth) {
        if (collapsedPaths.count(joinPath(path, depth)) != 0U) {
            return true;
        }
    }
    return false;
}

static std::vector<TreeRowInfo> buildRows(const Neu_StringTable* model, const std::set<std::string>& collapsedPaths)
{
    std::vector<TreeRowInfo> rows;
    if (!model) {
        return rows;
    }

    std::map<std::string, bool> hasChild;
    std::vector<std::vector<std::string>> paths;
    paths.reserve(model->size());

    for (const auto& sourceRow : *model) {
        auto path = rowPath(sourceRow);
        if (!path.empty()) {
            for (size_t depth = 1; depth < path.size(); ++depth) {
                hasChild[joinPath(path, depth)] = true;
            }
        }
        paths.push_back(std::move(path));
    }

    for (size_t row = 0; row < paths.size(); ++row) {
        const auto& path = paths[row];
        if (path.empty() || hasCollapsedAncestor(path, collapsedPaths)) {
            continue;
        }
        const std::string pathKey = joinPath(path, path.size());
        rows.push_back({row,
                        static_cast<int>(path.size() - 1),
                        path.back(),
                        pathKey,
                        hasChild.count(pathKey) != 0U});
    }
    return rows;
}

static void applyVisibleSelection(std::set<int>& rows, int& selected, int& anchor, int row, bool ctrl, bool shift, bool multi)
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

void Neu_TreeView::expandAll()
{
    collapsedPaths_.clear();
    requestRedraw();
}

void Neu_TreeView::collapseAll()
{
    collapsedPaths_.clear();
    if (!model()) {
        return;
    }
    for (const auto& row : *model()) {
        const auto path = rowPath(row);
        if (!path.empty()) {
            for (size_t depth = 1; depth <= path.size(); ++depth) {
                collapsedPaths_.insert(joinPath(path, depth));
            }
        }
    }
    requestRedraw();
}

void Neu_TreeView::toggleNodePath(const std::string& path)
{
    const auto existing = collapsedPaths_.find(path);
    if (existing == collapsedPaths_.end()) {
        collapsedPaths_.insert(path);
    } else {
        collapsedPaths_.erase(existing);
    }
    requestRedraw();
}

bool Neu_TreeView::isPathCollapsed(const std::string& path) const
{
    return collapsedPaths_.count(path) != 0U;
}

void Neu_TreeView::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(display, drawable, gc, theme);
    if (!model()) {
        return;
    }

    const auto rect = bounds();
    constexpr int rowHeight = 22;
    const auto rows = buildRows(model(), collapsedPaths_);
    int maxLabelWidth = rect.width;
    for (const auto& row : rows) {
        maxLabelWidth = std::max(maxLabelWidth, row.depth * 18 + measureTextWidth(display, drawable, gc, theme, row.label) + 48);
    }
    setAutoScroll(true);
    setVirtualSize(std::max(rect.width, maxLabelWidth), std::max(rect.height, static_cast<int>(rows.size()) * rowHeight + 12));

    XRectangle clip{static_cast<short>(rect.x + 4),
                    static_cast<short>(rect.y + 4),
                    static_cast<unsigned short>(std::max(1, rect.width - 16)),
                    static_cast<unsigned short>(std::max(1, rect.height - 16))};
    XSetClipRectangles(display, gc, 0, 0, &clip, 1, Unsorted);

    int y = rect.y + 20 - scrollY();
    for (size_t index = 0; index < rows.size(); ++index, y += rowHeight) {
        if (y < rect.y + 8) {
            continue;
        }
        if (y >= rect.y + rect.height - 6) {
            break;
        }

        const int visibleIndex = static_cast<int>(index);
        if (selectedVisibleRows_.count(visibleIndex) != 0U || visibleIndex == selectedVisibleRow_ || visibleIndex == hoveredVisibleRow_) {
            XSetForeground(display, gc, Neu_Pixel(display, (selectedVisibleRows_.count(visibleIndex) != 0U || visibleIndex == selectedVisibleRow_) ? theme.pressed : theme.hover));
            XFillRectangle(display, drawable, gc, rect.x + 4, y - 16, rect.width - 18, rowHeight);
        }

        const auto& row = rows[index];
        const int indent = row.depth * 18;
        const std::string indicator = row.hasChildren ? (isPathCollapsed(row.path) ? "+ " : "- ") : "  ";
        const int textX = rect.x + 8 + indent - scrollX();
        const int clipLeft = std::max(textX, rect.x + 6);
        const int clipRight = rect.x + rect.width - 14;
        if (clipRight <= clipLeft) {
            continue;
        }
        XRectangle textClip{static_cast<short>(clipLeft),
                            static_cast<short>(rect.y + 4),
                            static_cast<unsigned short>(std::max(1, clipRight - clipLeft)),
                            static_cast<unsigned short>(std::max(1, rect.height - 14))};
        XSetClipRectangles(display, gc, 0, 0, &textClip, 1, Unsorted);
        const int drawX = std::max(textX, clipLeft + 2);
        const int maxTextWidth = std::max(1, clipRight - drawX - 2);
        drawText(display,
                 drawable,
                 gc,
                 theme,
                 truncateTextToWidth(display, drawable, gc, theme, indicator + row.label, maxTextWidth),
                 drawX,
                 y);
        XSetClipRectangles(display, gc, 0, 0, &clip, 1, Unsorted);
    }
    XSetClipMask(display, gc, None);
    drawScrollbars(display, drawable, gc, theme);
    drawHintPopup(display, drawable, gc, theme);
}

void Neu_TreeView::handleXEvent(XEvent& event)
{
    if (event.type == MotionNotify && contains(event.xmotion.x, event.xmotion.y) && model()) {
        const auto rect = bounds();
        const int visibleRow = (event.xmotion.y - rect.y + scrollY()) / 22;
        const auto rows = buildRows(model(), collapsedPaths_);
        const int validRow = (visibleRow >= 0 && visibleRow < static_cast<int>(rows.size())) ? visibleRow : -1;
        if (validRow != hoveredVisibleRow_) {
            hoveredVisibleRow_ = validRow;
            requestRedraw();
        }
    }

    if (event.type == LeaveNotify) {
        if (hoveredVisibleRow_ != -1) {
            hoveredVisibleRow_ = -1;
            requestRedraw();
        }
    }

    if (event.type == ButtonRelease && contains(event.xbutton.x, event.xbutton.y) && model()) {
        const auto rect = bounds();
        const int visibleRow = (event.xbutton.y - rect.y + scrollY()) / 22;
        const auto rows = buildRows(model(), collapsedPaths_);

        if (visibleRow >= 0 && visibleRow < static_cast<int>(rows.size())) {
            const bool ctrl = (event.xbutton.state & ControlMask) != 0U;
            const bool shift = (event.xbutton.state & ShiftMask) != 0U;
            applyVisibleSelection(selectedVisibleRows_, selectedVisibleRow_, anchorVisibleRow_, visibleRow, ctrl, shift, true);
            const auto& row = rows[static_cast<size_t>(visibleRow)];
            if (row.hasChildren && !ctrl && !shift) {
                toggleNodePath(row.path);
            }
            if (callbacks_.onSelectionChanged) {
                callbacks_.onSelectionChanged(this,
                                              static_cast<int>(row.modelRow),
                                              0,
                                              row.label.c_str(),
                                              callbacks_.userData);
            }
            requestRedraw();
        }
    }

    Neu_Control::handleXEvent(event);
}

} // namespace neutrino
