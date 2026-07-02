#include "Neutrino/Neutrino.hpp"
#include <iostream>
#include <cstring>
using namespace neutrino;

static void on_button(Neu_Control*, void* user_data) {
    auto* label = static_cast<Neu_Textbox*>(user_data);
    label->setText("Button clicked using a function pointer callback");
}

static void on_text(Neu_Control*, const char* text, void*) {
    std::cout << "Text changed: " << text << std::endl;
}

static void on_select(Neu_Control*, int row, int column, const char* value, void*) {
    std::cout << "Selected row=" << row << " column=" << column << " value=" << value << std::endl;
}

int main(int argc, char** argv) {
    bool fastMode = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--fast") == 0 || std::strcmp(argv[i], "--vm") == 0) {
            fastMode = true;
        }
    }

    Neu_Application app;
    if (!app.open()) {
        std::cerr << "Unable to open X display. Ensure X server and DISPLAY are available.\n";
        return 1;
    }

    if (fastMode) {
        Neu_UseVirtualMachineFriendlyDefaults(true);
    }

    Neu_Window win(app, 900, 620, "Neutrino Demo");
    win.setTheme(Neu_Theme::BlueGlass());
    if (!win.create()) return 1;

    auto textbox = std::make_shared<Neu_Textbox>(Neu_Layout{30, 30, 270, 36, 1.0f, 400, 60});
    textbox->setText("Edit me");
    textbox->setHintText("High quality text rendering is enabled by default with Xft/Fontconfig when available. This textbox also highlights when the pointer enters and leaves it.");
    Neu_Callbacks textCb; textCb.onTextChanged = on_text;
    textbox->setCallbacks(textCb);
    win.add(textbox);

    auto pass = std::make_shared<Neu_Passwordbox>(Neu_Layout{30, 80, 270, 36, 1.0f, 400, 60});
    pass->setText("secret");
    pass->setHintText("Neu_Passwordbox masks text input while keeping the same function pointer callback style as every other control.");
    win.add(pass);

    auto multi = std::make_shared<Neu_Multilinetextbox>(Neu_Layout{30, 130, 270, 100, 1.0f, 420, 160});
    multi->setText("Neu_Multilinetextbox\nSecond line");
    win.add(multi);

    auto button = std::make_shared<Neu_Button>(Neu_Layout{330, 30, 220, 40, 1.0f, 260, 60});
    button->setText("Neu_Button");
    button->setIconBmp("assets/icons/button_icon.bmp");
    button->setHintText("Buttons, flat buttons, and menu items load BMP icons only. Hover over controls to see highlighted rounded glass rendering and shadowing.");
    Neu_Callbacks btnCb; btnCb.onClick = on_button; btnCb.userData = textbox.get();
    button->setCallbacks(btnCb);
    win.add(button);

    auto list = std::make_shared<Neu_Listbox>(Neu_Layout{330, 90, 220, 140, 1.0f, 260, 180});
    list->setItems({"Neu_Listbox Item 1", "Neu_Listbox Item 2", "Neu_Listbox Item 3"});
    list->setHintText("Neu_Listbox supports mouse selection with function pointer callbacks. The tooltip window is limited to 400 pixels wide. Long content can display a drop-down marker and scrollbar.");
    Neu_Callbacks selCb; selCb.onSelectionChanged = on_select;
    list->setCallbacks(selCb);
    win.add(list);

    static Neu_StringTable table = {
        {"Name", "Age", "Active", "Score"},
        {"Alice", "42", "1:true", "98.5"},
        {"Bob", "0x2A", "0:false", "bin8:10101010"},
        {"Image", "image:sample.bmp", "tri:maybe", "enum:Admin"}
    };
    auto lv = std::make_shared<Neu_ListView>(Neu_Layout{30, 260, 520, 145, 1.0f, 700, 220});
    lv->bind(&table);
    lv->setHintText("Neu_ListView binds to std::vector<std::vector<std::string>> and interprets strings as numbers, booleans, hex, binary values, enums, BMP images, UTF strings, and other typed values. This intentionally long hint demonstrates wrapping, drop-down affordance, and the vertical scrollbar when text exceeds the maximum hint height.");
    lv->setCallbacks(selCb);
    win.add(lv);

    auto tree = std::make_shared<Neu_TreeView>(Neu_Layout{580, 30, 280, 200, 1.0f, 360, 260});
    tree->bind(&table);
    tree->setHintText("Neu_TreeView supports collapsible paths and typed string interpretation using the same List<List<string>> model shape as Neu_ListView.");
    win.add(tree);

    auto pop = std::make_shared<Neu_PopWindowMenu>(Neu_Layout{580, 260, 280, 220, 1.0f, 360, 300});
    pop->setHintText("Neu_PopWindowMenu uses a left sidebar for menu categories and a right side for items, while retaining rounded, shadowed, anti-aliased drawing.");
    pop->setCategories({"File", "Edit", "View"});
    pop->setItems("File", {"New", "Open", "Save", "Exit"});
    pop->setItems("Edit", {"Undo", "Copy", "Paste"});
    pop->setItems("View", {"Zoom In", "Zoom Out", "Themes"});
    win.add(pop);

    win.show();
    app.run();
    return 0;
}
