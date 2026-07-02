#include "common/Neu_TestCommon.hpp"

using namespace neutrino;
using namespace neutrino_tests;

int main(int argc, char** argv)
{
    Neu_Application app;
    if (!start_app(app, argc, argv)) {
        return 1;
    }

    Neu_Window win(app, 980, 660, "Neutrino Test 09 - Read-only Rich Text, Labels, Multiline Labels, Icon Lists");
    apply_test_window_defaults(win);
    if (!win.create()) {
        return 1;
    }

    add_status(win, "Read-only rich text contains label subcomponents and selects BMP icons from '#' count.");

    auto rich = std::make_shared<Neu_ReadOnlyRichText>(Neu_Layout{35, 90, 880, 480, 1.0f, 940, 560});
    rich->setAutoScroll(true);
    rich->setContentSize(820, 900);
    rich->setIconList({
        "assets/icons/sample_icon.bmp",
        "assets/icons/button_icon.bmp",
        "assets/icons/menu_icon.bmp",
        "assets/icons/save_icon.bmp"
    });
    rich->setLabelSpacing(12);
    rich->setLabelLineSpacing(8);
    rich->setHintText("Neu_ReadOnlyRichText adds Neu_Label and Neu_MultilineLabel subcomponents. The number of unescaped # characters chooses an icon from an STL vector.");
    rich->addLabel("# Section label uses icon index 1");
    rich->addMultilineLabel("## Multiline label uses icon index 2\nIt can hold documentation-style text.\nEscaped hash example: \\# displays a literal #.");
    rich->addLabel("### Third icon selection from three # characters");
    rich->addMultilineLabel("#### More content with four markers wraps to the available rich text area.\nThe container is read-only and scrollable when content exceeds the visible area.");
    rich->addLabel("No marker uses default label drawing");
    rich->addMultilineLabel("# Another label family element\nMore lines\nMore lines\nMore lines\nMore lines\nMore lines");
    win.add(rich);

    win.show();
    app.run();
    return 0;
}
