#include "common/Neu_TestCommon.hpp"

using namespace neutrino;
using namespace neutrino_tests;

int main(int argc, char** argv)
{
    Neu_Application app;
    if (!start_app(app, argc, argv)) {
        return 1;
    }

    Neu_Window win(app, 980, 640, "Neutrino Test 06 - Placement, Fixed Layout, Scale, Max Size");
    apply_test_window_defaults(win);
    if (!win.create()) {
        return 1;
    }

    auto status = add_status(win, "Controls use fixed left/top/width/height plus scale and max-size constraints.");
    Neu_Callbacks click = click_callbacks(status.get());
    Neu_Callbacks text = text_callbacks(status.get());

    auto frame = std::make_shared<Neu_Placement>(Neu_Layout{35, 82, 880, 470, 1.0f, 930, 540});
    frame->setText("Neu_Placement outer container");
    frame->setHintText("Placement is a container for sublayouting. Child controls keep fixed coordinates in the parent window coordinate system in this simple framework.");

    auto scaled = std::make_shared<Neu_Button>(Neu_Layout{60, 130, 180, 34, 1.6f, 230, 54});
    scaled->setText("Scaled button");
    scaled->setIconBmp("assets/icons/button_icon.bmp");
    scaled->setHintText("This button requests scale=1.6 but maxWidth/maxHeight clamp the final size.");
    scaled->setCallbacks(click);
    frame->add(scaled);

    auto capped_text = std::make_shared<Neu_Textbox>(Neu_Layout{310, 130, 260, 36, 1.4f, 300, 50});
    capped_text->setText("Scaled textbox capped by max size");
    capped_text->setCallbacks(text);
    frame->add(capped_text);

    auto inner = std::make_shared<Neu_Placement>(Neu_Layout{60, 220, 720, 220, 1.0f, 760, 260});
    inner->setText("Nested placement");
    inner->setHintText("Nested placement forwards draw and event routing to child controls.");

    auto inner_button = std::make_shared<Neu_FlatButton>(Neu_Layout{95, 275, 210, 42, 1.0f, 260, 60});
    inner_button->setText("Inner flat button");
    inner_button->setIconBmp("assets/icons/save_icon.bmp");
    inner_button->setCallbacks(click);
    inner->add(inner_button);

    auto inner_label = std::make_shared<Neu_MultilineLabel>(Neu_Layout{350, 260, 350, 90, 1.0f, 410, 130});
    inner_label->setText("Nested labels can include BMP icons and multiline text.\nThis test covers fixed layout metadata and parent propagation.");
    inner_label->setIconBmp("assets/icons/menu_icon.bmp");
    inner->add(inner_label);

    frame->add(inner);
    win.add(frame);

    win.show();
    app.run();
    return 0;
}
