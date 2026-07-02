#include "common/Neu_TestCommon.hpp"

using namespace neutrino;
using namespace neutrino_tests;

struct TreeControls
{
    Neu_TreeView* tree{nullptr};
    Neu_Textbox* status{nullptr};
};

static void expand_tree(Neu_Control*, void* user_data)
{
    auto* state = static_cast<TreeControls*>(user_data);
    if (state && state->tree) {
        state->tree->expandAll();
    }
    if (state && state->status) {
        state->status->setText("Tree expanded by function pointer callback");
    }
}

static void collapse_tree(Neu_Control*, void* user_data)
{
    auto* state = static_cast<TreeControls*>(user_data);
    if (state && state->tree) {
        state->tree->collapseAll();
    }
    if (state && state->status) {
        state->status->setText("Tree collapsed by function pointer callback");
    }
}

int main(int argc, char** argv)
{
    Neu_Application app;
    if (!start_app(app, argc, argv)) {
        return 1;
    }

    Neu_Window win(app, 940, 640, "Neutrino Test 05 - TreeView Collapse and Expand");
    apply_test_window_defaults(win);
    if (!win.create()) {
        return 1;
    }

    auto status = add_status(win, "Click tree paths with children, or use Expand All / Collapse All.");
    static Neu_StringTable model = tree_model();

    auto tree = std::make_shared<Neu_TreeView>(Neu_Layout{35, 95, 540, 430, 1.0f, 650, 520});
    tree->bind(&model);
    tree->setAutoScroll(true);
    tree->setHintText("Neu_TreeView uses the same STL List<List<string>> model shape and supports collapsible path rows.");
    tree->setCallbacks(selection_callbacks(status.get()));
    win.add(tree);

    TreeControls state{tree.get(), status.get()};
    Neu_Callbacks expand_cb;
    expand_cb.onClick = expand_tree;
    expand_cb.userData = &state;
    auto expand = std::make_shared<Neu_Button>(Neu_Layout{620, 120, 220, 44, 1.0f, 260, 58});
    expand->setText("Expand All");
    expand->setIconBmp("assets/icons/button_icon.bmp");
    expand->setCallbacks(expand_cb);
    win.add(expand);

    Neu_Callbacks collapse_cb;
    collapse_cb.onClick = collapse_tree;
    collapse_cb.userData = &state;
    auto collapse = std::make_shared<Neu_Button>(Neu_Layout{620, 185, 220, 44, 1.0f, 260, 58});
    collapse->setText("Collapse All");
    collapse->setIconBmp("assets/icons/menu_icon.bmp");
    collapse->setCallbacks(collapse_cb);
    win.add(collapse);

    auto info = std::make_shared<Neu_MultilineLabel>(Neu_Layout{620, 260, 250, 180, 1.0f, 300, 220});
    info->setText("Tree model rows:\n{Root}\n{Root, Child}\n{Root, Child, Leaf}\n\nThe control tracks collapsed paths internally.");
    info->setIconBmp("assets/icons/sample_icon.bmp");
    win.add(info);

    win.show();
    app.run();
    return 0;
}
