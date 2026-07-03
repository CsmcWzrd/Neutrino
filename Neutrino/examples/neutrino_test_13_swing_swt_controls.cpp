#include "common/Neu_TestCommon.hpp"

using namespace neutrino;
using namespace neutrino_tests;

struct Stage2State
{
    Neu_Window* window{nullptr};
    Neu_Textbox* status{nullptr};
    Neu_ProgressBar* progress{nullptr};
    Neu_Slider* slider{nullptr};
    Neu_ComboBox* themeBox{nullptr};
    float progressValue{0.25f};
};

static void update_status(Neu_Control* sender, void* user_data)
{
    auto* state = static_cast<Stage2State*>(user_data);
    if (!state || !state->status) {
        return;
    }
    state->status->setText(std::string(sender->className()) + " clicked");
}

static void step_progress(Neu_Control*, void* user_data)
{
    auto* state = static_cast<Stage2State*>(user_data);
    if (!state || !state->progress) {
        return;
    }
    state->progressValue += 0.15f;
    if (state->progressValue > 1.0f) {
        state->progressValue = 0.0f;
    }
    state->progress->setProgress(state->progressValue);
    if (state->status) {
        state->status->setText("ProgressBar changed to " + std::to_string(static_cast<int>(state->progressValue * 100.0f)) + "%");
    }
}

static void theme_changed(Neu_Control*, int row, int, const char* value, void* user_data)
{
    auto* state = static_cast<Stage2State*>(user_data);
    if (!state || !state->window || !value) {
        return;
    }
    state->window->setTheme(Neu_Theme::BuiltInThemeByName(value));
    if (state->status) {
        state->status->setText(std::string("Theme selected: ") + value + " row " + std::to_string(row));
    }
    state->window->requestRedraw();
}

