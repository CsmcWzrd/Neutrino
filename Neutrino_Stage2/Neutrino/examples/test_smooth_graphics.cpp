#include "Neutrino/Neutrino.hpp"
#include <memory>
#include <iostream>

using namespace neutrino;

static void toggleSmooth(Neu_Control*, void*) {
    auto options = Neu_GetSmoothGraphicsOptions();
    options.enabled = !options.enabled;
    Neu_SetSmoothGraphicsOptions(options);
    std::cout << "Antialiasing " << (options.enabled ? "enabled" : "disabled") << std::endl;
}

static void useSoftwareAA(Neu_Control*, void*) {
    auto options = Neu_GetSmoothGraphicsOptions();
    options.enabled = true;
    options.backend = Neu_GraphicsBackend::SoftwareAntialias;
    options.supersample = 4;
    Neu_SetSmoothGraphicsOptions(options);
    std::cout << "Using Cairo-free software antialiasing" << std::endl;
}


static void toggleBuffering(Neu_Control*, void*) {
    auto options = Neu_GetSmoothGraphicsOptions();
    options.multiStageDoubleBuffering = !options.multiStageDoubleBuffering;
    options.bufferStages = 3;
    Neu_SetSmoothGraphicsOptions(options);
    std::cout << "Multi-stage double buffering " << (options.multiStageDoubleBuffering ? "enabled" : "disabled") << std::endl;
}

static void useXRenderAA(Neu_Control*, void*) {
    auto options = Neu_GetSmoothGraphicsOptions();
    options.enabled = true;
    options.backend = Neu_GraphicsBackend::XRenderAntialias;
    options.supersample = 4;
    Neu_SetSmoothGraphicsOptions(options);
    std::cout << "Using XRender-preferred antialiasing with software fallback" << std::endl;
}

int main() {
    Neu_Application app;
    if (!app.open()) return 1;

    Neu_SmoothGraphicsOptions smooth;
    smooth.enabled = true;
    smooth.backend = app.xrenderAvailable() ? Neu_GraphicsBackend::XRenderAntialias : Neu_GraphicsBackend::SoftwareAntialias;
    smooth.supersample = 4;
    Neu_SetSmoothGraphicsOptions(smooth);

    Neu_Window window(app, 760, 520, "Neutrino Smooth Graphics Test");
    window.setTheme(Neu_Theme::BlueGlass());
    if (!window.create()) return 1;

    auto title = std::make_shared<Neu_Button>(Neu_Layout{30, 25, 420, 52, 1.0f, 0, 0});
    title->setText("Smooth rounded Neutrino controls");
    window.add(title);

    auto toggle = std::make_shared<Neu_Button>(Neu_Layout{30, 95, 220, 42, 1.0f, 0, 0});
    toggle->setText("Toggle antialiasing");
    Neu_Callbacks cb1; cb1.onClick = toggleSmooth;
    toggle->setCallbacks(cb1);
    window.add(toggle);

    auto software = std::make_shared<Neu_Button>(Neu_Layout{270, 95, 220, 42, 1.0f, 0, 0});
    software->setText("Software AA backend");
    Neu_Callbacks cb2; cb2.onClick = useSoftwareAA;
    software->setCallbacks(cb2);
    window.add(software);

    auto xrender = std::make_shared<Neu_Button>(Neu_Layout{510, 95, 220, 42, 1.0f, 0, 0});
    xrender->setText("XRender backend");
    Neu_Callbacks cb3; cb3.onClick = useXRenderAA;
    xrender->setCallbacks(cb3);
    window.add(xrender);

    auto buffering = std::make_shared<Neu_Button>(Neu_Layout{30, 145, 300, 36, 1.0f, 0, 0});
    buffering->setText("Toggle multi-stage buffering");
    Neu_Callbacks cb4; cb4.onClick = toggleBuffering;
    buffering->setCallbacks(cb4);
    window.add(buffering);

    auto placement = std::make_shared<Neu_Placement>(Neu_Layout{30, 195, 700, 275, 1.0f, 0, 0});
    auto tb = std::make_shared<Neu_Textbox>(Neu_Layout{55, 215, 280, 38, 1.0f, 0, 0});
    tb->setText("Antialiased textbox");
    auto flat = std::make_shared<Neu_FlatButton>(Neu_Layout{360, 215, 220, 38, 1.0f, 0, 0});
    flat->setText("Rounded flat/menu button");
    auto list = std::make_shared<Neu_Listbox>(Neu_Layout{55, 275, 280, 150, 1.0f, 0, 0});
    list->setItems({"AA edges", "No Cairo dependency", "X11/XRender aware", "Function pointer events"});
    placement->add(tb);
    placement->add(flat);
    placement->add(list);
    window.add(placement);

    window.show();
    app.run();
    return 0;
}
