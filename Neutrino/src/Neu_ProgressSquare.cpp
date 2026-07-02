#include "Neutrino/Neutrino.hpp"

namespace neutrino {

void Neu_ProgressSquare::setProgress(float progress)
{
    progress_ = std::max(0.0f, std::min(1.0f, progress));
    requestRedraw();
}

void Neu_ProgressSquare::draw(Display* display, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(display, drawable, gc, theme);
    const auto rect = bounds();
    const int side = std::min(rect.width, rect.height) - 16;
    const int x = rect.x + (rect.width - side) / 2;
    const int y = rect.y + 8;
    XSetForeground(display, gc, Neu_Pixel(display, Neu_Color{215, 225, 238, 255}));
    XFillRectangle(display, drawable, gc, x, y, side, side);
    XSetForeground(display, gc, Neu_Pixel(display, theme.accent));
    const int perimeter = 4 * side;
    int done = static_cast<int>(perimeter * progress_);
    auto drawPart = [&](int x1, int y1, int x2, int, int len) {
        if (done <= 0) {
            return;
        }
        int used = std::min(done, len);
        if (x1 == x2) {
            XFillRectangle(display, drawable, gc, x1 - 2, y1, 5, used);
        } else {
            XFillRectangle(display, drawable, gc, x1, y1 - 2, used, 5);
        }
        done -= used;
    };
    drawPart(x, y, x + side, y, side);
    drawPart(x + side, y, x + side, y + side, side);
    drawPart(x + side, y + side, x, y + side, side);
    drawPart(x, y + side, x, y, side);
    if (progress_ > 0.82f) {
        XSetForeground(display, gc, Neu_Pixel(display, Neu_Color{255, 255, 180, 255}));
        XDrawRectangle(display, drawable, gc, x - 3, y - 3, side + 6, side + 6);
    }
    drawText(display, drawable, gc, theme, std::to_string(static_cast<int>(progress_ * 100.0f)) + "%", x + side / 2 - 14, y + side / 2 + 4);
}

} // namespace neutrino
