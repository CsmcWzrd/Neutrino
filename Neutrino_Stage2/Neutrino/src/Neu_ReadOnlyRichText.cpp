#include "Neutrino/Neutrino.hpp"

namespace neutrino {

namespace {

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

static int estimateLabelWidth(const std::string& text)
{
    return std::max(60, static_cast<int>(text.size()) * 8 + 30);
}

} // namespace

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

void Neu_ReadOnlyRichText::crlf()
{
    contentCursorX_ = 12;
    contentCursorY_ += 30 + labelLineSpacing_;
    appendNextInline_ = false;
}

void Neu_ReadOnlyRichText::addLabel(const std::string& text)
{
    const std::string clean = unescapeHashes(text);
    const int availableWidth = std::max(40, layout().width - 36);
    if (!appendNextInline_ || contentCursorX_ <= 12) {
        contentCursorX_ = 12;
    }
    int width = std::min(availableWidth - std::max(0, contentCursorX_ - 12), estimateLabelWidth(clean));
    if (width < 40 || contentCursorX_ + width > layout().width - 12) {
        contentCursorX_ = 12;
        contentCursorY_ += 30 + labelLineSpacing_;
        width = std::min(availableWidth, estimateLabelWidth(clean));
    }

    auto label = std::make_shared<Neu_Label>(Neu_Layout{layout().left + contentCursorX_,
                                                        layout().top + contentCursorY_,
                                                        width,
                                                        26,
                                                        1.0f,
                                                        0,
                                                        0});
    label->setText(clean);
    label->setWordWrap(false);
    label->setTextTruncation(true);
    if (!iconPaths_.empty()) {
        label->setIconBmp(iconPaths_[iconIndexForText(text)]);
    }
    add(label);

    if (appendNextInline_) {
        contentCursorX_ += width + labelSpacing_;
        appendNextInline_ = false;
    } else {
        contentCursorX_ = 12;
        contentCursorY_ += 30 + labelLineSpacing_;
    }
    setContentSize(layout().width, contentCursorY_ + 50);
}

void Neu_ReadOnlyRichText::addMultilineLabel(const std::string& text)
{
    const std::string clean = unescapeHashes(text);
    const int availableWidth = std::max(40, layout().width - 36);
    if (!appendNextInline_) {
        contentCursorX_ = 12;
    }
    if (contentCursorX_ + availableWidth > layout().width - 12) {
        contentCursorX_ = 12;
        contentCursorY_ += 30 + labelLineSpacing_;
    }

    int estimatedLines = 1;
    for (char ch : clean) {
        if (ch == '\n') {
            ++estimatedLines;
        }
    }
    estimatedLines += static_cast<int>(clean.size()) / std::max(1, availableWidth / 8);
    const int height = std::max(56, estimatedLines * (18 + labelLineSpacing_) + 12);

    auto label = std::make_shared<Neu_MultilineLabel>(Neu_Layout{layout().left + contentCursorX_,
                                                                 layout().top + contentCursorY_,
                                                                 availableWidth,
                                                                 height,
                                                                 1.0f,
                                                                 0,
                                                                 0});
    label->setText(clean);
    label->setWordWrap(true);
    label->setAutoScroll(true);
    if (!iconPaths_.empty()) {
        label->setIconBmp(iconPaths_[iconIndexForText(text)]);
    }
    add(label);

    if (appendNextInline_) {
        contentCursorX_ += availableWidth + labelSpacing_;
        appendNextInline_ = false;
    } else {
        contentCursorX_ = 12;
        contentCursorY_ += height + labelLineSpacing_;
    }
    setContentSize(layout().width, contentCursorY_ + 50);
}

} // namespace neutrino
