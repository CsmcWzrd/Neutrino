#include "Neutrino/Neutrino.hpp"
#include <iostream>
#include <memory>

using namespace neutrino;

struct TreeControls {
    Neu_TreeView* tree{nullptr};
};

static void on_expand_all(Neu_Control*, void* userData)
{
    auto* controls = static_cast<TreeControls*>(userData);
    if (controls && controls->tree) {
        controls->tree->expandAll();
        std::cout << "Tree expanded" << std::endl;
    }
}

static void on_collapse_all(Neu_Control*, void* userData)
{
    auto* controls = static_cast<TreeControls*>(userData);
    if (controls && controls->tree) {
        controls->tree->collapseAll();
        std::cout << "Tree collapsed" << std::endl;
    }
}

static void on_tree_selection(Neu_Control*, int row, int, const char* value, void*)
{
    std::cout << "Tree node selected from model row " << row << ": " << value << std::endl;
}

int main()
{
    Neu_Application app;
    if (!app.open()) {
        std::cerr << "Unable to open X display. Ensure X server and DISPLAY are available.\n";
        return 1;
    }

    Neu_Window window(app, 720, 520, "Neutrino Test - Collapsible TreeView");
    window.setTheme(Neu_Theme::MaterialDark());
    if (!window.create()) {
        return 1;
    }

    static Neu_StringTable treeModel = {
        {"Neutrino"},
        {"Neutrino", "Controls"},
        {"Neutrino", "Controls", "Neu_Textbox"},
        {"Neutrino", "Controls", "Neu_Passwordbox"},
        {"Neutrino", "Controls", "Neu_Multilinetextbox"},
        {"Neutrino", "Controls", "Neu_Button"},
        {"Neutrino", "Views"},
        {"Neutrino", "Views", "Neu_ListView"},
        {"Neutrino", "Views", "Neu_TreeView"},
        {"Neutrino", "Windows"},
        {"Neutrino", "Windows", "Neu_Window"},
        {"Neutrino", "Windows", "Neu_PopWindowMenu"},
        {"Neutrino", "Build"},
        {"Neutrino", "Build", "CMake"},
        {"Neutrino", "Build", "Makefile"},
        {"Neutrino", "Build", "Autoconf"}
    };

    TreeControls controls;

    Neu_Callbacks treeCallbacks;
    treeCallbacks.onSelectionChanged = on_tree_selection;

    auto tree = std::make_shared<Neu_TreeView>(Neu_Layout{25, 25, 420, 430, 1.0f, 520, 480});
    tree->bind(&treeModel);
    tree->setCallbacks(treeCallbacks);
    controls.tree = tree.get();
    window.add(tree);

    Neu_Callbacks expandCallbacks;
    expandCallbacks.onClick = on_expand_all;
    expandCallbacks.userData = &controls;

    Neu_Callbacks collapseCallbacks;
    collapseCallbacks.onClick = on_collapse_all;
    collapseCallbacks.userData = &controls;

    auto expandButton = std::make_shared<Neu_Button>(Neu_Layout{470, 40, 190, 38, 1.0f, 220, 48});
    expandButton->setText("Expand all");
    expandButton->setCallbacks(expandCallbacks);
    window.add(expandButton);

    auto collapseButton = std::make_shared<Neu_Button>(Neu_Layout{470, 90, 190, 38, 1.0f, 220, 48});
    collapseButton->setText("Collapse all");
    collapseButton->setCallbacks(collapseCallbacks);
    window.add(collapseButton);

    auto help = std::make_shared<Neu_Multilinetextbox>(Neu_Layout{470, 150, 210, 150, 1.0f, 250, 180});
    help->setText("Click + / - tree rows to toggle nodes.\nUse buttons to expand or collapse the entire tree.\nEvents are plain function pointers.");
    window.add(help);

    window.show();
    app.run();
    return 0;
}
