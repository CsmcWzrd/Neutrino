#include "common/Neu_TestCommon.hpp"

using namespace neutrino;
using namespace neutrino_tests;

static const char* sample_code =
    "#include <iostream>\n"
    "#include <vector>\n\n"
    "class Widget {\n"
    "public:\n"
    "    void draw() {\n"
    "        std::cout << \"Neutrino code editor sample\" << std::endl;\n"
    "    }\n"
    "};\n\n"
    "int main() {\n"
    "    std::vector<int> values = {1, 2, 3};\n"
    "    for (int value : values) {\n"
    "        std::cout << value << std::endl;\n"
    "    }\n"
    "    return 0;\n"
    "}\n";

int main(int argc, char** argv)
{
    Neu_Application app;
    if (!start_app(app, argc, argv)) {
        return 1;
    }

    Neu_Window win(app, 980, 680, "Neutrino Test 08 - Coding Rich Text Control");
    apply_test_window_defaults(win);
    if (!win.create()) {
        return 1;
    }

    auto status = add_status(win, "Neu_RichTextCode displays code-oriented rich text with scroll support.");
    auto code = std::make_shared<Neu_RichTextCode>(Neu_Layout{35, 90, 880, 500, 1.0f, 940, 600});
    code->setLanguageName("C++17");
    code->setReadOnly(false);
    code->setToolbarVisible(true);
    code->setWordWrap(true);
    code->setDefaultFontColor(Neu_Color{25, 35, 45, 255});
    code->setSketchHighlightColor(Neu_Color{255, 240, 130, 160});
    code->setAutoScroll(true);
    code->setText(sample_code);
    code->setHintText("Simple coding rich-text component: line numbers, language name, keyword-line highlighting, and scroll offsets.");
    code->setCallbacks(text_callbacks(status.get()));
    win.add(code);

    win.show();
    app.run();
    return 0;
}
