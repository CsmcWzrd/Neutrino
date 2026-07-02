#include "Neutrino/Neutrino.hpp"

namespace neutrino {

void Neu_ScrollWindow::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(display, drawable, gc, theme);
    const auto rect = bounds();
    const Neu_Rect clipRect{rect.x + 2, rect.y + 2, std::max(0, rect.width - 14), std::max(0, rect.height - 14)};
    if (clipRect.width <= 0 || clipRect.height <= 0) {
        drawScrollbars(display, drawable, gc, theme);
        return;
    }

    // Draw scrolled content into an off-screen pixmap first.  This avoids child controls
    // clearing the parent's X11 clip region and leaking outside the scroll window.
    const int screen = DefaultScreen(display);
    const unsigned int depth = static_cast<unsigned int>(DefaultDepth(display, screen));
    Pixmap content = XCreatePixmap(display,
                                   drawable,
                                   static_cast<unsigned int>(clipRect.width),
                                   static_cast<unsigned int>(clipRect.height),
                                   depth);
    GC contentGc = XCreateGC(display, content, 0, nullptr);
    XSetForeground(display, contentGc, Neu_Pixel(display, theme.glass));
    XFillRectangle(display, content, contentGc, 0, 0, static_cast<unsigned int>(clipRect.width), static_cast<unsigned int>(clipRect.height));

    for (const auto& child : children()) {
        if (child && child->visible()) {
            auto original = child->layout();
            auto shifted = original;
            shifted.left = original.left - scrollX() - clipRect.x;
            shifted.top = original.top - scrollY() - clipRect.y;
            child->setLayout(shifted);
            child->draw(display, content, contentGc, theme);
            child->setLayout(original);
        }
    }

    XCopyArea(display,
              content,
              drawable,
              gc,
              0,
              0,
              static_cast<unsigned int>(clipRect.width),
              static_cast<unsigned int>(clipRect.height),
              clipRect.x,
              clipRect.y);
    XFreeGC(display, contentGc);
    XFreePixmap(display, content);

    drawScrollbars(display, drawable, gc, theme);
    drawHintPopup(display, drawable, gc, theme);
}

void Neu_ScrollWindow::handleXEvent(XEvent& event)
{
    // Route scrollbars first; then temporarily translate events into the scrolled content coordinates.
    if (handleScrollMouseEvent(event)) {
        return;
    }

    XEvent translated = event;
    if (event.type == ButtonPress || event.type == ButtonRelease) {
        translated.xbutton.x += scrollX();
        translated.xbutton.y += scrollY();
    } else if (event.type == MotionNotify) {
        translated.xmotion.x += scrollX();
        translated.xmotion.y += scrollY();
    }

    for (const auto& child : children()) {
        if (child && child->visible()) {
            child->handleXEvent(translated);
        }
    }
    Neu_Control::handleXEvent(event);
}

} // namespace neutrino
