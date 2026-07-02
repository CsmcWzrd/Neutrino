#include "common/Neu_TestCommon.hpp"

using namespace neutrino;
using namespace neutrino_tests;

struct ProgressState
{
    Neu_ProgressSquare* progress{nullptr};
    Neu_Textbox* status{nullptr};
    float value{0.0f};
};

static void step_progress(Neu_Control*, void* user_data)
{
    auto* state = static_cast<ProgressState*>(user_data);
    if (!state || !state->progress) {
        return;
    }
    state->value += 0.20f;
    if (state->value > 1.0f) {
        state->value = 0.0f;
    }
    state->progress->setProgress(state->value);
    if (state->status) {
        std::ostringstream out;
        out << "ProgressSquare value set to " << static_cast<int>(state->value * 100.0f) << "%";
        state->status->setText(out.str());
    }
}

int main(int argc, char** argv)
{
    Neu_Application app;
    if (!start_app(app, argc, argv)) {
        return 1;
    }

    Neu_Window win(app, 960, 620, "Neutrino Test 10 - ImageView, ProgressSquare, Label, MultilineLabel");
    apply_test_window_defaults(win);
    if (!win.create()) {
        return 1;
    }

    auto status = add_status(win, "This test covers image, progress, label, and multiline-label controls.");

    auto image = std::make_shared<Neu_ImageView>(Neu_Layout{45, 95, 210, 210, 1.0f, 240, 240});
    image->loadBmp("assets/icons/sample_icon.bmp");
    image->setHintText("Neu_ImageView displays BMP images. This framework intentionally limits icon/image loading to BMP format.");
    win.add(image);

    auto label = std::make_shared<Neu_Label>(Neu_Layout{300, 95, 320, 40, 1.0f, 390, 55});
    label->setText("Neu_Label with BMP icon");
    label->setIconBmp("assets/icons/button_icon.bmp");
    label->setHintText("Single-line labels support BMP icons.");
    win.add(label);

    auto multi = std::make_shared<Neu_MultilineLabel>(Neu_Layout{300, 155, 360, 130, 1.0f, 430, 180});
    multi->setText("Neu_MultilineLabel with BMP icon\nLine 2 of text\nLine 3 of text");
    multi->setIconBmp("assets/icons/menu_icon.bmp");
    multi->setHintText("Multiline labels support icon drawing and wrapped documentation text.");
    win.add(multi);

    auto progress = std::make_shared<Neu_ProgressSquare>(Neu_Layout{700, 95, 150, 150, 1.0f, 180, 180});
    progress->setProgress(0.65f);
    progress->setHintText("ProgressSquare edge highlights increase as progress nears completion.");
    win.add(progress);

    ProgressState state{progress.get(), status.get(), 0.65f};
    Neu_Callbacks step_cb;
    step_cb.onClick = step_progress;
    step_cb.userData = &state;
    auto step = std::make_shared<Neu_Button>(Neu_Layout{680, 285, 190, 44, 1.0f, 240, 58});
    step->setText("Step progress");
    step->setIconBmp("assets/icons/save_icon.bmp");
    step->setCallbacks(step_cb);
    win.add(step);

    win.show();
    app.run();
    return 0;
}
