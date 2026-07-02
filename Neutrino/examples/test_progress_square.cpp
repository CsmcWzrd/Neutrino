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
    Neu_Window window(app, 640, 360, "Neutrino Test - Progress Square");
    window.create();
    auto label = std::make_shared<Neu_Label>(Neu_Layout{24, 20, 560, 32});
    label->setText("Progress traces clockwise from the center of the top edge and returns to top center.");
    window.add(label);
    for (int i = 0; i < 5; ++i) {
        auto p = std::make_shared<Neu_ProgressSquare>(Neu_Layout{40 + i * 115, 80, 100, 100});
        p->setProgress(static_cast<float>(i + 1) / 5.0f);
        window.add(p);
    }
    window.show();
    app.run();
    return 0;
}
