#include "common/Neu_TestCommon.hpp"

using namespace neutrino;
using namespace neutrino_tests;

static const char* code_sample =
    "#include <iostream>\n"
    "#include <string>\n"
    "#include <vector>\n\n"
    "class NeutrinoSample {\n"
    "public:\n"
    "    void run() {\n"
    "        std::vector<std::string> words = {\"rich\", \"text\", \"code\"};\n"
    "        for (const auto& word : words) {\n"
    "            std::cout << word << std::endl;\n"
    "        }\n"
    "    }\n"
    "};\n\n"
    "int main() {\n"
    "    NeutrinoSample sample;\n"
    "    sample.run();\n"
    "    return 0;\n"
    "}\n";

int main(int argc, char** argv)
{
    Neu_Application app;
    if (!start_app(app, argc, argv)) {
        return 1;
    }

    Neu_Window win(app, 1180, 800, "Neutrino Test 21 - Full Rich Text Code Control");
    apply_test_window_defaults(win);
    if (!win.create()) {
        return 1;
    }

    auto status = add_status(win, "Full code editor test: toolbar formatting, code text, selection highlight, cut/copy/paste, undo/redo, and drag selection.");

    auto codeEditor = std::make_shared<Neu_RichTextCode>(Neu_Layout{24, 74, 1120, 670, 1.0f, 1160, 740});
    codeEditor->setLanguageName("C++17");
    codeEditor->setReadOnly(false);
    codeEditor->setToolbarVisible(true);
    codeEditor->setWordWrap(false);
    codeEditor->setDefaultFontName("Monospace");
    codeEditor->setDefaultFontColor(Neu_Color{232, 234, 237, 255});
    codeEditor->setSketchHighlightColor(Neu_Color{42, 112, 178, 125});
    codeEditor->setAutoScroll(true);
    codeEditor->setText(code_sample);
    codeEditor->setHintText("Focused full code control test. Try Backspace, Alt+Backspace undo, Ctrl+Y redo, and toolbar formatting on selected code.");
    codeEditor->setCallbacks(text_callbacks(status.get()));
    win.add(codeEditor);

    win.show();
    app.run();
    return 0;
}
