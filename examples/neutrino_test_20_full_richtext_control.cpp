#include "common/Neu_TestCommon.hpp"

using namespace neutrino;
using namespace neutrino_tests;

static const char* richtext_sample =
    "Neutrino full rich text editing test\n\n"
    "Select text with the mouse, Shift+Home/End/PageUp/PageDown, then use the toolbar.\n"
    "The toolbar buttons apply bold, italic, underline, strike, headings, monospace, "
    "font color, background color, highlight color, alignment, and word wrap.\n\n"
    "Cut/copy/paste: Ctrl+X, Ctrl+C, Ctrl+V.\n"
    "Undo/redo: Ctrl+Z, Alt+Backspace for undo, Ctrl+Y or Ctrl+Shift+Z for redo.\n\n"
    "Multiple     spaces and indentation are preserved:\n"
    "    one      two      three\n"
    "        nested indentation line\n";

int main(int argc, char** argv)
{
    Neu_Application app;
    if (!start_app(app, argc, argv)) {
        return 1;
    }

    Neu_Window win(app, 1120, 760, "Neutrino Test 20 - Full Rich Text Control");
    apply_test_window_defaults(win);
    if (!win.create()) {
        return 1;
    }

    auto status = add_status(win, "Full rich text editor: select text and use toolbar formatting, alignment, undo/redo, and clipboard shortcuts.");

    auto editor = std::make_shared<Neu_RichTextCode>(Neu_Layout{24, 74, 1060, 630, 1.0f, 1100, 700});
    editor->setLanguageName("Rich Text");
    editor->setReadOnly(false);
    editor->setToolbarVisible(true);
    editor->setWordWrap(true);
    editor->setDefaultFontColor(Neu_Color{232, 234, 237, 255});
    editor->setSketchHighlightColor(Neu_Color{255, 214, 102, 170});
    editor->setAutoScroll(true);
    editor->setText(richtext_sample);
    editor->setHintText("This focused test exercises full rich-text toolbar editing and keyboard text operations.");
    editor->setCallbacks(text_callbacks(status.get()));
    win.add(editor);

    win.show();
    app.run();
    return 0;
}
