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
    const int y = rect.y + 8;
    const int half = side / 2;
    const int stroke = 5;

    XSetForeground(display, gc, Neu_Pixel(display, Neu_Color{215, 225, 238, 255}));
    XFillRectangle(display, drawable, gc, x, y, side, side);
    XSetForeground(display, gc, Neu_Pixel(display, theme.border));
    XDrawRectangle(display, drawable, gc, x, y, side, side);

    int remaining = static_cast<int>(std::round(4 * side * progress_));
    XSetForeground(display, gc, Neu_Pixel(display, progress_ > 0.82f ? Neu_Color{255, 225, 72, 255} : theme.accent));

    auto drawHorizontal = [&](int sx, int sy, int direction, int length) {
        if (remaining <= 0 || length <= 0) {
            return;
        }
        const int used = std::min(remaining, length);
        const int drawX = direction >= 0 ? sx : sx - used;
        XFillRectangle(display, drawable, gc, drawX, sy - stroke / 2, used, stroke);
        remaining -= used;
    };

    auto drawVertical = [&](int sx, int sy, int direction, int length) {
        if (remaining <= 0 || length <= 0) {
            return;
        }
        const int used = std::min(remaining, length);
        const int drawY = direction >= 0 ? sy : sy - used;
        XFillRectangle(display, drawable, gc, sx - stroke / 2, drawY, stroke, used);
        remaining -= used;
    };

    // Start at the center of the top edge and trace clockwise around all edges.
    drawHorizontal(x + half, y, +1, side - half);      // top center -> top right
    drawVertical(x + side, y, +1, side);               // right edge down
    drawHorizontal(x + side, y + side, -1, side);      // bottom edge right -> left
    drawVertical(x, y + side, -1, side);               // left edge up
    drawHorizontal(x, y, +1, half);                    // top left -> top center

    if (progress_ > 0.82f) {
        XSetForeground(display, gc, Neu_Pixel(display, Neu_Color{255, 255, 185, 255}));
        XDrawRectangle(display, drawable, gc, x - 3, y - 3, side + 6, side + 6);
    }
    drawText(display, drawable, gc, theme, std::to_string(static_cast<int>(progress_ * 100.0f)) + "%", x + side / 2 - 14, y + side / 2 + 4);
}

} // namespace neutrino
