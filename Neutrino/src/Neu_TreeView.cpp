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
    int y = rect.y + 20;

    for (const auto& row : rows) {
        if (y >= rect.y + rect.height) {
            break;
        }

        const int indent = row.depth * 18;
        const std::string indicator = row.hasChildren ? (isPathCollapsed(row.path) ? "+ " : "- ") : "  ";
        drawText(display, drawable, gc, theme, indicator + row.label, rect.x + 8 + indent, y);
        y += 20;
    }
}

void Neu_TreeView::handleXEvent(XEvent& event)
{
    if (event.type == ButtonRelease && contains(event.xbutton.x, event.xbutton.y) && model()) {
        const auto rect = bounds();
        const int visibleRow = (event.xbutton.y - rect.y) / 20;
        const auto rows = buildRows(model(), collapsedPaths_);

        if (visibleRow >= 0 && visibleRow < static_cast<int>(rows.size())) {
            const auto& row = rows[static_cast<size_t>(visibleRow)];

            if (row.hasChildren) {
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
