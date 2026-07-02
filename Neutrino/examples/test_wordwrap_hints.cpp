#include "Neutrino/Neutrino.hpp"
#include <iostream>
using namespace neutrino;

int main()
{
    Neu_Application app;
    if (!app.open()) {
        std::cerr << "Unable to open display\n";
        return 1;
    }
    Neu_Window window(app, 760, 460, "Neutrino Test - Word Wrap and Hints");
    window.create();
    auto label = std::make_shared<Neu_Label>(Neu_Layout{24, 24, 320, 32});
    label->setText("Single-line label truncates long text at the control boundary when wrapping is off.");
    label->setTruncateText(true);
    window.add(label);
    auto multi = std::make_shared<Neu_MultilineLabel>(Neu_Layout{24, 76, 320, 160});
    multi->setText("Multiline labels word wrap by default. This line is intentionally long so it demonstrates wrapping inside the label rectangle instead of drawing outside the boundary.");
    window.add(multi);
    auto button = std::make_shared<Neu_Button>(Neu_Layout{380, 76, 260, 44});
    button->setText("Hover for bounded hint");
    button->setHintText("Hints are wrapped inside a maximum 400 pixel wide popup. This text is deliberately very long to verify that it stays within the hint boundary, shows the dropdown marker, and draws the vertical scrollbar when the natural height exceeds the maximum height." );
    window.add(button);
    window.show();
    app.run();
    return 0;
}
