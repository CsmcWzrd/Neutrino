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

static std::string fragmentsText(const std::vector<Neu_RichTextFragment>& fragments)
{
    std::string output;
    for (const auto& fragment : fragments) {
        output += fragment.text;
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
    const std::string cleaned = unescapeHashes(text);
    const int labelHeight = 26;
    const int maxWidth = std::max(40, layout().width - 36);
    const int estimatedWidth = std::min(maxWidth, std::max(60, static_cast<int>(cleaned.size()) * 8 + 28));
    if (!appendNextWithoutCrLf_) {
        cursorX_ = 12;
    }
    auto label = std::make_shared<Neu_Label>(Neu_Layout{layout().left + cursorX_, layout().top + cursorY_, estimatedWidth, labelHeight, 1.0f, 0, 0});
    label->setText(cleaned);
    label->setTruncateText(true);
    if (!iconPaths_.empty()) {
        label->setIconBmp(iconPaths_[iconIndexForText(text)]);
    }
    add(label);
    if (appendNextWithoutCrLf_) {
        cursorX_ += estimatedWidth + labelSpacing_;
    } else {
        cursorY_ += labelHeight + labelSpacing_;
    }
    setContentSize(layout().width, cursorY_ + labelHeight + labelSpacing_);
}

void Neu_ReadOnlyRichText::addLabel(const std::vector<Neu_RichTextFragment>& fragments)
{
    const std::string text = fragmentsText(fragments);
    const int labelHeight = 28;
    const int maxWidth = std::max(40, layout().width - 36);
    const int estimatedWidth = std::min(maxWidth, std::max(60, static_cast<int>(text.size()) * 8 + 28));
    if (!appendNextWithoutCrLf_) {
        cursorX_ = 12;
    }
    auto label = std::make_shared<Neu_Label>(Neu_Layout{layout().left + cursorX_, layout().top + cursorY_, estimatedWidth, labelHeight, 1.0f, 0, 0});
    label->clearRichTextFragments();
    for (const auto& fragment : fragments) {
        label->addTextFragment(fragment);
    }
    if (!iconPaths_.empty()) {
        label->setIconBmp(iconPaths_[iconIndexForText(text)]);
    }
    add(label);
    if (appendNextWithoutCrLf_) {
        cursorX_ += estimatedWidth + labelSpacing_;
    } else {
        cursorY_ += labelHeight + labelSpacing_;
    }
    setContentSize(layout().width, cursorY_ + labelHeight + labelSpacing_);
}

void Neu_ReadOnlyRichText::addMultilineLabel(const std::string& text)
{
    const std::string cleaned = unescapeHashes(text);
    if (!appendNextWithoutCrLf_) {
        cursorX_ = 12;
    }
    const int maxWidth = std::max(40, layout().width - 36 - cursorX_);
    int lines = 1;
    for (char ch : cleaned) {
        if (ch == '\n') {
            ++lines;
        }
    }
    lines = std::max(lines, static_cast<int>(cleaned.size()) / std::max(1, maxWidth / 7) + 1);
    const int labelHeight = std::max(42, lines * (18 + labelLineSpacing_) + 8);
    auto label = std::make_shared<Neu_MultilineLabel>(Neu_Layout{layout().left + cursorX_, layout().top + cursorY_, maxWidth, labelHeight, 1.0f, 0, 0});
    label->setText(cleaned);
    label->setAutoScroll(true);
    label->setWordWrap(true);
    if (!iconPaths_.empty()) {
        label->setIconBmp(iconPaths_[iconIndexForText(text)]);
    }
    add(label);
    if (appendNextWithoutCrLf_) {
        cursorX_ += maxWidth + labelSpacing_;
    } else {
        cursorY_ += labelHeight + labelSpacing_;
    }
    setContentSize(layout().width, cursorY_ + labelHeight + labelSpacing_);
}

} // namespace neutrino
