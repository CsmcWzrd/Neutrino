#include "Neutrino/Neutrino.hpp"

namespace neutrino {

namespace {

static int eventX(const XEvent& event, int fallback)
{
    if (event.type == MotionNotify) {
        return event.xmotion.x;
    }
    if (event.type == ButtonPress || event.type == ButtonRelease) {
        return event.xbutton.x;
    }
    return fallback;
}

static int eventY(const XEvent& event, int fallback)
{
    if (event.type == MotionNotify) {
        return event.xmotion.y;
    }
    if (event.type == ButtonPress || event.type == ButtonRelease) {
        return event.xbutton.y;
    }
    return fallback;
}

static void setEventPosition(XEvent& event, int x, int y)
{
    if (event.type == MotionNotify) {
        event.xmotion.x = x;
        event.xmotion.y = y;
    } else if (event.type == ButtonPress || event.type == ButtonRelease) {
        event.xbutton.x = x;
        event.xbutton.y = y;
    }
}

} // namespace

void Neu_ScrollWindow::add(std::shared_ptr<Neu_Control> child)
{
    if (!child) {
        return;
    }

    Neu_Layout childLayout = child->layout();
    const auto rect = bounds();

    // Scroll-window children are stored in content-relative coordinates.  Older
    // demos used window-absolute positions, so convert coordinates that appear
    // to be absolute when the child is added.
    if (childLayout.left >= rect.x && childLayout.top >= rect.y) {
        childLayout.left -= rect.x;
        childLayout.top -= rect.y;
        child->setLayout(childLayout);
    }

    Neu_Placement::add(child);
}

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
            shifted.left = original.left - scrollX();
            shifted.top = original.top - scrollY();
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
            maxRight = std::max(maxRight, original.left + original.width + 20);
            maxBottom = std::max(maxBottom, original.top + original.height + 20);
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
    setVirtualSize(std::max(rect.width, maxRight), std::max(rect.height, maxBottom));
    drawScrollbars(display, drawable, gc, theme);
    drawHintPopup(display, drawable, gc, theme);
}

void Neu_ScrollWindow::handleXEvent(XEvent& event)
{
    const auto rect = bounds();
    const int x = eventX(event, rect.x);
    const int y = eventY(event, rect.y);
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
        setEventPosition(adjusted, x - viewportLeft + scrollX(), y - viewportTop + scrollY());
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
