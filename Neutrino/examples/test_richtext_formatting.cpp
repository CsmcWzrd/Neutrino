#include "Neutrino/Neutrino.hpp"
#include <iostream>
using namespace neutrino;

static Neu_RichTextFragment frag(const std::string& text, uint32_t style, int heading = 0)
{
    Neu_RichTextFragment f;
    f.text = text;
    f.style = style;
    f.headingLevel = heading;
    return f;
}

int main()
{
    Neu_Application app;
    if (!app.open()) {
        std::cerr << "Unable to open display\n";
        return 1;
    }
    Neu_Window window(app, 900, 560, "Neutrino Test - Rich Text Formatting");
    window.create();
    auto rich = std::make_shared<Neu_RichTextCode>(Neu_Layout{24, 24, 820, 430});
    rich->setWordWrap(true);
    rich->setText("// Editable/read-only rich text code area with toolbar\nclass Demo {\npublic:\n    void run();\n};\n");
    rich->addTextFragment(frag("Bold ", Neu_TextStyle_Bold));
    rich->addTextFragment(frag("Italic ", Neu_TextStyle_Italic));
    rich->addTextFragment(frag("Underline ", Neu_TextStyle_Underline));
    rich->addTextFragment(frag("Strike ", Neu_TextStyle_Strikethrough));
    rich->addTextFragment(frag("DoubleStrike ", Neu_TextStyle_DoubleStrikethrough));
    rich->addTextFragment(frag("Mono ", Neu_TextStyle_Monospaced));
    Neu_RichTextFragment h = frag("Heading 1-7 API", Neu_TextStyle_Bold, 1);
    h.hasHighlight = true;
    h.highlightColor = {255, 245, 120, 255};
    rich->addTextFragment(h);
    window.add(rich);
    window.show();
    app.run();
    return 0;
}
