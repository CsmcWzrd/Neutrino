#include "common/Neu_TestCommon.hpp"

using namespace neutrino;
using namespace neutrino_tests;

struct RenderState
{
    Neu_Window* window{nullptr};
    Neu_Textbox* status{nullptr};
    bool buffering{true};
    bool shadows{true};
};

static void toggle_buffering(Neu_Control*, void* user_data)
{
    auto* state = static_cast<RenderState*>(user_data);
    if (!state || !state->window) {
        return;
    }
    state->buffering = !state->buffering;
    state->window->setMultiStageDoubleBuffering(state->buffering);
    if (state->status) {
        state->status->setText(state->buffering ? "Multi-stage double buffering enabled" : "Multi-stage double buffering disabled");
    }
    state->window->redraw();
}

static void toggle_shadows(Neu_Control*, void* user_data)
{
    auto* state = static_cast<RenderState*>(user_data);
    if (!state || !state->window) {
        return;
    }
    state->shadows = !state->shadows;
    Neu_SmoothGraphicsOptions options = Neu_GetSmoothGraphicsOptions();
    options.drawShadows = state->shadows;
    Neu_SetSmoothGraphicsOptions(options);
    if (state->status) {
        state->status->setText(state->shadows ? "Control shadows enabled" : "Control shadows disabled");
    }
    state->window->redraw();
}

int main(int argc, char** argv)
{
    Neu_Application app;
    if (!start_app(app, argc, argv)) {
        return 1;
    }

    Neu_SmoothGraphicsOptions options = Neu_GetSmoothGraphicsOptions();
    options.enabled = true;
    options.multiStageDoubleBuffering = true;
    options.bufferStages = 3;
    options.drawHints = true;
    options.drawShadows = true;
    Neu_SetSmoothGraphicsOptions(options);

    Neu_Window win(app, 940, 600, "Neutrino Test 11 - Rendering, Shadows, Hints, Double Buffering");
    apply_test_window_defaults(win);
    if (!win.create()) {
        return 1;
    }

    auto status = add_status(win, "Hover controls to view hints; toggle rendering features with function pointers.");
    RenderState state{&win, status.get(), true, true};

    Neu_Callbacks buffering_cb;
    buffering_cb.onClick = toggle_buffering;
    buffering_cb.userData = &state;
    auto buffering = std::make_shared<Neu_Button>(Neu_Layout{45, 95, 290, 44, 1.0f, 340, 58});
    buffering->setText("Toggle double buffering");
    buffering->setIconBmp("assets/icons/button_icon.bmp");
    buffering->setHintText("Neutrino supports software multi-stage double buffering: background, composition, final overlay, then one blit.");
    buffering->setCallbacks(buffering_cb);
    win.add(buffering);

    Neu_Callbacks shadow_cb;
    shadow_cb.onClick = toggle_shadows;
    shadow_cb.userData = &state;
    auto shadows = std::make_shared<Neu_Button>(Neu_Layout{370, 95, 260, 44, 1.0f, 310, 58});
    shadows->setText("Toggle shadows");
    shadows->setIconBmp("assets/icons/save_icon.bmp");
    shadows->setHintText("Control shadows can be disabled for VM-friendly rendering, or enabled for smoother glass UI.");
    shadows->setCallbacks(shadow_cb);
    win.add(shadows);

    auto long_hint = std::make_shared<Neu_Textbox>(Neu_Layout{45, 170, 585, 40, 1.0f, 650, 55});
    long_hint->setText("Hover here for the long hint popup");
    long_hint->setHintExpanded(true);
    long_hint->setHintText("This intentionally long hint verifies the hint popup path. The popup wraps to a maximum width of 400 pixels, displays a drop-down indicator for more content, and draws a vertical scrollbar when the natural text height exceeds the configured maximum. The same rendering path is used on Linux/X11 and the Win32/GDI backend where available.");
    win.add(long_hint);

    auto list = std::make_shared<Neu_Listbox>(Neu_Layout{45, 245, 585, 230, 1.0f, 650, 300});
    list->setAutoScroll(true);
    list->setItems(many_items("Buffered rendering row", 60));
    list->setCallbacks(selection_callbacks(status.get()));
    list->setHintText("Scroll this list to exercise redraw coalescing and buffering.");
    win.add(list);

    auto progress = std::make_shared<Neu_ProgressSquare>(Neu_Layout{690, 155, 150, 150, 1.0f, 180, 180});
    progress->setProgress(0.92f);
    progress->setHintText("ProgressSquare edge highlight at 92% completion.");
    win.add(progress);

    win.show();
    app.run();
    return 0;
}
