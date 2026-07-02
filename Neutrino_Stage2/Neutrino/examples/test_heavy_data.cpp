#include "Neutrino/Neutrino.hpp"
#include <memory>
#include <sstream>
#include <cstring>

using namespace neutrino;

static void closeApp(Neu_Window*, void*)
{
    if (Neu_Application::current()) {
        Neu_Application::current()->quit();
    }
}

static void clicked(Neu_Control* sender, void*)
{
    sender->setText("Clicked - callback via function pointer");
}

int main(int argc, char** argv)
{
    bool fastMode = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--fast") == 0 || std::strcmp(argv[i], "--vm") == 0) {
            fastMode = true;
        }
    }

    Neu_Application app;
    if (!app.open()) {
        return 1;
    }

    if (fastMode) {
        Neu_UseVirtualMachineFriendlyDefaults(true);
    }

    if (!fastMode) {
        Neu_SmoothGraphicsOptions options;
        options.enabled = true;
        options.backend = Neu_GraphicsBackend::SoftwareAntialias;
        options.supersample = 2;
        options.drawShadows = true;
        options.repaintOnMouseMove = true;
        Neu_SetSmoothGraphicsOptions(options);
    }

    Neu_Window window(app, 1220, 820, "Neutrino heavy data, rich text, scroll, image and progress demo");
    window.setTheme(Neu_Theme::BlueGlass());
    window.setOnClose(closeApp, nullptr);
    if (!window.create()) {
        return 1;
    }

    auto title = std::make_shared<Neu_Label>(Neu_Layout{20, 16, 520, 28, 1.0f, 0, 0});
    title->setText("Neutrino - heavy data auto-scroll and new controls");
    title->setIconBmp("assets/icons/menu_icon.bmp");
    window.add(title);

    auto list = std::make_shared<Neu_Listbox>(Neu_Layout{20, 60, 260, 280, 1.0f, 0, 0});
    std::vector<std::string> listItems;
    for (int i = 0; i < 250; ++i) {
        listItems.push_back("Auto-scroll listbox row " + std::to_string(i));
    }
    list->setItems(listItems);
    list->setAutoScroll(true);
    list->setHintText("This list contains 250 rows. Use the mouse wheel to verify automatic vertical scrolling.");
    window.add(list);

    static Neu_StringTable table;
    for (int r = 0; r < 220; ++r) {
        table.push_back({"row:" + std::to_string(r),
                         "int:" + std::to_string(r * 17),
                         "float:" + std::to_string(r * 0.125),
                         "hex:0x" + std::to_string(0x1000 + r),
                         (r % 2) ? "checkbox:1" : "checkbox:0",
                         "utf:sample row " + std::to_string(r)});
    }
    auto listView = std::make_shared<Neu_ListView>(Neu_Layout{300, 60, 520, 280, 1.0f, 0, 0});
    listView->bind(&table);
    listView->setAutoScroll(true);
    listView->setHintText("Large Neu_ListView bound to std::vector<std::vector<std::string>>. It auto-scrolls vertically and horizontally.");
    window.add(listView);

    auto code = std::make_shared<Neu_RichTextCode>(Neu_Layout{20, 365, 800, 250, 1.0f, 0, 0});
    std::ostringstream source;
    source << "#include <Neutrino/Neutrino.hpp>\n";
    source << "class DemoWindow {\npublic:\n";
    for (int i = 0; i < 120; ++i) {
        source << "    void callback" << i << "() { /* long coding line " << i << " demonstrating horizontal scrolling and antialiased Xft text */ }\n";
    }
    source << "};\n";
    code->setText(source.str());
    code->setAutoScroll(true);
    code->setLanguageName("C++17");
    code->setHintText("A simple coding-oriented rich text control. It shows line numbers, keyword line highlighting and read-only mode support.");
    window.add(code);

    auto image = std::make_shared<Neu_ImageView>(Neu_Layout{850, 60, 150, 140, 1.0f, 0, 0});
    image->loadBmp("assets/icons/save_icon.bmp");
    image->setHintText("Neu_ImageView displays BMP images without Cairo.");
    window.add(image);

    auto progress = std::make_shared<Neu_ProgressSquare>(Neu_Layout{1025, 60, 150, 140, 1.0f, 0, 0});
    progress->setProgress(0.92f);
    progress->setHintText("Neu_ProgressSquare highlights the square edges as progress nears completion.");
    window.add(progress);

    auto button = std::make_shared<Neu_Button>(Neu_Layout{850, 220, 325, 40, 1.0f, 0, 0});
    button->setText("BMP icon button");
    button->setIconBmp("assets/icons/button_icon.bmp");
    Neu_Callbacks callbacks;
    callbacks.onClick = clicked;
    button->setCallbacks(callbacks);
    button->setHintText("Neu_Button still uses function-pointer callbacks, not signals or slots.");
    window.add(button);

    auto rich = std::make_shared<Neu_ReadOnlyRichText>(Neu_Layout{850, 285, 325, 330, 1.0f, 0, 0});
    rich->setContentSize(325, 900);
    rich->setAutoScroll(true);
    rich->setIconList({"assets/icons/sample_icon.bmp", "assets/icons/menu_icon.bmp", "assets/icons/save_icon.bmp", "assets/icons/button_icon.bmp"});
    rich->addLabel("No hash label chooses icon index 0");
    rich->addLabel("# One marker chooses icon index 1");
    rich->addMultilineLabel("## Two markers choose icon index 2\nThis is a multiline sub-label inside read-only rich text.");
    rich->addMultilineLabel("### Three markers choose icon index 3\nUse \\# to show a literal hash: \\# remains visible as #.");
    for (int i = 0; i < 20; ++i) {
        rich->addLabel("# Generated read-only rich label row " + std::to_string(i));
    }
    rich->setHintText("Read-only rich text can contain Neu_Label and Neu_MultilineLabel sub-components. Icons are selected from an STL list by the count of unescaped # characters.");
    window.add(rich);

    auto multiline = std::make_shared<Neu_MultilineLabel>(Neu_Layout{20, 640, 1155, 120, 1.0f, 0, 0});
    multiline->setIconBmp("assets/icons/sample_icon.bmp");
    multiline->setAutoScroll(true);
    std::ostringstream lines;
    for (int i = 0; i < 50; ++i) {
        lines << "Multiline label line " << i << " with BMP icon and auto-scroll support.\n";
    }
    multiline->setText(lines.str());
    window.add(multiline);

    window.show();
    app.run();
    return 0;
}
