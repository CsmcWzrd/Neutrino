#include "common/Neu_TestCommon.hpp"

using namespace neutrino;
using namespace neutrino_tests;

int main(int argc, char** argv)
{
    Neu_Application app;
    if (!start_app(app, argc, argv)) {
        return 1;
    }

    Neu_Window win(app, 920, 660, "Neutrino Test 15 - Stage2 Value and Progress Controls");
    apply_test_window_defaults(win);
    if (!win.create()) {
        return 1;
    }

    auto status = add_status(win, "Value controls: progress bar, progress square, slider, vertical slider, spinner.");

    auto progress = std::make_shared<Neu_ProgressBar>(Neu_Layout{48, 100, 720, 38});
    progress->setProgress(0.72f);
    progress->setText("ProgressBar 72% - horizontal value indicator");
    progress->setHintText("Neu_ProgressBar mirrors Swing JProgressBar and SWT ProgressBar style behaviour.");
    win.add(progress);

    auto square = std::make_shared<Neu_ProgressSquare>(Neu_Layout{48, 160, 170, 170});
    square->setProgress(0.84f);
    square->setHintText("ProgressSquare traces clockwise from top-center around all edges.");
    win.add(square);

    auto slider = std::make_shared<Neu_Slider>(Neu_Layout{260, 178, 500, 48});
    slider->setRange(0, 100);
    slider->setValue(64);
    slider->setCallbacks(click_callbacks(status.get()));
    slider->setHintText("Neu_Slider mirrors Swing JSlider and SWT Scale within the fixed layout model.");
    win.add(slider);

    auto vertical = std::make_shared<Neu_Slider>(Neu_Layout{790, 108, 56, 250});
    vertical->setVertical(true);
    vertical->setRange(0, 255);
    vertical->setValue(190);
    vertical->setHintText("Vertical slider test. Drag the knob on Linux or Windows.");
    win.add(vertical);

    auto spinner = std::make_shared<Neu_Spinner>(Neu_Layout{260, 254, 220, 40});
    spinner->setRange(-20, 120);
    spinner->setValue(15);
    spinner->setCallbacks(text_callbacks(status.get()));
    spinner->setHintText("Neu_Spinner provides simple up/down numeric input.");
    win.add(spinner);

    auto image = std::make_shared<Neu_ImageView>(Neu_Layout{510, 254, 96, 96});
    image->loadBmp("assets/icons/save_icon.bmp");
    image->setHintText("ImageView displays BMP content only, matching the project icon requirement.");
    win.add(image);

    auto label = std::make_shared<Neu_MultilineLabel>(Neu_Layout{48, 380, 798, 190});
    label->setWordWrap(true);
    label->setBorderVisible(true);
    label->setTextOffset(12, 18, 12, 18);
    label->setText("This test keeps value-oriented controls separate from selection controls. It also validates that labels and hints remain clipped while the Stage2 value controls are used interactively.");
    win.add(label);

    win.show();
    app.run();
    return 0;
}
