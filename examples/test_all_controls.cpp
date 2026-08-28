#include "Neutrino/Neutrino.hpp"
#include <iostream>
#include <cstring>
#include <memory>

using namespace neutrino;

static void on_click(Neu_Control* sender, void* user_data) {
    auto* textbox = static_cast<Neu_Textbox*>(user_data);
    textbox->setText(std::string(sender->className()) + " clicked through function pointer callback");
}

static void on_text_changed(Neu_Control* sender, const char* text, void*) {
    std::cout << sender->className() << " changed text to: " << text << std::endl;
}

static void on_selection(Neu_Control* sender, int row, int column, const char* value, void*) {
    std::cout << sender->className() << " selection row=" << row
              << " column=" << column << " value=" << value << std::endl;
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

    Neu_Window win(app, 1120, 760, "Neutrino Test - All Controls");
    win.setTheme(Neu_Theme::MaterialDark());
    if (!win.create()) return 1;

    auto status = std::make_shared<Neu_Textbox>(Neu_Layout{25, 20, 520, 36, 1.0f, 620, 48});
    status->setText("Status output textbox");
    status->setHintText("Move the mouse over any control to see hover highlighting, shadows, and hint popup behavior.");
    win.add(status);

    Neu_Callbacks text_cb;
    text_cb.onTextChanged = on_text_changed;

    auto textbox = std::make_shared<Neu_Textbox>(Neu_Layout{25, 75, 250, 36, 1.0f, 360, 50});
    textbox->setText("Neu_Textbox");
    textbox->setCallbacks(text_cb);
    textbox->setHintText("Neu_Textbox uses high-quality anti-aliased Xft text when Linux provides it, otherwise it falls back to Xlib text.");
    win.add(textbox);

    auto password = std::make_shared<Neu_Passwordbox>(Neu_Layout{300, 75, 250, 36, 1.0f, 360, 50});
    password->setText("password");
    password->setCallbacks(text_cb);
    password->setHintText("Neu_Passwordbox masks characters but keeps normal textbox callbacks.");
    win.add(password);

    auto multiline = std::make_shared<Neu_Multilinetextbox>(Neu_Layout{25, 130, 525, 110, 1.0f, 650, 160});
    multiline->setText("Neu_Multilinetextbox\nType multiple lines here.");
    multiline->setCallbacks(text_cb);
    multiline->setHintText("Neu_Multilinetextbox accepts several lines and demonstrates the same rounded, shadowed control rendering.");
    win.add(multiline);

    Neu_Callbacks click_cb;
    click_cb.onClick = on_click;
    click_cb.userData = status.get();

    auto button = std::make_shared<Neu_Button>(Neu_Layout{580, 20, 180, 40, 1.0f, 240, 55});
    button->setText("Neu_Button");
    button->setIconBmp("assets/icons/button_icon.bmp");
    button->setHintText("Neu_Button with a BMP icon, shadow, hover highlight, and function-pointer click callback.");
    button->setCallbacks(click_cb);
    win.add(button);

    auto flat = std::make_shared<Neu_FlatButton>(Neu_Layout{780, 20, 180, 40, 1.0f, 240, 55});
    flat->setText("Neu_FlatButton");
    flat->setIconBmp("assets/icons/save_icon.bmp");
    flat->setHintText("Neu_FlatButton shares the Neu_Button implementation and supports the same BMP icon and callback behavior.");
    flat->setCallbacks(click_cb);
    win.add(flat);

    auto menu_item = std::make_shared<Neu_MenuItem>(Neu_Layout{580, 75, 180, 40, 1.0f, 240, 55});
    menu_item->setText("Neu_MenuItem");
    menu_item->setIconBmp("assets/icons/menu_icon.bmp");
    menu_item->setHintText("Neu_MenuItem is implemented as the same control family as Neu_Button/Neu_FlatButton and can be used in menus or toolbars.");
    menu_item->setCallbacks(click_cb);
    win.add(menu_item);

    Neu_Callbacks sel_cb;
    sel_cb.onSelectionChanged = on_selection;

    auto list = std::make_shared<Neu_Listbox>(Neu_Layout{580, 130, 220, 140, 1.0f, 260, 180});
    list->setItems({"Neu_Listbox row 1", "Neu_Listbox row 2", "Neu_Listbox row 3", "Neu_Listbox row 4"});
    list->setCallbacks(sel_cb);
    list->setHintText("Neu_Listbox selection uses function pointer callbacks, not signals or slots.");
    win.add(list);

    auto combo = std::make_shared<Neu_ComboBox>(Neu_Layout{825, 130, 220, 140, 1.0f, 260, 180});
    combo->setItems({"Neu_ComboBox choice A", "Neu_ComboBox choice B", "Neu_ComboBox choice C"});
    combo->setCallbacks(sel_cb);
    combo->setHintText("Neu_ComboBox opens its list view inside the control bounds and uses the same callback model.");
    win.add(combo);

    static Neu_StringTable table = {
        {"Column", "String", "Interpreted type", "Example value"},
        {"Int64", "42", "number", "42"},
        {"Hex", "0x2A", "hex value", "0x2A"},
        {"Bool true", "1:true", "checkbox/bool", "1:true"},
        {"Bool false", "0:false", "checkbox/bool", "0:false"},
        {"Binary8", "bin8:10101010", "binary 8 bit", "bin8:10101010"},
        {"Binary16", "bin16:1010101010101010", "binary 16 bit", "bin16:1010101010101010"},
        {"Binary32", "bin32:10101010101010101010101010101010", "binary 32 bit", "bin32:..."},
        {"Binary64", "bin64:1010101010101010101010101010101010101010101010101010101010101010", "binary 64 bit", "bin64:..."},
        {"Binary128", "bin128:10101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010", "binary 128 bit", "bin128:..."},
        {"Float", "3.14f", "float", "3.14f"},
        {"Double", "2.71828", "double", "2.71828"},
        {"TriState", "tri:maybe", "tristate", "tri:maybe"},
        {"Enum", "enum:Admin", "enum", "enum:Admin"},
        {"Image", "image:assets/icons/sample_icon.bmp", "BMP image ref", "image:..."},
        {"UTF", "utf:Hello UTF", "UTF string", "utf:Hello UTF"}
    };

    auto list_view = std::make_shared<Neu_ListView>(Neu_Layout{25, 270, 640, 220, 1.0f, 760, 280});
    list_view->bind(&table);
    list_view->setCallbacks(sel_cb);
    list_view->setHintText("Neu_ListView is bound to a std::vector<std::vector<std::string>> model and demonstrates typed interpretation of string cells including integer, float, double, hex, binary 8/16/32/64/128, checkbox, tristate, enum, BMP image references, UTF strings, and plain strings. The hint is deliberately long so the popup uses wrapping, a drop-down indicator, and a scrollbar when expanded.");
    win.add(list_view);

    auto tree_view = std::make_shared<Neu_TreeView>(Neu_Layout{690, 300, 360, 190, 1.0f, 430, 260});
    tree_view->bind(&table);
    tree_view->setCallbacks(sel_cb);
    tree_view->setHintText("Neu_TreeView supports collapse/expand and the same typed string model integration as Neu_ListView.");
    win.add(tree_view);

    auto placement = std::make_shared<Neu_Placement>(Neu_Layout{25, 520, 500, 160, 1.0f, 620, 220});
    placement->setText("Neu_Placement container with sublayout controls");
    auto nested_button = std::make_shared<Neu_Button>(Neu_Layout{45, 555, 180, 36, 1.0f, 240, 50});
    nested_button->setText("Nested Button");
    nested_button->setIconBmp("assets/icons/button_icon.bmp");
    nested_button->setHintText("Nested controls inside Neu_Placement keep their own layout and hints.");
    nested_button->setCallbacks(click_cb);
    placement->add(nested_button);
    auto nested_text = std::make_shared<Neu_Textbox>(Neu_Layout{250, 555, 240, 36, 1.0f, 300, 50});
    nested_text->setText("Nested Textbox");
    placement->add(nested_text);
    win.add(placement);

    auto pop_menu = std::make_shared<Neu_PopWindowMenu>(Neu_Layout{555, 520, 500, 160, 1.0f, 620, 220});
    pop_menu->setHintText("Neu_PopWindowMenu has a category sidebar and a right-hand menu item view.");
    pop_menu->setCategories({"File", "Edit", "View", "Help"});
    pop_menu->setItems("File", {"New", "Open", "Save", "Exit"});
    pop_menu->setItems("Edit", {"Cut", "Copy", "Paste", "Find"});
    pop_menu->setItems("View", {"Light theme", "Dark theme", "Blue glass"});
    pop_menu->setItems("Help", {"About Neutrino", "Controls test"});
    win.add(pop_menu);

    win.show();
    app.run();
    return 0;
}
