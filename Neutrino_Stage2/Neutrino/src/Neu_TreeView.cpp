#include "Neutrino/Neutrino.hpp"
#include <map>
#include <set>

namespace neutrino {

namespace {

constexpr int kTreeRowHeight = 26;
constexpr int kTreeHeaderHeight = 24;
constexpr int kHeaderGrip = 6;

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


static Neu_Color darkerTreeHeader(const Neu_Color& c)
{
    return Neu_Color{static_cast<uint8_t>(std::max(0, static_cast<int>(c.r) - 38)),
                     static_cast<uint8_t>(std::max(0, static_cast<int>(c.g) - 38)),
                     static_cast<uint8_t>(std::max(0, static_cast<int>(c.b) - 38)),
                     c.a};
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
    const auto rows = buildRows(model(), collapsedPaths_);
    int maxLabelWidth = treeColumnWidth_;
    for (const auto& row : rows) {
        maxLabelWidth = std::max(maxLabelWidth, row.depth * 18 + measureTextWidth(display, drawable, gc, theme, row.label) + 56);
    }
    treeColumnWidth_ = std::max(96, std::max(treeColumnWidth_, std::min(maxLabelWidth, std::max(180, rect.width * 70 / 100))));
    setAutoScroll(true);
    setVirtualSize(std::max(rect.width, std::max(maxLabelWidth + 40, treeColumnWidth_ + 40)),
                   std::max(rect.height, kTreeHeaderHeight + static_cast<int>(rows.size()) * kTreeRowHeight + 16));

    const int viewportLeft = rect.x + 4;
    const int viewportRight = rect.x + rect.width - 14;
    const int viewportTop = rect.y + 4;
    const int viewportBottom = rect.y + rect.height - 14;

    XRectangle clip{static_cast<short>(viewportLeft),
                    static_cast<short>(viewportTop),
                    static_cast<unsigned short>(std::max(1, viewportRight - viewportLeft)),
                    static_cast<unsigned short>(std::max(1, viewportBottom - viewportTop))};
    XSetClipRectangles(display, gc, 0, 0, &clip, 1, Unsorted);

    const int headerBottom = std::min(viewportBottom, viewportTop + kTreeHeaderHeight);
    Neu_DrawSmoothRoundedRect(display, drawable, gc,
                              darkerTreeHeader(theme.glass),
                              theme.background,
                              viewportLeft, viewportTop,
                              std::max(1, viewportRight - viewportLeft),
                              std::max(1, headerBottom - viewportTop),
                              std::max(2, theme.radius - 2),
                              true,
                              theme.antiAliasSamples);
    const int headerX = rect.x + 8 - scrollX();
    const int headerRight = headerX + treeColumnWidth_;
    XSetForeground(display, gc, Neu_Pixel(display, theme.border));
    XDrawRectangle(display, drawable, gc, std::max(viewportLeft, headerX - 4), viewportTop,
                   static_cast<unsigned int>(std::max(1, std::min(viewportRight, headerRight) - std::max(viewportLeft, headerX - 4))),
                   static_cast<unsigned int>(std::max(1, kTreeHeaderHeight - 1)));
    XSetForeground(display, gc, Neu_Pixel(display, theme.accent));
    XDrawLine(display, drawable, gc, std::min(viewportRight - 1, headerRight - 1), viewportTop + 3, std::min(viewportRight - 1, headerRight - 1), headerBottom - 3);
    drawTextColored(display, drawable, gc, theme, "Tree", std::max(viewportLeft + 4, headerX), viewportTop + kTreeHeaderHeight - 7, theme.text, true);

    int y = rect.y + kTreeHeaderHeight + kTreeRowHeight - scrollY();
    for (size_t index = 0; index < rows.size(); ++index, y += kTreeRowHeight) {
        if (y < headerBottom + 2) {
            continue;
        }
        if (y >= viewportBottom + kTreeRowHeight) {
            break;
        }

        const int visibleIndex = static_cast<int>(index);
        if (selectedVisibleRows_.count(visibleIndex) != 0U || visibleIndex == selectedVisibleRow_ || visibleIndex == hoveredVisibleRow_) {
            XSetForeground(display, gc, Neu_Pixel(display, (selectedVisibleRows_.count(visibleIndex) != 0U || visibleIndex == selectedVisibleRow_) ? theme.pressed : theme.hover));
            Neu_DrawSmoothRoundedRect(display, drawable, gc,
                                      (selectedVisibleRows_.count(visibleIndex) != 0U || visibleIndex == selectedVisibleRow_) ? theme.pressed : theme.hover,
                                      theme.background,
                                      viewportLeft, y - 16,
                                      std::max(1, viewportRight - viewportLeft),
                                      kTreeRowHeight,
                                      std::max(2, theme.radius - 3),
                                      true,
                                      theme.antiAliasSamples);
        }

        const auto& row = rows[index];
        const int indent = row.depth * 18;
        const std::string indicator = row.hasChildren ? (isPathCollapsed(row.path) ? "+ " : "- ") : "  ";
        const int textX = rect.x + 8 + indent - scrollX();
        const int columnRight = rect.x + 8 + treeColumnWidth_ - scrollX();
        const int clipLeft = std::max(textX, viewportLeft + 2);
        const int clipRight = std::min(columnRight, viewportRight - 2);
        if (clipRight <= clipLeft) {
            continue;
        }
        XRectangle textClip{static_cast<short>(clipLeft),
                            static_cast<short>(std::max(y - 18, headerBottom)),
                            static_cast<unsigned short>(std::max(1, clipRight - clipLeft)),
                            static_cast<unsigned short>(std::max(1, std::min(y + 4, viewportBottom) - std::max(y - 18, headerBottom)))};
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
    if (event.type == MotionNotify && headerResizeActive_) {
        setTreeColumnWidth(headerResizeStartWidth_ + event.xmotion.x - headerResizeStartX_);
        return;
    }
    if (event.type == ButtonRelease && headerResizeActive_) {
        headerResizeActive_ = false;
        return;
    }

    Neu_Control::handleXEvent(event);
    if (activeScrollDrag_ != 0 || !model()) {
        return;
    }

    const auto rect = bounds();
    const int viewportLeft = rect.x + 4;
    const int viewportRight = rect.x + rect.width - 14;
    const int viewportTop = rect.y + 4;
    const int viewportBottom = rect.y + rect.height - 14;

    auto mx = [&]() { return event.type == MotionNotify ? event.xmotion.x : event.xbutton.x; };
    auto my = [&]() { return event.type == MotionNotify ? event.xmotion.y : event.xbutton.y; };

    if ((event.type == ButtonPress || event.type == ButtonRelease || event.type == MotionNotify) && !contains(mx(), my())) {
        if (hoveredVisibleRow_ != -1) {
            hoveredVisibleRow_ = -1;
            requestRedraw();
        }
        return;
    }

    if (event.type == ButtonPress && event.xbutton.button == Button1) {
        const int boundaryX = rect.x + 8 + treeColumnWidth_ - scrollX();
        if (event.xbutton.y >= viewportTop && event.xbutton.y < viewportTop + kTreeHeaderHeight
            && std::abs(event.xbutton.x - boundaryX) <= kHeaderGrip) {
            headerResizeActive_ = true;
            headerResizeStartX_ = event.xbutton.x;
            headerResizeStartWidth_ = treeColumnWidth_;
            return;
        }
    }

    const auto rows = buildRows(model(), collapsedPaths_);
    if (event.type == MotionNotify) {
        if (mx() >= viewportLeft && mx() < viewportRight && my() >= viewportTop + kTreeHeaderHeight && my() < viewportBottom) {
            const int visibleRow = (my() - (rect.y + kTreeHeaderHeight) + scrollY()) / kTreeRowHeight;
            const int validRow = (visibleRow >= 0 && visibleRow < static_cast<int>(rows.size())) ? visibleRow : -1;
            if (validRow != hoveredVisibleRow_) {
                hoveredVisibleRow_ = validRow;
                requestRedraw();
            }
        } else if (hoveredVisibleRow_ != -1) {
            hoveredVisibleRow_ = -1;
            requestRedraw();
        }
        return;
    }

    if (event.type == LeaveNotify) {
        if (hoveredVisibleRow_ != -1) {
            hoveredVisibleRow_ = -1;
            requestRedraw();
        }
        return;
    }

    if (event.type == ButtonRelease && event.xbutton.button == Button1) {
        const int visibleRow = (event.xbutton.y - (rect.y + kTreeHeaderHeight) + scrollY()) / kTreeRowHeight;

        if (visibleRow >= 0 && visibleRow < static_cast<int>(rows.size())) {
            const bool ctrl = (event.xbutton.state & ControlMask) != 0U;
            const bool shift = (event.xbutton.state & ShiftMask) != 0U;
            applyVisibleSelection(selectedVisibleRows_, selectedVisibleRow_, anchorVisibleRow_, visibleRow, ctrl, shift, multiSelect_);
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
}

} // namespace neutrino
