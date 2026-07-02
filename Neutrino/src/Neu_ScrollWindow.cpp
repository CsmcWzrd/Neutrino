#include "Neutrino/Neutrino.hpp"

namespace neutrino {

namespace {

static void offsetEvent(XEvent& event, int dx, int dy)
{
    if (event.type == MotionNotify) {
        event.xmotion.x += dx;
        event.xmotion.y += dy;
    } else if (event.type == ButtonPress || event.type == ButtonRelease) {
        event.xbutton.x += dx;
        event.xbutton.y += dy;
    }
}

} // namespace

void Neu_ScrollWindow::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(display, drawable, gc, theme);
    const auto rect = bounds();
    const int viewportLeft = rect.x + 4;
    const int viewportTop = rect.y + 4;
    const int viewportWidth = std::max(1, rect.width - 18);
    const int viewportHeight = std::max(1, rect.height - 18);

    int maxRight = rect.width;
    int maxBottom = rect.height;

    const int screen = DefaultScreen(display);
    Pixmap viewportPixmap = XCreatePixmap(display,
                                          drawable,
                                          static_cast<unsigned int>(viewportWidth),
                                          static_cast<unsigned int>(viewportHeight),
                                          static_cast<unsigned int>(DefaultDepth(display, screen)));
    XSetForeground(display, gc, Neu_Pixel(display, theme.glass));
    XFillRectangle(display, viewportPixmap, gc, 0, 0,
                   static_cast<unsigned int>(viewportWidth),
                   static_cast<unsigned int>(viewportHeight));

    XRectangle localClip{0,
                         0,
                         static_cast<unsigned short>(viewportWidth),
                         static_cast<unsigned short>(viewportHeight)};
    XSetClipRectangles(display, gc, 0, 0, &localClip, 1, Unsorted);

    for (const auto& child : children()) {
        if (child && child->visible()) {
            Neu_Layout original = child->layout();
            Neu_Layout shifted = original;
            shifted.left = original.left - scrollX() - viewportLeft;
            shifted.top = original.top - scrollY() - viewportTop;
            child->setLayout(shifted);
            const auto childRect = child->bounds();
            const bool intersects = childRect.x + childRect.width >= 0
                                    && childRect.y + childRect.height >= 0
                                    && childRect.x <= viewportWidth
                                    && childRect.y <= viewportHeight;
            if (intersects) {
                XSetClipRectangles(display, gc, 0, 0, &localClip, 1, Unsorted);
                child->draw(display, viewportPixmap, gc, theme);
                XSetClipRectangles(display, gc, 0, 0, &localClip, 1, Unsorted);
            }
            maxRight = std::max(maxRight, original.left - rect.x + original.width + 20);
            maxBottom = std::max(maxBottom, original.top - rect.y + original.height + 20);
            child->setLayout(original);
        }
    }

    XSetClipMask(display, gc, None);
    XCopyArea(display,
              viewportPixmap,
              drawable,
              gc,
              0,
              0,
              static_cast<unsigned int>(viewportWidth),
              static_cast<unsigned int>(viewportHeight),
              viewportLeft,
              viewportTop);
    XFreePixmap(display, viewportPixmap);

    setAutoScroll(true);
    setVirtualSize(std::max(virtualSize().width, maxRight), std::max(virtualSize().height, maxBottom));
    drawScrollbars(display, drawable, gc, theme);
    drawHintPopup(display, drawable, gc, theme);
}

void Neu_ScrollWindow::handleXEvent(XEvent& event)
{
    const auto rect = bounds();
    const int x = event.type == MotionNotify ? event.xmotion.x : (event.type == ButtonPress || event.type == ButtonRelease ? event.xbutton.x : rect.x);
    const int y = event.type == MotionNotify ? event.xmotion.y : (event.type == ButtonPress || event.type == ButtonRelease ? event.xbutton.y : rect.y);
    if (!contains(x, y)) {
        Neu_Control::handleXEvent(event);
        return;
    }

    Neu_Control::handleXEvent(event);
    if (event.type == ButtonPress && autoScroll()) {
        if (x >= rect.x + rect.width - 14 || y >= rect.y + rect.height - 14 || event.xbutton.button >= Button4) {
            return;
        }
    }

    const int viewportLeft = rect.x + 4;
    const int viewportTop = rect.y + 4;
    const int viewportRight = rect.x + rect.width - 14;
    const int viewportBottom = rect.y + rect.height - 14;
    if (x < viewportLeft || x > viewportRight || y < viewportTop || y > viewportBottom) {
        return;
    }

    if (event.type == MotionNotify || event.type == ButtonPress || event.type == ButtonRelease) {
        XEvent adjusted = event;
        offsetEvent(adjusted, scrollX(), scrollY());
        for (auto it = children().rbegin(); it != children().rend(); ++it) {
            const auto& child = *it;
            if (child && child->visible() && child->enabled()) {
                if ((adjusted.type == MotionNotify && child->contains(adjusted.xmotion.x, adjusted.xmotion.y))
                    || ((adjusted.type == ButtonPress || adjusted.type == ButtonRelease) && child->contains(adjusted.xbutton.x, adjusted.xbutton.y))) {
                    child->handleXEvent(adjusted);
                    return;
                }
            }
        }
    }
}

} // namespace neutrino
