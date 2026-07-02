#include "Neutrino/Neutrino.hpp"
#include <iostream>
using namespace neutrino;

static Neu_RichTextFragment frag(const std::string& text, uint32_t style)
{
    Neu_RichTextFragment f;
    f.text = text;
    f.style = style;
    return f;
}

int main()
{
    Neu_Application app;
    if (!app.open()) {
        std::cerr << "Unable to open display\n";
        return 1;
    }
    Neu_Window window(app, 820, 520, "Neutrino Test - Read-only Rich Text Spacing");
    window.create();
    auto doc = std::make_shared<Neu_ReadOnlyRichText>(Neu_Layout{24, 24, 740, 420});
    doc->setAutoScroll(true);
    doc->setLabelSpacing(14);
    doc->setLabelLineSpacing(6);
    doc->setIconList({"assets/icons/sample_icon.bmp", "assets/icons/menu_icon.bmp", "assets/icons/save_icon.bmp"});
    doc->addLabel("# First label with icon 1");
    doc->no_crlf();
    doc->addLabel("## appended after no_crlf");
    doc->crlf();
    std::vector<Neu_RichTextFragment> fragments;
    fragments.push_back(frag("Bold fragment ", Neu_TextStyle_Bold));
    fragments.push_back(frag("underlined fragment", Neu_TextStyle_Underline));
    doc->addLabel(fragments);
    doc->addMultilineLabel("### Multiline read-only label uses spacing and wraps by default. Escaped hash appears as literal: \\#.");
    window.add(doc);
    window.show();
    app.run();
    return 0;
}
