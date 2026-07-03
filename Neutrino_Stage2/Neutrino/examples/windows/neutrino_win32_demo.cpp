#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Neutrino/Neutrino.hpp"
#include <memory>

using namespace neutrino;

static void on_button(Neu_Control* sender, void*)
{
    sender->setText("Clicked from Win32 callback");
}

static int Neu_RunWin32Demo()
{
    Neu_Application app;
    app.open();

    Neu_Window window(app, 900, 620, "Neutrino Win32 Demo");
    window.create();
    window.setTheme(Neu_Theme::MaterialDark());

    Neu_Callbacks callbacks;
    callbacks.onClick = on_button;

    auto title = std::make_shared<Neu_Label>(Neu_Layout{20, 20, 420, 34});
    title->setText("Neutrino native Win32/GDI backend");
    title->setHintText("This demo uses the same Neu_ C++17 API with basic Win32 C-style calls underneath.");
    window.add(title);

    auto button = std::make_shared<Neu_Button>(Neu_Layout{20, 70, 260, 42});
    button->setText("Neu_Button with callback");
    button->setIconBmp("assets/icons/button_icon.bmp");
    button->setCallbacks(callbacks);
    window.add(button);

    auto edit = std::make_shared<Neu_Textbox>(Neu_Layout{20, 130, 320, 38});
    edit->setText("Editable Neu_Textbox");
    window.add(edit);

    auto pass = std::make_shared<Neu_Passwordbox>(Neu_Layout{20, 180, 320, 38});
    pass->setText("password");
    window.add(pass);

    auto list = std::make_shared<Neu_Listbox>(Neu_Layout{370, 70, 260, 220});
    list->setAutoScroll(true);
    list->setItems({"Windows", "Linux", "X11", "GDI", "Double buffered", "BMP icons", "Neu_ prefix"});
    window.add(list);

    auto progress = std::make_shared<Neu_ProgressSquare>(Neu_Layout{660, 70, 120, 120});
    progress->setProgress(0.88f);
    progress->setHintText("Neu_ProgressSquare highlights as it approaches completion.");
    window.add(progress);

    auto rich = std::make_shared<Neu_ReadOnlyRichText>(Neu_Layout{20, 330, 820, 210});
    rich->setAutoScroll(true);
    rich->setIconList({"assets/icons/sample_icon.bmp", "assets/icons/menu_icon.bmp", "assets/icons/save_icon.bmp"});
    rich->addLabel("# Label subcomponent with icon 1");
    rich->addMultilineLabel("## Multiline label subcomponent with icon 2\nEscaped hash: \\# remains visible.");
    window.add(rich);

    window.show();
    app.run();
    return 0;
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    return Neu_RunWin32Demo();
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    return Neu_RunWin32Demo();
}
