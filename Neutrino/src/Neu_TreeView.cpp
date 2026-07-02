#include "Neutrino/Neutrino.hpp"
#include <map>
#include <set>

namespace neutrino {

namespace {

constexpr int kTreeRowHeight = 22;

struct TreeRowInfo {
    size_t modelRow{0};
    int depth{0};
    std::string label;
    std::string path;
    bool hasChildren{false};
};

static bool isBlank(const std::string& text)
{
    return text.empty();
}

static std::vector<std::string> rowPath(const std::vector<std::string>& row)
{
    std::vector<std::string> path;
    for (const auto& cell : row) {
        if (!isBlank(cell)) {
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
    autoScroll_ = autoScroll_ || static_cast<int>(rows.size()) * kTreeRowHeight > rect.height;
    setVirtualSize(rect.width, std::max(rect.height, static_cast<int>(rows.size()) * kTreeRowHeight + 12));

    XRectangle clip{};
    clip.x = static_cast<short>(rect.x + 2);
    clip.y = static_cast<short>(rect.y + 2);
    clip.width = static_cast<unsigned short>(std::max(0, rect.width - 14));
    clip.height = static_cast<unsigned short>(std::max(0, rect.height - 14));
    XSetClipRectangles(display, gc, 0, 0, &clip, 1, Unsorted);

    int y = rect.y + 20 - scrollY();
    for (size_t index = 0; index < rows.size(); ++index, y += kTreeRowHeight) {
        if (y < rect.y + 8) {
            continue;
        }
        if (y >= rect.y + rect.height - 4) {
            break;
        }
        if (static_cast<int>(index) == selectedVisibleRow_ || static_cast<int>(index) == hoverVisibleRow_) {
            XSetForeground(display, gc, Neu_Pixel(display, static_cast<int>(index) == selectedVisibleRow_ ? theme.pressed : theme.hover));
            XFillRectangle(display, drawable, gc, rect.x + 4, y - 15, rect.width - 18, kTreeRowHeight);
        }
        const auto& row = rows[index];
        const int indent = row.depth * 18;
        const std::string indicator = row.hasChildren ? (isPathCollapsed(row.path) ? "+ " : "- ") : "  ";
        drawTextInRect(display,
                       drawable,
                       gc,
                       theme,
                       indicator + row.label,
                       Neu_Rect{rect.x + 8 + indent, y - 15, rect.width - 24 - indent, kTreeRowHeight},
                       Neu_TextLayoutOptions{false, true, Neu_TextAlign::Left, kTreeRowHeight, 0});
    }
    XSetClipMask(display, gc, None);
    drawScrollbars(display, drawable, gc, theme);
}

void Neu_TreeView::handleXEvent(XEvent& event)
{
    if (!model()) {
        Neu_Control::handleXEvent(event);
        return;
    }
    const auto rect = bounds();
    const auto rows = buildRows(model(), collapsedPaths_);

    if (event.type == MotionNotify) {
        int hover = -1;
        if (contains(event.xmotion.x, event.xmotion.y)) {
            hover = (event.xmotion.y - rect.y + scrollY()) / kTreeRowHeight;
            if (hover < 0 || hover >= static_cast<int>(rows.size())) {
                hover = -1;
            }
        }
        if (hover != hoverVisibleRow_) {
            hoverVisibleRow_ = hover;
            requestRedraw();
        }
    }

    if (event.type == LeaveNotify && hoverVisibleRow_ != -1) {
        hoverVisibleRow_ = -1;
        requestRedraw();
    }

    if (event.type == ButtonRelease && contains(event.xbutton.x, event.xbutton.y)) {
        const int visibleRow = (event.xbutton.y - rect.y + scrollY()) / kTreeRowHeight;
        if (visibleRow >= 0 && visibleRow < static_cast<int>(rows.size())) {
            const auto& row = rows[static_cast<size_t>(visibleRow)];
            selectedVisibleRow_ = visibleRow;
            if (row.hasChildren && event.xbutton.x <= rect.x + 30 + row.depth * 18) {
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
