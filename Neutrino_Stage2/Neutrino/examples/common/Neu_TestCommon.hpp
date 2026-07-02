#pragma once

#include "Neutrino/Neutrino.hpp"
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace neutrino_tests {

inline bool has_arg(int argc, char** argv, const char* name)
{
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], name) == 0) {
            return true;
        }
    }
    return false;
}

inline bool start_app(neutrino::Neu_Application& app, int argc, char** argv)
{
    if (!app.open()) {
#ifdef _WIN32
        std::cerr << "Unable to initialize the Neutrino Win32 backend.\n";
#else
        std::cerr << "Unable to open X display. Ensure X server and DISPLAY are available.\n";
#endif
        return false;
    }

    if (has_arg(argc, argv, "--fast") || has_arg(argc, argv, "--vm")) {
        neutrino::Neu_UseVirtualMachineFriendlyDefaults(true);
    }
    return true;
}

inline void apply_test_window_defaults(neutrino::Neu_Window& window)
{
    window.setTheme(neutrino::Neu_Theme::BlueGlass());
    window.setMultiStageDoubleBuffering(true);
}

inline std::vector<std::string> many_items(const std::string& prefix, int count)
{
    std::vector<std::string> values;
    values.reserve(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        std::ostringstream out;
        out << prefix << " " << (i + 1);
        values.push_back(out.str());
    }
    return values;
}

inline neutrino::Neu_StringTable typed_model()
{
    return {
        {"Type", "Literal", "Interpreted meaning"},
        {"String", "plain text", "std::string"},
        {"UTF string", "utf:Neutrino UTF text", "UTF marker"},
        {"Int64", "-12345", "signed integer"},
        {"UInt64", "12345", "integer text"},
        {"Float", "3.25f", "single precision"},
        {"Double", "42.75", "double precision"},
        {"Hex", "0xDEADBEEF", "hexadecimal"},
        {"Binary8", "bin8:10101010", "8 bit binary"},
        {"Binary16", "bin16:1010101010101010", "16 bit binary"},
        {"Binary32", "bin32:11110000111100001111000011110000", "32 bit binary"},
        {"Binary64", "bin64:1111000011110000111100001111000011110000111100001111000011110000", "64 bit binary"},
        {"Binary128", "bin128:11110000111100001111000011110000111100001111000011110000111100001111000011110000111100001111000011110000111100001111000011110000", "128 bit binary"},
        {"Boolean true", "1:true", "checkbox true"},
        {"Boolean false", "0:false", "checkbox false"},
        {"TriState", "tri:maybe", "three-state text"},
        {"Enum", "enum:Administrator", "enum literal"},
        {"BMP image", "image:assets/icons/sample_icon.bmp", "BMP path"}
    };
}

inline neutrino::Neu_StringTable tree_model()
{
    return {
        {"Neutrino"},
        {"Neutrino", "Controls"},
        {"Neutrino", "Controls", "Neu_Button"},
        {"Neutrino", "Controls", "Neu_Textbox"},
        {"Neutrino", "Controls", "Neu_ListView"},
        {"Neutrino", "Controls", "Neu_TreeView"},
        {"Neutrino", "Containers"},
        {"Neutrino", "Containers", "Neu_Window"},
        {"Neutrino", "Containers", "Neu_Placement"},
        {"Neutrino", "Containers", "Neu_ScrollWindow"},
        {"Neutrino", "Rendering"},
        {"Neutrino", "Rendering", "Antialiasing"},
        {"Neutrino", "Rendering", "Double buffering"},
        {"Neutrino", "Rendering", "Themes"}
    };
}

static void on_click_status(neutrino::Neu_Control* sender, void* user_data)
{
    auto* status = static_cast<neutrino::Neu_Textbox*>(user_data);
    if (!status) {
        return;
    }
    status->setText(std::string(sender->className()) + " clicked through function pointer callback");
}

static void on_focus_status(neutrino::Neu_Control* sender, void* user_data)
{
    auto* status = static_cast<neutrino::Neu_Textbox*>(user_data);
    if (status) {
        status->setText(std::string("Focus/hover entered: ") + sender->className());
    }
}

static void on_blur_status(neutrino::Neu_Control* sender, void* user_data)
{
    auto* status = static_cast<neutrino::Neu_Textbox*>(user_data);
    if (status) {
        status->setText(std::string("Focus/hover left: ") + sender->className());
    }
}

static void on_text_changed_log(neutrino::Neu_Control* sender, const char* text, void* user_data)
{
    auto* status = static_cast<neutrino::Neu_Textbox*>(user_data);
    std::cout << sender->className() << " text changed: " << text << std::endl;
    if (status) {
        std::string clipped = text ? text : "";
        if (clipped.size() > 60) {
            clipped.resize(60);
            clipped += "...";
        }
        status->setText(std::string(sender->className()) + " text: " + clipped);
    }
}

static void on_selection_status(neutrino::Neu_Control* sender, int row, int column, const char* value, void* user_data)
{
    auto* status = static_cast<neutrino::Neu_Textbox*>(user_data);
    std::cout << sender->className() << " selection row=" << row << " column=" << column
              << " value=" << (value ? value : "") << std::endl;
    if (status) {
        std::ostringstream out;
        out << sender->className() << " selected row " << row << ", column " << column
            << ": " << (value ? value : "");
        status->setText(out.str());
    }
}

static void on_scroll_status(neutrino::Neu_Control* sender, int sx, int sy, void* user_data)
{
    auto* status = static_cast<neutrino::Neu_Textbox*>(user_data);
    if (status) {
        std::ostringstream out;
        out << sender->className() << " scroll offset x=" << sx << " y=" << sy;
        status->setText(out.str());
    }
}

inline neutrino::Neu_Callbacks click_callbacks(neutrino::Neu_Textbox* status)
{
    neutrino::Neu_Callbacks cb;
    cb.onClick = on_click_status;
    cb.onFocus = on_focus_status;
    cb.onBlur = on_blur_status;
    cb.userData = status;
    return cb;
}

inline neutrino::Neu_Callbacks text_callbacks(neutrino::Neu_Textbox* status)
{
    neutrino::Neu_Callbacks cb;
    cb.onTextChanged = on_text_changed_log;
    cb.onFocus = on_focus_status;
    cb.onBlur = on_blur_status;
    cb.userData = status;
    return cb;
}

inline neutrino::Neu_Callbacks selection_callbacks(neutrino::Neu_Textbox* status)
{
    neutrino::Neu_Callbacks cb;
    cb.onSelectionChanged = on_selection_status;
    cb.onFocus = on_focus_status;
    cb.onBlur = on_blur_status;
    cb.onScroll = on_scroll_status;
    cb.userData = status;
    return cb;
}

inline std::shared_ptr<neutrino::Neu_Textbox> add_status(neutrino::Neu_Window& win, const std::string& text)
{
    auto status = std::make_shared<neutrino::Neu_Textbox>(neutrino::Neu_Layout{20, 18, 720, 36, 1.0f, 900, 48});
    status->setText(text);
    status->setHintText("Status control used by function-pointer callbacks in this test application.");
    win.add(status);
    return status;
}

} // namespace neutrino_tests