int main(int argc, char** argv)
{
    Neu_Application app;
    if (!start_app(app, argc, argv)) {
        return 1;
    }

    Neu_Window win(app, 1120, 760, "Neutrino Test 13 - Stage2 Swing/SWT Inspired Controls and 24+ Themes");
    apply_test_window_defaults(win);
    if (!win.create()) {
        return 1;
    }

    auto status = add_status(win, "Stage2: check box, radio button, slider/scale, spinner, tab view, group box, separator, toolbar, link, progress bar, splitter, and themes.");
    Stage2State state{&win, status.get(), nullptr, nullptr, nullptr, 0.25f};

    Neu_Callbacks cb;
    cb.onClick = update_status;
    cb.userData = &state;

    auto toolbar = std::make_shared<Neu_ToolBar>(Neu_Layout{25, 72, 1060, 46});
    toolbar->setText("Toolbar");
    win.add(toolbar);

    auto save = std::make_shared<Neu_Button>(Neu_Layout{40, 80, 110, 30});
    save->setText("Save");
    save->setIconBmp("assets/icons/save_icon.bmp");
    save->setCallbacks(cb);
    toolbar->add(save);

    auto run = std::make_shared<Neu_Button>(Neu_Layout{160, 80, 110, 30});
    run->setText("Run");
    run->setIconBmp("assets/icons/button_icon.bmp");
    run->setCallbacks(cb);
    toolbar->add(run);

    auto themeBox = std::make_shared<Neu_ComboBox>(Neu_Layout{820, 80, 240, 300});
    themeBox->setItems(Neu_Theme::BuiltInThemeNames());
    themeBox->setText("Choose Theme");
    Neu_Callbacks themeCb;
    themeCb.onSelectionChanged = theme_changed;
    themeCb.userData = &state;
    themeBox->setCallbacks(themeCb);
    state.themeBox = themeBox.get();
    win.add(themeBox);

    auto group = std::make_shared<Neu_GroupBox>(Neu_Layout{30, 140, 360, 230});
    group->setText("Swing/SWT selection controls");
    win.add(group);

    auto check = std::make_shared<Neu_CheckBox>(Neu_Layout{55, 172, 290, 32});
    check->setText("Neu_CheckBox - selectable boolean option");
    check->setChecked(true);
    check->setTextOffset(2, 4, 2, 8);
    check->setHintText("Inspired by Swing JCheckBox and SWT Button CHECK style.");
    check->setCallbacks(cb);
    group->add(check);

    auto radio1 = std::make_shared<Neu_RadioButton>(Neu_Layout{55, 210, 280, 32});
    radio1->setText("Neu_RadioButton option A");
    radio1->setChecked(true);
    radio1->setCallbacks(cb);
    group->add(radio1);

    auto radio2 = std::make_shared<Neu_RadioButton>(Neu_Layout{55, 246, 280, 32});
    radio2->setText("Neu_RadioButton option B");
    radio2->setCallbacks(cb);
    group->add(radio2);

    auto spinner = std::make_shared<Neu_Spinner>(Neu_Layout{55, 292, 150, 34});
    spinner->setRange(0, 99);
    spinner->setValue(12);
    spinner->setHintText("Neu_Spinner is a simple numeric up/down control.");
    spinner->setCallbacks(cb);
    group->add(spinner);

    auto separator = std::make_shared<Neu_Separator>(Neu_Layout{225, 292, 120, 34});
    separator->setHintText("Neu_Separator mirrors SWT Separator style and Swing separators.");
    group->add(separator);

    auto toggle = std::make_shared<Neu_ToggleButton>(Neu_Layout{225, 326, 120, 30});
    toggle->setText("Toggle");
    toggle->setChecked(false);
    toggle->setHintText("Neu_ToggleButton maps to toggle-style push buttons in desktop toolkits.");
    toggle->setCallbacks(cb);
    group->add(toggle);

    auto progress = std::make_shared<Neu_ProgressBar>(Neu_Layout{430, 142, 360, 34});
    progress->setProgress(state.progressValue);
    progress->setHintText("Neu_ProgressBar is a linear progress display in addition to Neu_ProgressSquare.");
    state.progress = progress.get();
    win.add(progress);

    Neu_Callbacks stepCb;
    stepCb.onClick = step_progress;
    stepCb.userData = &state;
    auto step = std::make_shared<Neu_Button>(Neu_Layout{805, 142, 130, 34});
    step->setText("Step");
    step->setCallbacks(stepCb);
    win.add(step);

    auto slider = std::make_shared<Neu_Slider>(Neu_Layout{430, 195, 360, 42});
    slider->setRange(0, 100);
    slider->setValue(55);
    slider->setHintText("Neu_Slider mirrors Swing JSlider and SWT Scale with fixed-position layout.");
    state.slider = slider.get();
    win.add(slider);

    auto link = std::make_shared<Neu_LinkLabel>(Neu_Layout{430, 250, 420, 32});
    link->setText("Neu_LinkLabel - clickable underlined label");
    link->setHintText("Inspired by SWT Link and hyperlink-style Swing labels.");
    link->setCallbacks(cb);
    win.add(link);

    auto tabs = std::make_shared<Neu_TabView>(Neu_Layout{430, 300, 650, 260});
    tabs->setHintText("Neu_TabView mirrors Swing JTabbedPane and SWT TabFolder without adding a new layouting scheme.");

    auto page1 = std::make_shared<Neu_Placement>(Neu_Layout{450, 350, 590, 170});
    auto p1l = std::make_shared<Neu_Label>(Neu_Layout{470, 365, 520, 34});
    p1l->setText("Tab page 1 contains fixed-position child controls.");
    p1l->setBorderVisible(true);
    p1l->setTextOffset(4, 8, 4, 10);
    page1->add(p1l);
    auto p1b = std::make_shared<Neu_Button>(Neu_Layout{470, 415, 150, 34});
    p1b->setText("Page Button");
    p1b->setCallbacks(cb);
    page1->add(p1b);

    auto page2 = std::make_shared<Neu_Placement>(Neu_Layout{450, 350, 590, 170});
    auto p2m = std::make_shared<Neu_MultilineLabel>(Neu_Layout{470, 365, 520, 95});
    p2m->setText("Tab page 2 demonstrates multiline label clipping and wrapping. Long text remains inside the fixed rectangle.");
    p2m->setBorderVisible(true);
    p2m->setTextOffset(6, 8, 6, 10);
    page2->add(p2m);

    tabs->addTab("Controls", page1);
    tabs->addTab("Text", page2);
    win.add(tabs);

    auto splitter = std::make_shared<Neu_Splitter>(Neu_Layout{30, 395, 360, 160});
    splitter->setText("Splitter");
    splitter->setSplitPosition(170);
    splitter->setHintText("Neu_Splitter mirrors Swing JSplitPane/SWT Sash as a fixed-position sash control.");
    win.add(splitter);

    auto leftLabel = std::make_shared<Neu_Label>(Neu_Layout{45, 425, 140, 36});
    leftLabel->setText("Left pane");
    leftLabel->setBorderVisible(true);
    leftLabel->setTextOffset(4, 8, 4, 10);
    splitter->add(leftLabel);

    auto rightLabel = std::make_shared<Neu_Label>(Neu_Layout{205, 425, 145, 36});
    rightLabel->setText("Right pane");
    rightLabel->setBorderVisible(true);
    rightLabel->setTextOffset(4, 8, 4, 10);
    splitter->add(rightLabel);

    auto note = std::make_shared<Neu_MultilineLabel>(Neu_Layout{30, 590, 1050, 90});
    note->setText("Stage2 keeps the original fixed top-left/width/height layout model. New controls were selected after checking Swing and SWT control families: check/radio buttons, progress, slider/scale, tab folder, group box, separator, link, toolbar, spinner and splitter/sash equivalents.");
    note->setBorderVisible(true);
    note->setTextOffset(8, 10, 8, 10);
    note->setHintText("Move the mouse here and hold still for five seconds to see the delayed hint popup.");
    win.add(note);

    win.show();
    app.run();
    return 0;
}
