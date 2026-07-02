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
    const int side = std::max(8, std::min(rect.width, rect.height) - 18);
    const int x = rect.x + (rect.width - side) / 2;
    const int y = rect.y + (rect.height - side) / 2;
    const int cx = x + side / 2;
    const int perimeter = side / 2 + side + side + side + side / 2;
    int remaining = static_cast<int>(std::round(perimeter * progress_));

    XSetForeground(display, gc, Neu_Pixel(display, Neu_Color{215, 225, 238, 255}));
    XDrawRectangle(display, drawable, gc, x, y, side, side);

    XSetLineAttributes(display, gc, 5, LineSolid, CapRound, JoinRound);
    XSetForeground(display, gc, Neu_Pixel(display, progress_ > 0.88f ? Neu_Color{255, 215, 60, 255} : theme.accent));

    auto drawSegment = [&](int x1, int y1, int x2, int y2) {
        if (remaining <= 0) {
            return;
        }
        const int len = std::max(std::abs(x2 - x1), std::abs(y2 - y1));
        const int used = std::min(remaining, len);
        int ex = x1;
        int ey = y1;
        if (len > 0) {
            ex = x1 + (x2 - x1) * used / len;
            ey = y1 + (y2 - y1) * used / len;
        }
        XDrawLine(display, drawable, gc, x1, y1, ex, ey);
        remaining -= used;
    };

    // Start at the top-center, travel clockwise, and return to the top-center.
    drawSegment(cx, y, x + side, y);
    drawSegment(x + side, y, x + side, y + side);
    drawSegment(x + side, y + side, x, y + side);
    drawSegment(x, y + side, x, y);
    drawSegment(x, y, cx, y);

    if (progress_ > 0.82f) {
        XSetForeground(display, gc, Neu_Pixel(display, Neu_Color{255, 255, 180, 255}));
        XDrawRectangle(display, drawable, gc, x - 4, y - 4, side + 8, side + 8);
    }
    XSetLineAttributes(display, gc, 1, LineSolid, CapButt, JoinMiter);
    drawText(display,
             drawable,
             gc,
             theme,
             std::to_string(static_cast<int>(progress_ * 100.0f)) + "%",
             x + side / 2 - 16,
             y + side / 2 + 5);
    drawHintPopup(display, drawable, gc, theme);
}

} // namespace neutrino
