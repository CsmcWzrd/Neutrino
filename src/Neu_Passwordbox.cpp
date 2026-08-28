#include "Neutrino/Neutrino.hpp"

namespace neutrino {

void Neu_Passwordbox::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    const std::string originalText = text_;
    text_ = std::string(originalText.size(), '*');
    Neu_Textbox::draw(display, drawable, gc, theme);
    text_ = originalText;
}

} // namespace neutrino
