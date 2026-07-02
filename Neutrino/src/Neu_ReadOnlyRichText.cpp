#include "Neutrino/Neutrino.hpp"

namespace neutrino {

static size_t countUnescapedHashes(const std::string& text)
{
    size_t count = 0;
    bool escaped = false;
    for (char ch : text) {
        if (escaped) {
            escaped = false;
            continue;
        }
        if (ch == '\\') {
            escaped = true;
            continue;
        }
        if (ch == '#') {
            ++count;
        }
    }
    return count;
}

static std::string unescapeHashes(const std::string& text)
{
    std::string output;
    output.reserve(text.size());
    bool escaped = false;
    for (char ch : text) {
        if (escaped) {
            output.push_back(ch);
            escaped = false;
            continue;
        }
        if (ch == '\\') {
            escaped = true;
            continue;
        }
        output.push_back(ch);
    }
    if (escaped) {
        output.push_back('\\');
    }
    return output;
}

void Neu_ReadOnlyRichText::setIconList(const std::vector<std::string>& bmpIconPaths)
{
    iconPaths_ = bmpIconPaths;
}

size_t Neu_ReadOnlyRichText::iconIndexForText(const std::string& text) const
{
    if (iconPaths_.empty()) {
        return 0;
    }
    const size_t count = countUnescapedHashes(text);
    return std::min(count, iconPaths_.size() - 1);
}

void Neu_ReadOnlyRichText::addLabel(const std::string& text)
{
    const int y = 12 + static_cast<int>(children().size()) * 34;
    auto label = std::make_shared<Neu_Label>(Neu_Layout{layout().left + 12, layout().top + y, layout().width - 36, 26, 1.0f, 0, 0});
    label->setText(unescapeHashes(text));
    if (!iconPaths_.empty()) {
        label->setIconBmp(iconPaths_[iconIndexForText(text)]);
    }
    add(label);
    setContentSize(layout().width, y + 50);
}

void Neu_ReadOnlyRichText::addMultilineLabel(const std::string& text)
{
    const int y = 12 + static_cast<int>(children().size()) * 62;
    auto label = std::make_shared<Neu_MultilineLabel>(Neu_Layout{layout().left + 12, layout().top + y, layout().width - 36, 56, 1.0f, 0, 0});
    label->setText(unescapeHashes(text));
    label->setAutoScroll(true);
    if (!iconPaths_.empty()) {
        label->setIconBmp(iconPaths_[iconIndexForText(text)]);
    }
    add(label);
    setContentSize(layout().width, y + 80);
}

} // namespace neutrino
