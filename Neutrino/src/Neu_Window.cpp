#include "Neutrino/Neutrino.hpp"

namespace neutrino {

Neu_Window::Neu_Window(Neu_Application& app, int width, int height, const std::string& title)
    : app_(app),
      display_(app.display()),
      width_(width),
      height_(height),
      title_(title)
{
}

Neu_Window::~Neu_Window()
{
    close();
}

bool Neu_Window::create()
{
    if (!display_) {
        return false;
    }

    const int screen = DefaultScreen(display_);
    window_ = XCreateSimpleWindow(display_,
                                  RootWindow(display_, screen),
                                  80,
                                  80,
                                  width_,
                                  height_,
                                  1,
                                  BlackPixel(display_, screen),
                                  Neu_Pixel(display_, theme_.background));
    XStoreName(display_, window_, title_.c_str());
    XSelectInput(display_,
                 window_,
                 ExposureMask
                 | ButtonPressMask
                 | ButtonReleaseMask
                 | KeyPressMask
                 | PointerMotionMask
                 | LeaveWindowMask
                 | StructureNotifyMask);
    gc_ = XCreateGC(display_, window_, 0, nullptr);
    XSetBackground(display_, gc_, Neu_Pixel(display_, theme_.background));
    wmDelete_ = XInternAtom(display_, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display_, window_, &wmDelete_, 1);
    app_.registerWindow(this);
    return true;
}

void Neu_Window::show()
{
    XMapWindow(display_, window_);
}

void Neu_Window::releaseBuffers()
{
    if (!display_) {
        return;
    }

    if (stageBackground_) {
        XFreePixmap(display_, stageBackground_);
        stageBackground_ = 0;
    }

    if (stageCompose_) {
        XFreePixmap(display_, stageCompose_);
        stageCompose_ = 0;
    }

    if (stageFinal_) {
        XFreePixmap(display_, stageFinal_);
        stageFinal_ = 0;
    }

    bufferWidth_ = 0;
    bufferHeight_ = 0;
}

void Neu_Window::close()
{
    if (!window_) {
        return;
    }

    releaseBuffers();
    app_.unregisterWindow(this);
    XFreeGC(display_, gc_);
    XDestroyWindow(display_, window_);
    window_ = 0;
}

void Neu_Window::ensureBuffers()
{
    if (!display_ || !window_ || width_ <= 0 || height_ <= 0) {
        return;
    }

    if (stageBackground_ && stageCompose_ && stageFinal_ && bufferWidth_ == width_ && bufferHeight_ == height_) {
        return;
    }

    releaseBuffers();

    const int screen = DefaultScreen(display_);
    const unsigned int depth = static_cast<unsigned int>(DefaultDepth(display_, screen));
    stageBackground_ = XCreatePixmap(display_, window_, static_cast<unsigned int>(width_), static_cast<unsigned int>(height_), depth);
    stageCompose_ = XCreatePixmap(display_, window_, static_cast<unsigned int>(width_), static_cast<unsigned int>(height_), depth);
    stageFinal_ = XCreatePixmap(display_, window_, static_cast<unsigned int>(width_), static_cast<unsigned int>(height_), depth);

    if (!stageBackground_ || !stageCompose_ || !stageFinal_) {
        releaseBuffers();
        return;
    }

    bufferWidth_ = width_;
    bufferHeight_ = height_;
}

void Neu_Window::drawScene(Drawable target)
{
    XSetForeground(display_, gc_, Neu_Pixel(display_, theme_.background));
    XSetBackground(display_, gc_, Neu_Pixel(display_, theme_.background));
    XFillRectangle(display_, target, gc_, 0, 0, width_, height_);

    for (auto& control : controls_) {
        if (control->visible()) {
            control->drawShadow(display_, target, gc_, theme_);
        }
    }

    for (auto& control : controls_) {
        if (control->visible()) {
            control->draw(display_, target, gc_, theme_);
        }
    }

    for (auto& control : controls_) {
        if (control->visible()) {
            control->drawHintPopup(display_, target, gc_, theme_);
        }
    }
}

void Neu_Window::redraw()
{
    if (!window_) {
        return;
    }

    const auto options = Neu_GetSmoothGraphicsOptions();
    const bool buffered = multiStageDoubleBuffering_ && options.multiStageDoubleBuffering;
    const int stages = std::max(1, std::min(3, options.bufferStages));

    if (!buffered || stages < 2) {
        drawScene(window_);
        XFlush(display_);
        return;
    }

    ensureBuffers();
    if (!stageBackground_ || !stageCompose_) {
        drawScene(window_);
        XFlush(display_);
        return;
    }

    XSetForeground(display_, gc_, Neu_Pixel(display_, theme_.background));
    XSetBackground(display_, gc_, Neu_Pixel(display_, theme_.background));
    XFillRectangle(display_, stageBackground_, gc_, 0, 0, width_, height_);

    XCopyArea(display_, stageBackground_, stageCompose_, gc_, 0, 0, width_, height_, 0, 0);

    for (auto& control : controls_) {
        if (control->visible()) {
            control->drawShadow(display_, stageCompose_, gc_, theme_);
        }
    }

    for (auto& control : controls_) {
        if (control->visible()) {
            control->draw(display_, stageCompose_, gc_, theme_);
        }
    }

    if (stages >= 3 && stageFinal_) {
        XCopyArea(display_, stageCompose_, stageFinal_, gc_, 0, 0, width_, height_, 0, 0);

        for (auto& control : controls_) {
            if (control->visible()) {
                control->drawHintPopup(display_, stageFinal_, gc_, theme_);
            }
        }

        XCopyArea(display_, stageFinal_, window_, gc_, 0, 0, width_, height_, 0, 0);
    } else {
        for (auto& control : controls_) {
            if (control->visible()) {
                control->drawHintPopup(display_, stageCompose_, gc_, theme_);
            }
        }

        XCopyArea(display_, stageCompose_, window_, gc_, 0, 0, width_, height_, 0, 0);
    }

    XFlush(display_);
}

void Neu_Window::setMultiStageDoubleBuffering(bool enabled)
{
    multiStageDoubleBuffering_ = enabled;
    if (!enabled) {
        releaseBuffers();
    }
    redraw();
}

void Neu_Window::add(std::shared_ptr<Neu_Control> control)
{
    control->setParent(this);
    controls_.push_back(control);
}

void Neu_Window::handleXEvent(XEvent& event)
{
    if (event.type == ClientMessage && static_cast<Atom>(event.xclient.data.l[0]) == wmDelete_) {
        if (onClose_) {
            onClose_(this, closeUserData_);
        }
        close();
        return;
    }

    if (event.type == ConfigureNotify) {
        if (width_ != event.xconfigure.width || height_ != event.xconfigure.height) {
            width_ = event.xconfigure.width;
            height_ = event.xconfigure.height;
            releaseBuffers();
        }
    }

    if (event.type == Expose && event.xexpose.count == 0) {
        redraw();
    }

    for (auto& control : controls_) {
        if (control->visible()) {
            control->handleXEvent(event);
        }
    }

    const bool repaintMotion = Neu_GetSmoothGraphicsOptions().repaintOnMouseMove;
    if (event.type == ButtonRelease || event.type == KeyPress || event.type == LeaveNotify || (repaintMotion && event.type == MotionNotify)) {
        redraw();
    }
}

} // namespace neutrino
