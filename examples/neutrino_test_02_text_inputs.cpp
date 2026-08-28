#include "common/Neu_TestCommon.hpp"

using namespace neutrino;
using namespace neutrino_tests;

int main(int argc, char** argv)
{
    Neu_Application app;
    if (!start_app(app, argc, argv)) {
        return 1;
    }

    Neu_Window win(app, 920, 620, "Neutrino Test 02 - Textbox, Passwordbox, Multiline Textbox");
    apply_test_window_defaults(win);
    if (!win.create()) {
        return 1;
    }

    auto status = add_status(win, "Edit the fields. Text changes are reported through function pointers.");
    Neu_Callbacks text = text_callbacks(status.get());

    auto label1 = std::make_shared<Neu_Label>(Neu_Layout{35, 78, 260, 28, 1.0f, 320, 36});
    label1->setText("Neu_Textbox");
    label1->setIconBmp("assets/icons/sample_icon.bmp");
    win.add(label1);

    auto textbox = std::make_shared<Neu_Textbox>(Neu_Layout{35, 108, 350, 40, 1.0f, 420, 55});
    textbox->setText("Type here");
    textbox->setHintText("Single-line editable text input. On Windows this exercises WM_CHAR/WM_KEYDOWN routing; on Linux it exercises X11 key input.");
    textbox->setCallbacks(text);
    win.add(textbox);

    auto label2 = std::make_shared<Neu_Label>(Neu_Layout{420, 78, 260, 28, 1.0f, 320, 36});
    label2->setText("Neu_Passwordbox");
    label2->setIconBmp("assets/icons/button_icon.bmp");
    win.add(label2);

    auto password = std::make_shared<Neu_Passwordbox>(Neu_Layout{420, 108, 350, 40, 1.0f, 420, 55});
    password->setText("secret");
    password->setHintText("Password text is masked while retaining normal textbox editing and callbacks.");
    password->setCallbacks(text);
    win.add(password);

    auto label3 = std::make_shared<Neu_MultilineLabel>(Neu_Layout{35, 175, 350, 54, 1.0f, 420, 70});
    label3->setText("Neu_Multilinetextbox below supports\nmultiple lines and scroll offsets.");
    label3->setIconBmp("assets/icons/menu_icon.bmp");
    win.add(label3);

    auto multiline = std::make_shared<Neu_Multilinetextbox>(Neu_Layout{35, 245, 735, 260, 1.0f, 820, 340});
    multiline->setAutoScroll(true);
    multiline->setText("int main() {\n    return 0;\n}\n\nAdd more text here. Mouse wheel scroll is routed to this control when focused or hovered.");
    multiline->setHintText("Multiline editing, auto-scroll metadata, and text-change callbacks are demonstrated here.");
    multiline->setCallbacks(text);
    win.add(multiline);

    win.show();
    app.run();
    return 0;
}
