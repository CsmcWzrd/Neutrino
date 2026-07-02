#include "Neutrino/Neutrino.hpp"

namespace neutrino {

void Neu_ComboBox::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(display, drawable, gc, theme);
    const auto rect = bounds();
    const std::string selectedText = selected_ >= 0 && selected_ < static_cast<int>(items_.size())
                                     ? items_[static_cast<size_t>(selected_)]
                                     : std::string{};

    drawText(display, drawable, gc, theme, selectedText, rect.x + 8, rect.y + rect.height / 2 + 5);
    drawText(display, drawable, gc, theme, "v", rect.x + rect.width - 18, rect.y + rect.height / 2 + 5);

    if (open_) {
        Neu_Listbox::draw(display, drawable, gc, theme);
    }
}

void Neu_ComboBox::handleXEvent(XEvent& event)
{
    if (event.type == ButtonRelease && contains(event.xbutton.x, event.xbutton.y)) {
        open_ = !open_;
    }

    if (open_) {
        Neu_Listbox::handleXEvent(event);
    }

    requestRedraw();
}

} // namespace neutrino
