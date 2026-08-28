#include "common/Neu_TestCommon.hpp"

using namespace neutrino;
using namespace neutrino_tests;

int main(int argc, char** argv)
{
    Neu_Application app;
    if (!start_app(app, argc, argv)) {
        return 1;
    }

    Neu_Window win(app, 1040, 650, "Neutrino Test 04 - ListView Typed Data Model");
    apply_test_window_defaults(win);
    if (!win.create()) {
        return 1;
    }

    auto status = add_status(win, "Click cells. Data is bound from std::vector<std::vector<std::string>>.");
    static Neu_StringTable model = typed_model();

    Neu_Callbacks selection = selection_callbacks(status.get());
    auto list_view = std::make_shared<Neu_ListView>(Neu_Layout{35, 82, 925, 470, 1.0f, 980, 560});
    list_view->bind(&model);
    list_view->setAutoScroll(true);
    list_view->setHintText("Neu_ListView integrates a view/controller with STL List<List<string>> style data and interprets strings as numbers, booleans, binary values, hex, enums, images, and UTF text.");
    list_view->setCallbacks(selection);
    win.add(list_view);

    win.show();
    app.run();
    return 0;
}
