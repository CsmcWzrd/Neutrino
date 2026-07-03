#include "Neutrino/Neutrino.hpp"
#include <X11/Xutil.h>
#include <cstdlib>

namespace neutrino {

static Neu_SmoothGraphicsOptions g_smoothOptions{};
static Neu_Theme g_currentDrawingTheme{};

unsigned long Neu_Pixel(Display* display, const Neu_Color& color)
{
    if (!display) {
        return (static_cast<unsigned long>(color.r) << 16)
               | (static_cast<unsigned long>(color.g) << 8)
               | color.b;
    }

    XColor xcolor{};
    xcolor.red = static_cast<unsigned short>(color.r * 257);
    xcolor.green = static_cast<unsigned short>(color.g * 257);
    xcolor.blue = static_cast<unsigned short>(color.b * 257);

    if (XAllocColor(display, DefaultColormap(display, DefaultScreen(display)), &xcolor)) {
        return xcolor.pixel;
    }

    return (static_cast<unsigned long>(color.r) << 16)
           | (static_cast<unsigned long>(color.g) << 8)
           | color.b;
}

Neu_Color Neu_PixelToColor(Display* display, unsigned long pixel)
{
    if (!display) {
        return {
            static_cast<uint8_t>((pixel >> 16) & 0xff),
            static_cast<uint8_t>((pixel >> 8) & 0xff),
            static_cast<uint8_t>(pixel & 0xff),
            255
        };
    }

    XColor xcolor{};
    xcolor.pixel = pixel;
    XQueryColor(display, DefaultColormap(display, DefaultScreen(display)), &xcolor);
    return {
        static_cast<uint8_t>(xcolor.red / 257),
        static_cast<uint8_t>(xcolor.green / 257),
        static_cast<uint8_t>(xcolor.blue / 257),
        255
    };
}

Neu_Color Neu_LightenColor(const Neu_Color& color, int amount)
{
    auto add = [amount](uint8_t value) -> uint8_t {
        return static_cast<uint8_t>(std::min(255, static_cast<int>(value) + amount));
    };
    return {add(color.r), add(color.g), add(color.b), color.a};
}

Neu_Color Neu_DarkenColor(const Neu_Color& color, int amount)
{
    auto sub = [amount](uint8_t value) -> uint8_t {
        return static_cast<uint8_t>(std::max(0, static_cast<int>(value) - amount));
    };
    return {sub(color.r), sub(color.g), sub(color.b), color.a};
}

Neu_Color Neu_MixColor(const Neu_Color& a, const Neu_Color& b, double t)
{
    t = std::max(0.0, std::min(1.0, t));
    auto mix = [t](uint8_t av, uint8_t bv) -> uint8_t {
        return static_cast<uint8_t>(std::round(static_cast<double>(av) * (1.0 - t) + static_cast<double>(bv) * t));
    };
    return {mix(a.r, b.r), mix(a.g, b.g), mix(a.b, b.b), mix(a.a, b.a)};
}

void Neu_ApplyThemeRenderingOptions(const Neu_Theme& theme)
{
    if (g_smoothOptions.vmFriendly) {
        return;
    }

    g_smoothOptions.enabled = true;
    if (theme.antiAliasMode == Neu_AntiAliasMode::SSAA) {
        g_smoothOptions.backend = Neu_GraphicsBackend::SoftwareAntialias;
        g_smoothOptions.supersample = std::max(4, std::min(8, theme.antiAliasSamples));
    } else if (theme.antiAliasMode == Neu_AntiAliasMode::MSAA) {
        g_smoothOptions.backend = Neu_GraphicsBackend::SoftwareAntialias;
        g_smoothOptions.supersample = std::max(3, std::min(6, theme.antiAliasSamples));
    } else {
        g_smoothOptions.backend = Neu_GraphicsBackend::XRenderAntialias;
        g_smoothOptions.supersample = std::max(2, std::min(4, theme.antiAliasSamples));
    }
}

void Neu_SetCurrentDrawingTheme(const Neu_Theme& theme)
{
    g_currentDrawingTheme = theme;
    Neu_ApplyThemeRenderingOptions(theme);
}

void Neu_SetSmoothGraphicsOptions(const Neu_SmoothGraphicsOptions& options)
{
    g_smoothOptions = options;
    g_smoothOptions.supersample = std::max(1, std::min(8, g_smoothOptions.supersample));
    g_smoothOptions.bufferStages = std::max(1, std::min(3, g_smoothOptions.bufferStages));

    if (g_smoothOptions.vmFriendly) {
        g_smoothOptions.enabled = false;
        g_smoothOptions.backend = Neu_GraphicsBackend::X11Basic;
        g_smoothOptions.supersample = 1;
        g_smoothOptions.drawShadows = false;
        g_smoothOptions.repaintOnMouseMove = false;
        g_smoothOptions.multiStageDoubleBuffering = true;
        g_smoothOptions.bufferStages = std::max(2, g_smoothOptions.bufferStages);
    }
}

Neu_SmoothGraphicsOptions Neu_GetSmoothGraphicsOptions()
{
    return g_smoothOptions;
}

void Neu_EnableAntialiasing(bool enabled)
{
    g_smoothOptions.enabled = enabled;
}

void Neu_EnableMultiStageDoubleBuffering(bool enabled)
{
    g_smoothOptions.multiStageDoubleBuffering = enabled;
    if (enabled && g_smoothOptions.bufferStages < 2) {
        g_smoothOptions.bufferStages = 2;
    }
}

void Neu_UseVirtualMachineFriendlyDefaults(bool enabled)
{
    if (enabled) {
        Neu_SmoothGraphicsOptions options{};
        options.enabled = false;
        options.backend = Neu_GraphicsBackend::X11Basic;
        options.supersample = 1;
        options.drawShadows = false;
        options.drawHints = true;
        options.repaintOnMouseMove = false;
        options.cacheRoundedRects = false;
        options.vmFriendly = true;
        options.multiStageDoubleBuffering = true;
        options.bufferStages = 2;
        Neu_SetSmoothGraphicsOptions(options);
        return;
    }

    Neu_SetSmoothGraphicsOptions(Neu_SmoothGraphicsOptions{});
}

static Neu_Color Neu_Blend(const Neu_Color& source, const Neu_Color& destination, double coverage)
{
    coverage = std::max(0.0, std::min(1.0, coverage)) * (source.a / 255.0);

    auto mix = [coverage](uint8_t sourceChannel, uint8_t destinationChannel) -> uint8_t {
        return static_cast<uint8_t>(std::round(sourceChannel * coverage + destinationChannel * (1.0 - coverage)));
    };

    return {
        mix(source.r, destination.r),
        mix(source.g, destination.g),
        mix(source.b, destination.b),
        255
    };
}

static double Neu_RoundedRectDistance(double pointX, double pointY, double width, double height, double radius)
{
    radius = std::max(0.0, std::min(radius, std::min(width, height) * 0.5));
    const double qx = std::abs(pointX - width * 0.5) - (width * 0.5 - radius);
    const double qy = std::abs(pointY - height * 0.5) - (height * 0.5 - radius);
    const double outside = std::hypot(std::max(qx, 0.0), std::max(qy, 0.0));
    const double inside = std::min(std::max(qx, qy), 0.0);
    return outside + inside - radius;
}

void Neu_DrawSmoothRoundedRect(Display* display,
                             Drawable drawable,
                             GC gc,
                             const Neu_Color& color,
                             const Neu_Color& background,
                             int x,
                             int y,
                             int width,
                             int height,
                             int radius,
                             bool fill,
                             int supersample)
{
    if (!display || width <= 0 || height <= 0) {
        return;
    }

    supersample = std::max(1, std::min(8, supersample));

    XImage* image = XCreateImage(display,
                                 DefaultVisual(display, DefaultScreen(display)),
                                 DefaultDepth(display, DefaultScreen(display)),
                                 ZPixmap,
                                 0,
                                 nullptr,
                                 static_cast<unsigned int>(width),
                                 static_cast<unsigned int>(height),
                                 32,
                                 0);
    if (!image) {
        return;
    }

    const size_t bytes = static_cast<size_t>(image->bytes_per_line) * static_cast<size_t>(height);
    image->data = static_cast<char*>(std::calloc(bytes, 1));
    if (!image->data) {
        XDestroyImage(image);
        return;
    }

    const double borderWidth = fill ? 0.0 : 1.45;

    for (int pixelY = 0; pixelY < height; ++pixelY) {
        for (int pixelX = 0; pixelX < width; ++pixelX) {
            int hits = 0;
            const int total = supersample * supersample;

            for (int sampleY = 0; sampleY < supersample; ++sampleY) {
                for (int sampleX = 0; sampleX < supersample; ++sampleX) {
                    const double fx = pixelX + (sampleX + 0.5) / supersample;
                    const double fy = pixelY + (sampleY + 0.5) / supersample;
                    const double distance = Neu_RoundedRectDistance(fx, fy, width - 1.0, height - 1.0, radius);

                    if ((fill && distance <= 0.0) || (!fill && std::abs(distance) <= borderWidth)) {
                        ++hits;
                    }
                }
            }

            const double coverage = static_cast<double>(hits) / static_cast<double>(total);
            XPutPixel(image, pixelX, pixelY, Neu_Pixel(display, Neu_Blend(color, background, coverage)));
        }
    }

    XPutImage(display,
              drawable,
              gc,
              image,
              0,
              0,
              x,
              y,
              static_cast<unsigned int>(width),
              static_cast<unsigned int>(height));
    XDestroyImage(image);
}

static bool Neu_HasEdgeCorners(const Neu_Theme& theme)
{
    return theme.topLeftCorner == Neu_CornerStyle::EdgeCorner
           || theme.topRightCorner == Neu_CornerStyle::EdgeCorner
           || theme.bottomLeftCorner == Neu_CornerStyle::EdgeCorner
           || theme.bottomRightCorner == Neu_CornerStyle::EdgeCorner;
}

static bool Neu_InsideThemedShape(double px,
                                  double py,
                                  double width,
                                  double height,
                                  int radius,
                                  int edge,
                                  const Neu_Theme& theme)
{
    if (width <= 0.0 || height <= 0.0) {
        return false;
    }

    const double right = width - 1.0;
    const double bottom = height - 1.0;
    const double e = std::max(0, std::min(edge, static_cast<int>(std::min(width, height) * 0.45)));

    if (theme.topLeftCorner == Neu_CornerStyle::EdgeCorner && px + py < e) {
        return false;
    }
    if (theme.topRightCorner == Neu_CornerStyle::EdgeCorner && (right - px) + py < e) {
        return false;
    }
    if (theme.bottomLeftCorner == Neu_CornerStyle::EdgeCorner && px + (bottom - py) < e) {
        return false;
    }
    if (theme.bottomRightCorner == Neu_CornerStyle::EdgeCorner && (right - px) + (bottom - py) < e) {
        return false;
    }

    const double r = std::max(0, std::min(radius, static_cast<int>(std::min(width, height) * 0.5)));
    if (r <= 0.0) {
        return true;
    }

    auto roundedInside = [r](double dx, double dy) -> bool {
        return dx * dx + dy * dy <= r * r;
    };

    if (theme.topLeftCorner == Neu_CornerStyle::RoundedCorner && px < r && py < r) {
        return roundedInside(px - r, py - r);
    }
    if (theme.topRightCorner == Neu_CornerStyle::RoundedCorner && px > right - r && py < r) {
        return roundedInside(px - (right - r), py - r);
    }
    if (theme.bottomLeftCorner == Neu_CornerStyle::RoundedCorner && px < r && py > bottom - r) {
        return roundedInside(px - r, py - (bottom - r));
    }
    if (theme.bottomRightCorner == Neu_CornerStyle::RoundedCorner && px > right - r && py > bottom - r) {
        return roundedInside(px - (right - r), py - (bottom - r));
    }

    return true;
}

static bool Neu_InsideThemedShapeInset(double px,
                                       double py,
                                       double width,
                                       double height,
                                       int radius,
                                       int edge,
                                       const Neu_Theme& theme,
                                       double inset)
{
    return Neu_InsideThemedShape(px - inset,
                                 py - inset,
                                 width - inset * 2.0,
                                 height - inset * 2.0,
                                 std::max(0, radius - static_cast<int>(inset)),
                                 std::max(0, edge - static_cast<int>(inset)),
                                 theme);
}

static Neu_Color Neu_GradientColorForY(const Neu_Color& base, int y, int height, const Neu_Theme& theme)
{
    if (!theme.gradientControls || height <= 1) {
        return base;
    }

    const double t = static_cast<double>(y) / static_cast<double>(std::max(1, height - 1));
    const Neu_Color top = Neu_MixColor(base, theme.controlGradientTop, 0.42);
    const Neu_Color bottom = Neu_MixColor(base, theme.controlGradientBottom, 0.42);
    return Neu_MixColor(top, bottom, t);
}

static void Neu_DrawThemedRectImage(Display* display,
                                    Drawable drawable,
                                    GC gc,
                                    const Neu_Color& color,
                                    const Neu_Color& background,
                                    int x,
                                    int y,
                                    int width,
                                    int height,
                                    int radius,
                                    bool fill,
                                    int supersample)
{
    if (!display || width <= 0 || height <= 0) {
        return;
    }

    supersample = std::max(1, std::min(8, supersample));

    XImage* image = XCreateImage(display,
                                 DefaultVisual(display, DefaultScreen(display)),
                                 DefaultDepth(display, DefaultScreen(display)),
                                 ZPixmap,
                                 0,
                                 nullptr,
                                 static_cast<unsigned int>(width),
                                 static_cast<unsigned int>(height),
                                 32,
                                 0);
    if (!image) {
        return;
    }

    const size_t bytes = static_cast<size_t>(image->bytes_per_line) * static_cast<size_t>(height);
    image->data = static_cast<char*>(std::calloc(bytes, 1));
    if (!image->data) {
        XDestroyImage(image);
        return;
    }

    for (int pixelY = 0; pixelY < height; ++pixelY) {
        for (int pixelX = 0; pixelX < width; ++pixelX) {
            int hits = 0;
            const int total = supersample * supersample;
            for (int sampleY = 0; sampleY < supersample; ++sampleY) {
                for (int sampleX = 0; sampleX < supersample; ++sampleX) {
                    const double fx = pixelX + (sampleX + 0.5) / supersample;
                    const double fy = pixelY + (sampleY + 0.5) / supersample;
                    const bool outer = Neu_InsideThemedShape(fx,
                                                             fy,
                                                             width,
                                                             height,
                                                             radius,
                                                             g_currentDrawingTheme.edgeSize,
                                                             g_currentDrawingTheme);
                    if (fill) {
                        if (outer) {
                            ++hits;
                        }
                    } else {
                        const bool inner = Neu_InsideThemedShapeInset(fx,
                                                                       fy,
                                                                       width,
                                                                       height,
                                                                       radius,
                                                                       g_currentDrawingTheme.edgeSize,
                                                                       g_currentDrawingTheme,
                                                                       1.65);
                        if (outer && !inner) {
                            ++hits;
                        }
                    }
                }
            }

            const double coverage = static_cast<double>(hits) / static_cast<double>(total);
            const Neu_Color source = fill ? Neu_GradientColorForY(color, pixelY, height, g_currentDrawingTheme) : color;
            XPutPixel(image, pixelX, pixelY, Neu_Pixel(display, Neu_Blend(source, background, coverage)));
        }
    }

    XPutImage(display,
              drawable,
              gc,
              image,
              0,
              0,
              x,
              y,
              static_cast<unsigned int>(width),
              static_cast<unsigned int>(height));
    XDestroyImage(image);
}

static void Neu_DrawThemedRectBasic(Display* display,
                                    Drawable drawable,
                                    GC gc,
                                    int x,
                                    int y,
                                    int width,
                                    int height,
                                    int radius,
                                    bool fill)
{
    if (!display || width <= 0 || height <= 0) {
        return;
    }

    if (!Neu_HasEdgeCorners(g_currentDrawingTheme)) {
        if (radius < 1) {
            if (fill) {
                XFillRectangle(display, drawable, gc, x, y, width, height);
            } else {
                XDrawRectangle(display, drawable, gc, x, y, width, height);
            }
            return;
        }

        const int r = std::min(radius, std::min(width, height) / 2);
        if (fill) {
            XFillRectangle(display, drawable, gc, x + r, y, width - 2 * r, height);
            XFillRectangle(display, drawable, gc, x, y + r, width, height - 2 * r);
            XFillArc(display, drawable, gc, x, y, 2 * r, 2 * r, 90 * 64, 90 * 64);
            XFillArc(display, drawable, gc, x + width - 2 * r, y, 2 * r, 2 * r, 0, 90 * 64);
            XFillArc(display, drawable, gc, x, y + height - 2 * r, 2 * r, 2 * r, 180 * 64, 90 * 64);
            XFillArc(display, drawable, gc, x + width - 2 * r, y + height - 2 * r, 2 * r, 2 * r, 270 * 64, 90 * 64);
        } else {
            XDrawArc(display, drawable, gc, x, y, 2 * r, 2 * r, 90 * 64, 90 * 64);
            XDrawArc(display, drawable, gc, x + width - 2 * r, y, 2 * r, 2 * r, 0, 90 * 64);
            XDrawArc(display, drawable, gc, x, y + height - 2 * r, 2 * r, 2 * r, 180 * 64, 90 * 64);
            XDrawArc(display, drawable, gc, x + width - 2 * r, y + height - 2 * r, 2 * r, 2 * r, 270 * 64, 90 * 64);
            XDrawLine(display, drawable, gc, x + r, y, x + width - r, y);
            XDrawLine(display, drawable, gc, x + r, y + height, x + width - r, y + height);
            XDrawLine(display, drawable, gc, x, y + r, x, y + height - r);
            XDrawLine(display, drawable, gc, x + width, y + r, x + width, y + height - r);
        }
        return;
    }

    const int e = std::max(0, std::min(g_currentDrawingTheme.edgeSize, std::min(width, height) / 2));
    XPoint pts[8]{};
    pts[0] = {static_cast<short>(x + (g_currentDrawingTheme.topLeftCorner == Neu_CornerStyle::EdgeCorner ? e : 0)), static_cast<short>(y)};
    pts[1] = {static_cast<short>(x + width - (g_currentDrawingTheme.topRightCorner == Neu_CornerStyle::EdgeCorner ? e : 0)), static_cast<short>(y)};
    pts[2] = {static_cast<short>(x + width), static_cast<short>(y + (g_currentDrawingTheme.topRightCorner == Neu_CornerStyle::EdgeCorner ? e : 0))};
    pts[3] = {static_cast<short>(x + width), static_cast<short>(y + height - (g_currentDrawingTheme.bottomRightCorner == Neu_CornerStyle::EdgeCorner ? e : 0))};
    pts[4] = {static_cast<short>(x + width - (g_currentDrawingTheme.bottomRightCorner == Neu_CornerStyle::EdgeCorner ? e : 0)), static_cast<short>(y + height)};
    pts[5] = {static_cast<short>(x + (g_currentDrawingTheme.bottomLeftCorner == Neu_CornerStyle::EdgeCorner ? e : 0)), static_cast<short>(y + height)};
    pts[6] = {static_cast<short>(x), static_cast<short>(y + height - (g_currentDrawingTheme.bottomLeftCorner == Neu_CornerStyle::EdgeCorner ? e : 0))};
    pts[7] = {static_cast<short>(x), static_cast<short>(y + (g_currentDrawingTheme.topLeftCorner == Neu_CornerStyle::EdgeCorner ? e : 0))};

    if (fill) {
        XFillPolygon(display, drawable, gc, pts, 8, Complex, CoordModeOrigin);
    } else {
        XDrawLines(display, drawable, gc, pts, 8, CoordModeOrigin);
        XDrawLine(display, drawable, gc, pts[7].x, pts[7].y, pts[0].x, pts[0].y);
    }
}

void Neu_DrawRoundedRect(Display* display,
                         Drawable drawable,
                         GC gc,
                         int x,
                         int y,
                         int width,
                         int height,
                         int radius,
                         bool fill)
{
    if (!display || width <= 0 || height <= 0) {
        return;
    }

    XGCValues values{};
    XGetGCValues(display, gc, GCForeground | GCBackground, &values);
    const Neu_Color color = Neu_PixelToColor(display, values.foreground);
    const Neu_Color background = Neu_PixelToColor(display, values.background);

    if (!fill || !g_smoothOptions.enabled || g_smoothOptions.backend == Neu_GraphicsBackend::X11Basic) {
        Neu_DrawThemedRectBasic(display, drawable, gc, x, y, width, height, radius, fill);
        return;
    }

    Neu_DrawThemedRectImage(display,
                            drawable,
                            gc,
                            color,
                            background,
                            x,
                            y,
                            width,
                            height,
                            radius,
                            fill,
                            g_smoothOptions.supersample);
}


void Neu_DrawSmoothDropShadow(Display* display,
                            Drawable drawable,
                            GC gc,
                            const Neu_Color& shadow,
                            const Neu_Color& background,
                            int x,
                            int y,
                            int width,
                            int height,
                            int radius,
                            int blur,
                            int offsetX,
                            int offsetY)
{
    if (!display || width <= 0 || height <= 0 || blur <= 0) {
        return;
    }

    const int shadowX = x + offsetX - blur;
    const int shadowY = y + offsetY - blur;
    const int shadowW = width + blur * 2;
    const int shadowH = height + blur * 2;

    XImage* image = XCreateImage(display,
                                 DefaultVisual(display, DefaultScreen(display)),
                                 DefaultDepth(display, DefaultScreen(display)),
                                 ZPixmap,
                                 0,
                                 nullptr,
                                 static_cast<unsigned int>(shadowW),
                                 static_cast<unsigned int>(shadowH),
                                 32,
                                 0);
    if (!image) {
        return;
    }

    const size_t bytes = static_cast<size_t>(image->bytes_per_line) * static_cast<size_t>(shadowH);
    image->data = static_cast<char*>(std::calloc(bytes, 1));
    if (!image->data) {
        XDestroyImage(image);
        return;
    }

    for (int py = 0; py < shadowH; ++py) {
        for (int px = 0; px < shadowW; ++px) {
            const double localX = px - blur;
            const double localY = py - blur;
            const double distance = Neu_RoundedRectDistance(localX, localY, width - 1.0, height - 1.0, radius);
            double coverage = 0.0;
            if (distance <= 0.0) {
                coverage = 0.18;
            } else if (distance < blur) {
                coverage = (1.0 - distance / static_cast<double>(blur)) * 0.18;
            }
            XPutPixel(image, px, py, Neu_Pixel(display, Neu_Blend(shadow, background, coverage)));
        }
    }

    XPutImage(display,
              drawable,
              gc,
              image,
              0,
              0,
              shadowX,
              shadowY,
              static_cast<unsigned int>(shadowW),
              static_cast<unsigned int>(shadowH));
    XDestroyImage(image);
}

} // namespace neutrino

