#include "Neutrino/Neutrino.hpp"
#include <iostream>
#include <memory>

using namespace neutrino;

static const char* type_name(Neu_CellType type) {
    switch (type) {
        case Neu_CellType::String: return "String";
        case Neu_CellType::UtfString: return "UtfString";
        case Neu_CellType::Int64: return "Int64";
        case Neu_CellType::UInt64: return "UInt64";
        case Neu_CellType::Float: return "Float";
        case Neu_CellType::Double: return "Double";
        case Neu_CellType::Binary8: return "Binary8";
        case Neu_CellType::Binary16: return "Binary16";
        case Neu_CellType::Binary32: return "Binary32";
        case Neu_CellType::Binary64: return "Binary64";
        case Neu_CellType::Binary128: return "Binary128";
        case Neu_CellType::Hex: return "Hex";
        case Neu_CellType::Boolean: return "Boolean";
        case Neu_CellType::TriState: return "TriState";
        case Neu_CellType::Enum: return "Enum";
        case Neu_CellType::ImageBmp: return "ImageBmp";
        case Neu_CellType::Checkbox: return "Checkbox";
    }
    return "Unknown";
}

static void on_view_selection(Neu_Control* sender, int row, int column, const char* value, void* user_data) {
    auto* model = static_cast<Neu_StringTable*>(user_data);
    if (!model || row < 0 || column < 0) return;
    Neu_TypedValue typed = Neu_TypeInterpreter::interpret((*model)[static_cast<size_t>(row)][static_cast<size_t>(column)]);
    std::cout << sender->className() << " selected [" << row << "," << column << "] "
              << value << " -> " << type_name(typed.type) << std::endl;
}

int main() {
    Neu_Application app;
    if (!app.open()) {
        std::cerr << "Unable to open X display. Ensure X server and DISPLAY are available.\n";
        return 1;
    }

    Neu_Window win(app, 980, 620, "Neutrino Test - ListView and TreeView Typed Data");
    win.setTheme(Neu_Theme::MaterialDark());
    if (!win.create()) return 1;

    static Neu_StringTable model = {
        {"Type", "Literal", "Meaning"},
        {"string", "plain text", "basic std::string"},
        {"utf string", "utf:Unicode text", "UTF string marker"},
        {"signed number", "-12345", "int64"},
        {"unsigned number", "12345", "uint64/int64"},
        {"float", "1.25f", "single precision"},
        {"double", "1.25", "double precision"},
        {"boolean true", "1:true", "true checkbox"},
        {"boolean false", "0:false", "false checkbox"},
        {"tristate", "tri:maybe", "three-state value"},
        {"enum", "enum:ReadWrite", "enumeration literal"},
        {"hex", "0xDEADBEEF", "hexadecimal number"},
        {"binary8", "bin8:11110000", "8-bit binary"},
        {"binary16", "bin16:1111000011110000", "16-bit binary"},
        {"binary32", "bin32:11110000111100001111000011110000", "32-bit binary"},
        {"binary64", "bin64:1111000011110000111100001111000011110000111100001111000011110000", "64-bit binary"},
        {"binary128", "bin128:11110000111100001111000011110000111100001111000011110000111100001111000011110000111100001111000011110000111100001111000011110000", "128-bit binary"},
        {"BMP image", "image:assets/icons/sample_icon.bmp", "BMP-only icon/image reference"}
    };

    Neu_Callbacks cb;
    cb.onSelectionChanged = on_view_selection;
    cb.userData = &model;

    auto list_view = std::make_shared<Neu_ListView>(Neu_Layout{25, 25, 600, 540, 1.0f, 760, 600});
    list_view->bind(&model);
    list_view->setCallbacks(cb);
    win.add(list_view);

    auto tree_view = std::make_shared<Neu_TreeView>(Neu_Layout{650, 25, 300, 540, 1.0f, 380, 600});
    tree_view->bind(&model);
    tree_view->setCallbacks(cb);
    win.add(tree_view);

    win.show();
    app.run();
    return 0;
}
