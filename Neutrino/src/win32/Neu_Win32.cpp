#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Neutrino/Neutrino.hpp"
#include <windowsx.h>
#include <commctrl.h>
#include <algorithm>
#include <cwchar>
#include <sstream>

namespace neutrino {

static Neu_SmoothGraphicsOptions g_options{};
static const wchar_t* kNeuWindowClass = L"Neutrino_Neu_Window";
static HINSTANCE g_instance = nullptr;
static bool g_windowClassRegistered = false;
static HFONT g_uiFont = nullptr;

static std::wstring toWide(const std::string& s)
{
    if (s.empty()) {
        return L"";
    }

    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    UINT codePage = CP_UTF8;
    if (n <= 0) {
        codePage = CP_ACP;
        n = MultiByteToWideChar(codePage, 0, s.c_str(), -1, nullptr, 0);
    }

    if (n <= 0) {
        return L"";
    }

    std::wstring w(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(codePage, 0, s.c_str(), -1, &w[0], n);
    if (!w.empty() && w.back() == L'\0') {
        w.pop_back();
    }
    return w;
}

static std::string utf8FromWide(const wchar_t* text, int count)
{
    if (!text || count <= 0) {
        return {};
    }

    int n = WideCharToMultiByte(CP_UTF8, 0, text, count, nullptr, 0, nullptr, nullptr);
    if (n <= 0) {
        return {};
    }

    std::string out(static_cast<size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, count, &out[0], n, nullptr, nullptr);
    return out;
}

static std::string unescapeHashesWin32(const std::string& text)
{
    std::string output;
    output.reserve(text.size());
    bool escaped = false;
    for (char ch : text) {
        if (escaped) {
            output.push_back(ch);
            escaped = false;
            continue;
        }
        if (ch == '\\') {
            escaped = true;
            continue;
        }
        output.push_back(ch);
    }
    if (escaped) {
        output.push_back('\\');
    }
    return output;
}


static COLORREF rgb(const Neu_Color& c)
{
    return RGB(c.r, c.g, c.b);
}

static HFONT uiFont()
{
    if (!g_uiFont) {
        g_uiFont = CreateFontW(-16,
                               0,
                               0,
                               0,
                               FW_NORMAL,
                               FALSE,
                               FALSE,
                               FALSE,
                               DEFAULT_CHARSET,
                               OUT_OUTLINE_PRECIS,
                               CLIP_DEFAULT_PRECIS,
                               CLEARTYPE_QUALITY,
                               VARIABLE_PITCH | FF_DONTCARE,
                               L"Segoe UI");
    }
    return g_uiFont;
}

static void fillRound(HDC hdc, const Neu_Rect& r, int radius, COLORREF fill, COLORREF border)
{
    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ oldBrush = SelectObject(hdc, brush);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    RoundRect(hdc, r.x, r.y, r.x + r.width, r.y + r.height, radius * 2, radius * 2);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
}

static uint16_t read16(const std::vector<uint8_t>& bytes, size_t offset)
{
    if (offset + 1 >= bytes.size()) {
        return 0;
    }
    return static_cast<uint16_t>(bytes[offset] | (bytes[offset + 1] << 8));
}

static uint32_t read32(const std::vector<uint8_t>& bytes, size_t offset)
{
    if (offset + 3 >= bytes.size()) {
        return 0;
    }
    return static_cast<uint32_t>(bytes[offset])
           | (static_cast<uint32_t>(bytes[offset + 1]) << 8)
           | (static_cast<uint32_t>(bytes[offset + 2]) << 16)
           | (static_cast<uint32_t>(bytes[offset + 3]) << 24);
}

static void clientPointFromLParam(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, int& x, int& y)
{
    if (msg == WM_MOUSEWHEEL || msg == WM_MOUSEHWHEEL) {
        POINT pt{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        ScreenToClient(hwnd, &pt);
        x = pt.x;
        y = pt.y;
    } else if (msg == WM_KEYDOWN || msg == WM_CHAR) {
        (void)wp;
        POINT pt{};
        GetCursorPos(&pt);
        ScreenToClient(hwnd, &pt);
        x = pt.x;
        y = pt.y;
    } else {
        x = GET_X_LPARAM(lp);
        y = GET_Y_LPARAM(lp);
    }
}

static LRESULT CALLBACK Neu_WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    Neu_Window* win = reinterpret_cast<Neu_Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        win = reinterpret_cast<Neu_Window*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(win));
    }

    if (!win) {
        return DefWindowProcW(hwnd, msg, wp, lp);
    }

    switch (msg) {
    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT: {
        PAINTSTRUCT ps{};
        BeginPaint(hwnd, &ps);
        win->paint(ps.hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_KEYDOWN:
    case WM_CHAR:
    case WM_MOUSELEAVE:
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
    case WM_SIZE:
    case WM_CLOSE: {
        XEvent ev{};
        ev.message = msg;
        ev.wParam = wp;
        ev.lParam = lp;
        clientPointFromLParam(hwnd, msg, wp, lp, ev.x, ev.y);
        win->handleXEvent(ev);
        return 0;
    }

    case WM_NCDESTROY:
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        return DefWindowProcW(hwnd, msg, wp, lp);

    default:
        return DefWindowProcW(hwnd, msg, wp, lp);
    }
}

Neu_Application* Neu_Application::current_ = nullptr;

Neu_Application::Neu_Application()
{
    current_ = this;
}

Neu_Application::~Neu_Application()
{
    if (g_uiFont) {
        DeleteObject(g_uiFont);
        g_uiFont = nullptr;
    }
    current_ = nullptr;
}

bool Neu_Application::open()
{
    g_instance = GetModuleHandleW(nullptr);
    SetProcessDPIAware();

    INITCOMMONCONTROLSEX icc{};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);

    if (!g_windowClassRegistered) {
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.style = CS_DBLCLKS;
        wc.lpfnWndProc = Neu_WindowProc;
        wc.hInstance = g_instance;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr;
        wc.lpszClassName = kNeuWindowClass;

        if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            return false;
        }
        g_windowClassRegistered = true;
    }

    const char* vmMode = std::getenv("NEUTRINO_VM_MODE");
    const char* fastMode = std::getenv("NEUTRINO_FAST_RENDER");
    if ((vmMode && std::strcmp(vmMode, "0") != 0) || (fastMode && std::strcmp(fastMode, "0") != 0)) {
        Neu_UseVirtualMachineFriendlyDefaults(true);
    }

    return true;
}

void Neu_Application::run()
{
    running_ = true;
    MSG msg{};
    while (running_ && GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void Neu_Application::quit()
{
    running_ = false;
    PostQuitMessage(0);
}

void Neu_Application::registerWindow(Neu_Window* win)
{
    if (win && std::find(windows_.begin(), windows_.end(), win) == windows_.end()) {
        windows_.push_back(win);
    }
}

void Neu_Application::unregisterWindow(Neu_Window* win)
{
    windows_.erase(std::remove(windows_.begin(), windows_.end(), win), windows_.end());
}

Neu_Application* Neu_Application::current()
{
    return current_;
}

bool Neu_Application::detectXRender()
{
    return false;
}

unsigned long Neu_Pixel(Display*, const Neu_Color& color)
{
    return static_cast<unsigned long>(rgb(color));
}

Neu_Color Neu_PixelToColor(Display*, unsigned long pixel)
{
    return Neu_Color{static_cast<uint8_t>(GetRValue(pixel)),
                     static_cast<uint8_t>(GetGValue(pixel)),
                     static_cast<uint8_t>(GetBValue(pixel)),
                     255};
}

void Neu_SetSmoothGraphicsOptions(const Neu_SmoothGraphicsOptions& options)
{
    g_options = options;
}

Neu_SmoothGraphicsOptions Neu_GetSmoothGraphicsOptions()
{
    return g_options;
}

void Neu_EnableAntialiasing(bool enabled)
{
    g_options.enabled = enabled;
}

void Neu_UseVirtualMachineFriendlyDefaults(bool enabled)
{
    g_options.vmFriendly = enabled;
    if (enabled) {
        g_options.drawShadows = false;
        g_options.supersample = 1;
        g_options.repaintOnMouseMove = false;
        g_options.bufferStages = 2;
    }
}

void Neu_EnableMultiStageDoubleBuffering(bool enabled)
{
    g_options.multiStageDoubleBuffering = enabled;
}

void Neu_DrawRoundedRect(Display*, Drawable drawable, GC, int x, int y, int w, int h, int radius, bool fill)
{
    Neu_Rect r{x, y, w, h};
    COLORREF border = RGB(110, 130, 160);
    COLORREF color = fill ? RGB(236, 244, 255) : RGB(255, 255, 255);
    fillRound(drawable, r, radius, color, border);
}

void Neu_DrawSmoothRoundedRect(Display*, Drawable drawable, GC, const Neu_Color& color, const Neu_Color&, int x, int y, int w, int h, int radius, bool, int)
{
    fillRound(drawable, Neu_Rect{x, y, w, h}, radius, rgb(color), RGB(110, 130, 160));
}

void Neu_DrawSmoothDropShadow(Display*, Drawable drawable, GC, const Neu_Color& shadow, const Neu_Color&, int x, int y, int w, int h, int radius, int, int offsetX, int offsetY)
{
    if (!g_options.drawShadows) {
        return;
    }

    HBRUSH brush = CreateSolidBrush(RGB(shadow.r, shadow.g, shadow.b));
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(shadow.r, shadow.g, shadow.b));
    HGDIOBJ oldBrush = SelectObject(drawable, brush);
    HGDIOBJ oldPen = SelectObject(drawable, pen);
    RoundRect(drawable, x + offsetX, y + offsetY, x + offsetX + w, y + offsetY + h, radius * 2, radius * 2);
    SelectObject(drawable, oldBrush);
    SelectObject(drawable, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
}

Neu_Theme Neu_Theme::Light()
{
    return Neu_Theme{};
}

Neu_Theme Neu_Theme::Dark()
{
    Neu_Theme t{};
    t.background = {32, 36, 42, 255};
    t.glass = {52, 58, 70, 230};
    t.text = {245, 245, 245, 255};
    t.border = {95, 118, 150, 255};
    t.hover = {72, 85, 105, 255};
    return t;
}

Neu_Theme Neu_Theme::BlueGlass()
{
    Neu_Theme t{};
    t.background = {232, 242, 255, 255};
    t.glass = {218, 236, 255, 230};
    t.accent = {35, 115, 220, 255};
    return t;
}

bool Neu_IconBmp::load(const std::string& path)
{
    HANDLE file = CreateFileW(toWide(path).c_str(),
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              nullptr,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD size = GetFileSize(file, nullptr);
    if (size == INVALID_FILE_SIZE || size < 54) {
        CloseHandle(file);
        return false;
    }

    std::vector<uint8_t> bytes(size);
    DWORD read = 0;
    const BOOL ok = ReadFile(file, bytes.data(), size, &read, nullptr);
    CloseHandle(file);

    if (!ok || read < 54 || bytes[0] != 'B' || bytes[1] != 'M') {
        return false;
    }

    const uint32_t off = read32(bytes, 10);
    const int32_t bw = static_cast<int32_t>(read32(bytes, 18));
    const int32_t bh = static_cast<int32_t>(read32(bytes, 22));
    const uint16_t bpp = read16(bytes, 28);
    if (bw <= 0 || bh == 0 || (bpp != 24 && bpp != 32)) {
        return false;
    }

    w_ = bw;
    h_ = std::abs(bh);
    pixels_.assign(static_cast<size_t>(w_ * h_), 0xff000000u);
    const int bytesPerPixel = bpp / 8;
    const int stride = ((w_ * bpp + 31) / 32) * 4;
    const bool bottomUp = bh > 0;

    if (off + static_cast<uint32_t>(stride * h_) > read) {
        return false;
    }

    for (int y = 0; y < h_; ++y) {
        const int sy = bottomUp ? (h_ - 1 - y) : y;
        const uint8_t* row = bytes.data() + off + static_cast<size_t>(sy * stride);
        for (int x = 0; x < w_; ++x) {
            const uint8_t b = row[x * bytesPerPixel + 0];
            const uint8_t g = row[x * bytesPerPixel + 1];
            const uint8_t r = row[x * bytesPerPixel + 2];
            const uint8_t a = bpp == 32 ? row[x * 4 + 3] : 255;
            pixels_[static_cast<size_t>(y * w_ + x)] = (static_cast<uint32_t>(a) << 24)
                                                       | (static_cast<uint32_t>(r) << 16)
                                                       | (static_cast<uint32_t>(g) << 8)
                                                       | b;
        }
    }

    return true;
}

Neu_TypedValue Neu_TypeInterpreter::interpret(const std::string& text)
{
    Neu_TypedValue v{};
    v.original = text;
    if (text == "true" || text == "1:true" || text == "checkbox:1") {
        v.type = Neu_CellType::Boolean;
        v.value = true;
    } else if (text == "false" || text == "0:false" || text == "checkbox:0") {
        v.type = Neu_CellType::Boolean;
        v.value = false;
    } else if (text.rfind("0x", 0) == 0 || text.rfind("hex:", 0) == 0) {
        v.type = Neu_CellType::Hex;
        v.value = text;
    } else if (text.rfind("image:", 0) == 0) {
        v.type = Neu_CellType::ImageBmp;
        v.value = text.substr(6);
    } else {
        char* end = nullptr;
        const double d = std::strtod(text.c_str(), &end);
        if (end && *end == '\0' && text.find('.') != std::string::npos) {
            v.type = Neu_CellType::Double;
            v.value = d;
        } else {
            v.type = Neu_CellType::String;
            v.value = text;
        }
    }
    return v;
}

Neu_Control::Neu_Control(Neu_Layout layout)
    : layout_(layout)
{
}

bool Neu_Control::contains(int x, int y) const
{
    Neu_Rect r = bounds();
    return x >= r.x && y >= r.y && x < r.x + r.width && y < r.y + r.height;
}

void Neu_Control::setText(const std::string& text)
{
    text_ = text;
    invokeTextChanged();
    requestRedraw();
}

void Neu_Control::setScrollOffset(int x, int y)
{
    const auto r = bounds();
    const int maxX = std::max(0, virtualSize_.width - r.width);
    const int maxY = std::max(0, virtualSize_.height - r.height);
    scrollX_ = std::max(0, std::min(x, maxX));
    scrollY_ = std::max(0, std::min(y, maxY));
    if (callbacks_.onScroll) {
        callbacks_.onScroll(this, scrollX_, scrollY_, callbacks_.userData);
    }
    requestRedraw();
}

void Neu_Control::setVirtualSize(int width, int height)
{
    virtualSize_ = {std::max(0, width), std::max(0, height)};
    const auto r = bounds();
    scrollX_ = std::max(0, std::min(scrollX_, std::max(0, virtualSize_.width - r.width)));
    scrollY_ = std::max(0, std::min(scrollY_, std::max(0, virtualSize_.height - r.height)));
}

void Neu_Control::requestRedraw()
{
    if (parent_) {
        parent_->requestRedraw();
    }
}

void Neu_Control::drawText(Display*, Drawable drawable, GC, const Neu_Theme& theme, const std::string& s, int x, int y)
{
    SetBkMode(drawable, TRANSPARENT);
    SetTextColor(drawable, rgb(theme.text));
    HFONT font = uiFont();
    HGDIOBJ old = SelectObject(drawable, font);
    std::wstring w = toWide(s);
    TextOutW(drawable, x, y, w.c_str(), static_cast<int>(w.size()));
    SelectObject(drawable, old);
}

void Neu_Control::drawIconBmp(Display*, Drawable drawable, GC, int x, int y, int maxSize)
{
    if (icon_.width() <= 0 || icon_.height() <= 0 || maxSize <= 0) {
        return;
    }

    const int sz = std::min(maxSize, std::min(icon_.width(), icon_.height()));
    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = icon_.width();
    bmi.bmiHeader.biHeight = -icon_.height();
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    StretchDIBits(drawable,
                  x,
                  y,
                  sz,
                  sz,
                  0,
                  0,
                  icon_.width(),
                  icon_.height(),
                  icon_.pixels().data(),
                  &bmi,
                  DIB_RGB_COLORS,
                  SRCCOPY);
}

void Neu_Control::drawShadow(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    if (!g_options.drawShadows) {
        return;
    }
    Neu_Rect r = bounds();
    Neu_DrawSmoothDropShadow(d,
                             drawable,
                             gc,
                             theme.shadow,
                             theme.background,
                             r.x,
                             r.y,
                             r.width,
                             r.height,
                             theme.radius,
                             theme.shadowSize,
                             theme.shadowOffsetX,
                             theme.shadowOffsetY);
}

void Neu_Control::drawHintPopup(Display*, Drawable drawable, GC, const Neu_Theme& theme)
{
    if (!g_options.drawHints || !hover_ || hintText_.empty()) {
        return;
    }

    Neu_Rect r = bounds();
    RECT box{r.x, r.y + r.height + 6, r.x + 400, r.y + r.height + 86};
    HBRUSH b = CreateSolidBrush(rgb(theme.hintBackground));
    FillRect(drawable, &box, b);
    DeleteObject(b);
    FrameRect(drawable, &box, reinterpret_cast<HBRUSH>(GetStockObject(GRAY_BRUSH)));
    drawText(nullptr, drawable, drawable, theme, hintText_, box.left + 8, box.top + 8);
}

void Neu_Control::drawScrollbars(Display*, Drawable drawable, GC, const Neu_Theme& theme)
{
    if (!autoScroll_) {
        return;
    }

    Neu_Rect r = bounds();
    const bool needVertical = virtualSize_.height > r.height;
    const bool needHorizontal = virtualSize_.width > r.width;
    HBRUSH track = CreateSolidBrush(RGB(205, 215, 226));
    HBRUSH thumb = CreateSolidBrush(rgb(theme.accent));

    if (needVertical) {
        const int trackX = r.x + r.width - 10;
        const int trackY = r.y + 8;
        const int trackH = std::max(1, r.height - 16);
        const int thumbH = std::max(20, trackH * r.height / std::max(1, virtualSize_.height));
        const int maxY = std::max(1, virtualSize_.height - r.height);
        const int thumbY = trackY + (trackH - thumbH) * scrollY_ / maxY;
        RECT tr{trackX, trackY, trackX + 5, trackY + trackH};
        RECT th{trackX - 1, thumbY, trackX + 6, thumbY + thumbH};
        FillRect(drawable, &tr, track);
        FillRect(drawable, &th, thumb);
    }

    if (needHorizontal) {
        const int trackX = r.x + 8;
        const int trackY = r.y + r.height - 10;
        const int trackW = std::max(1, r.width - 16);
        const int thumbW = std::max(20, trackW * r.width / std::max(1, virtualSize_.width));
        const int maxX = std::max(1, virtualSize_.width - r.width);
        const int thumbX = trackX + (trackW - thumbW) * scrollX_ / maxX;
        RECT tr{trackX, trackY, trackX + trackW, trackY + 5};
        RECT th{thumbX, trackY - 1, thumbX + thumbW, trackY + 6};
        FillRect(drawable, &tr, track);
        FillRect(drawable, &th, thumb);
    }

    DeleteObject(track);
    DeleteObject(thumb);
}

void Neu_Control::invokeClick()
{
    if (callbacks_.onClick) {
        callbacks_.onClick(this, callbacks_.userData);
    }
}

void Neu_Control::invokeTextChanged()
{
    if (callbacks_.onTextChanged) {
        callbacks_.onTextChanged(this, text_.c_str(), callbacks_.userData);
    }
}

void Neu_Control::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Rect r = bounds();
    fillRound(drawable, r, theme.radius, rgb(hover_ ? theme.hover : theme.glass), rgb(focused_ ? theme.accent : theme.border));
    if (focused_) {
        HPEN pen = CreatePen(PS_SOLID, 1, rgb(theme.accent));
        HGDIOBJ oldPen = SelectObject(drawable, pen);
        HGDIOBJ oldBrush = SelectObject(drawable, GetStockObject(NULL_BRUSH));
        RoundRect(drawable, r.x + 2, r.y + 2, r.x + r.width - 2, r.y + r.height - 2, theme.radius * 2, theme.radius * 2);
        SelectObject(drawable, oldBrush);
        SelectObject(drawable, oldPen);
        DeleteObject(pen);
    }
    drawIconBmp(d, drawable, gc, r.x + 6, r.y + 6, r.height - 12);
    drawText(d, drawable, gc, theme, text_, r.x + (icon_.width() > 0 ? r.height : 10), r.y + 8);
    drawScrollbars(d, drawable, gc, theme);
}

void Neu_Control::handleXEvent(XEvent& ev)
{
    const bool inside = contains(ev.x, ev.y);

    if (ev.message == WM_MOUSEMOVE) {
        if (inside && !hover_) {
            hover_ = true;
            if (callbacks_.onFocus) {
                callbacks_.onFocus(this, callbacks_.userData);
            }
            requestRedraw();
        } else if (!inside && hover_) {
            hover_ = false;
            if (callbacks_.onBlur) {
                callbacks_.onBlur(this, callbacks_.userData);
            }
            requestRedraw();
        }
    } else if (ev.message == WM_MOUSELEAVE) {
        if (hover_) {
            hover_ = false;
            if (callbacks_.onBlur) {
                callbacks_.onBlur(this, callbacks_.userData);
            }
            requestRedraw();
        }
    } else if (ev.message == WM_LBUTTONDOWN && inside) {
        pressed_ = true;
        requestRedraw();
    } else if (ev.message == WM_LBUTTONUP) {
        const bool wasPressed = pressed_;
        pressed_ = false;
        if (inside && wasPressed) {
            invokeClick();
        }
        if (wasPressed) {
            requestRedraw();
        }
    } else if (ev.message == WM_MOUSEWHEEL && autoScroll_) {
        const int delta = GET_WHEEL_DELTA_WPARAM(ev.wParam) / WHEEL_DELTA;
        if (inside || hover_ || focused_) {
            if (GET_KEYSTATE_WPARAM(ev.wParam) & MK_SHIFT) {
                setScrollOffset(scrollX_ - delta * 48, scrollY_);
            } else {
                setScrollOffset(scrollX_, scrollY_ - delta * 48);
            }
        }
    } else if (ev.message == WM_MOUSEHWHEEL && autoScroll_) {
        const int delta = GET_WHEEL_DELTA_WPARAM(ev.wParam) / WHEEL_DELTA;
        if (inside || hover_ || focused_) {
            setScrollOffset(scrollX_ + delta * 48, scrollY_);
        }
    } else if (ev.message == WM_KEYDOWN && focused_ && callbacks_.onKeyDown) {
        callbacks_.onKeyDown(this, static_cast<KeySym>(ev.wParam), 0, callbacks_.userData);
    }
}

void Neu_Button::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
}

void Neu_Button::handleXEvent(XEvent& ev)
{
    Neu_Control::handleXEvent(ev);
}

void Neu_Textbox::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    Neu_Rect r = bounds();
    const int caretX = r.x + 10 + static_cast<int>(std::min(cursor_, text_.size())) * 8;
    if (focused_) {
        HPEN pen = CreatePen(PS_SOLID, 1, rgb(theme.accent));
        HGDIOBJ old = SelectObject(drawable, pen);
        MoveToEx(drawable, caretX, r.y + 8, nullptr);
        LineTo(drawable, caretX, r.y + r.height - 8);
        SelectObject(drawable, old);
        DeleteObject(pen);
    }
}

void Neu_Textbox::handleXEvent(XEvent& ev)
{
    Neu_Control::handleXEvent(ev);

    if (ev.message == WM_LBUTTONDOWN && contains(ev.x, ev.y)) {
        cursor_ = text_.size();
        requestRedraw();
        return;
    }

    if (!focused_ || !enabled_) {
        return;
    }

    if (ev.message == WM_KEYDOWN) {
        switch (ev.wParam) {
        case VK_LEFT:
            if (cursor_ > 0) {
                --cursor_;
            }
            break;
        case VK_RIGHT:
            if (cursor_ < text_.size()) {
                ++cursor_;
            }
            break;
        case VK_HOME:
            cursor_ = 0;
            break;
        case VK_END:
            cursor_ = text_.size();
            break;
        case VK_DELETE:
            if (cursor_ < text_.size()) {
                text_.erase(cursor_, 1);
                invokeTextChanged();
            }
            break;
        default:
            break;
        }
        requestRedraw();
        return;
    }

    if (ev.message == WM_CHAR) {
        wchar_t ch = static_cast<wchar_t>(ev.wParam);
        if (ch == L'\b') {
            if (cursor_ > 0 && !text_.empty()) {
                text_.erase(cursor_ - 1, 1);
                --cursor_;
                invokeTextChanged();
            }
        } else if (ch == L'\r' || ch == L'\n') {
            // Single-line text boxes ignore Enter.
        } else if (ch >= 32) {
            const std::string utf8 = utf8FromWide(&ch, 1);
            text_.insert(cursor_, utf8);
            cursor_ += utf8.size();
            invokeTextChanged();
        }
        requestRedraw();
    }
}

void Neu_Passwordbox::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    const std::string old = text_;
    text_.assign(old.size(), '*');
    Neu_Textbox::draw(d, drawable, gc, theme);
    text_ = old;
}

void Neu_Multilinetextbox::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    Neu_Rect r = bounds();
    std::stringstream stream(text_);
    std::string line;
    int y = r.y + 8 - scrollY_;
    int count = 0;
    while (std::getline(stream, line) && y < r.y + r.height - 10) {
        if (y >= r.y + 6) {
            drawText(d, drawable, gc, theme, line, r.x + 10 - scrollX_, y);
        }
        y += 20;
        ++count;
    }
    setVirtualSize(std::max(r.width, 800), std::max(r.height, count * 20 + 16));
    drawScrollbars(d, drawable, gc, theme);
}

void Neu_Multilinetextbox::handleXEvent(XEvent& ev)
{
    if (focused_ && ev.message == WM_CHAR && (ev.wParam == L'\r' || ev.wParam == L'\n')) {
        const size_t insertAt = std::min(cursor_, text_.size());
        text_.insert(insertAt, 1, '\n');
        cursor_ = insertAt + 1;
        invokeTextChanged();
        requestRedraw();
        return;
    }
    Neu_Textbox::handleXEvent(ev);
}

void Neu_Listbox::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    Neu_Rect r = bounds();
    constexpr int rowHeight = 22;
    setVirtualSize(r.width, std::max(r.height, static_cast<int>(items_.size()) * rowHeight + 16));
    int y = r.y + 8 - scrollY_;
    for (size_t i = 0; i < items_.size() && y < r.y + r.height - 10; ++i, y += rowHeight) {
        if (y < r.y + 6) {
            continue;
        }
        if (static_cast<int>(i) == selected_) {
            RECT sr{r.x + 4, y - 2, r.x + r.width - 14, y + 20};
            HBRUSH b = CreateSolidBrush(rgb(theme.hover));
            FillRect(drawable, &sr, b);
            DeleteObject(b);
        }
        drawText(d, drawable, gc, theme, items_[i], r.x + 10, y);
    }
    drawScrollbars(d, drawable, gc, theme);
}

void Neu_Listbox::handleXEvent(XEvent& ev)
{
    Neu_Control::handleXEvent(ev);
    if (ev.message == WM_LBUTTONUP && contains(ev.x, ev.y)) {
        Neu_Rect r = bounds();
        selected_ = std::max(0, (ev.y - r.y + scrollY_) / 22);
        if (selected_ >= static_cast<int>(items_.size())) {
            selected_ = static_cast<int>(items_.size()) - 1;
        }
        if (callbacks_.onSelectionChanged && selected_ >= 0) {
            callbacks_.onSelectionChanged(this,
                                          selected_,
                                          0,
                                          items_[static_cast<size_t>(selected_)].c_str(),
                                          callbacks_.userData);
        }
        requestRedraw();
    }
}

void Neu_ComboBox::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Listbox::draw(d, drawable, gc, theme);
}

void Neu_ComboBox::handleXEvent(XEvent& ev)
{
    Neu_Listbox::handleXEvent(ev);
}

Neu_TypedValue Neu_ListView::cellValue(size_t r, size_t c) const
{
    if (!model_ || r >= model_->size() || c >= (*model_)[r].size()) {
        return Neu_TypeInterpreter::interpret("");
    }
    return Neu_TypeInterpreter::interpret((*model_)[r][c]);
}

void Neu_ListView::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    if (!model_) {
        return;
    }

    Neu_Rect r = bounds();
    constexpr int rowHeight = 22;
    constexpr int columnWidth = 110;
    size_t maxCols = 0;
    for (const auto& row : *model_) {
        maxCols = std::max(maxCols, row.size());
    }
    setVirtualSize(std::max(r.width, static_cast<int>(maxCols) * columnWidth + 20),
                   std::max(r.height, static_cast<int>(model_->size()) * rowHeight + 16));

    int y = r.y + 8 - scrollY_;
    for (size_t row = 0; row < model_->size() && y < r.y + r.height - 10; ++row, y += rowHeight) {
        if (y < r.y + 6) {
            continue;
        }
        int x = r.x + 10 - scrollX_;
        for (size_t col = 0; col < (*model_)[row].size() && x < r.x + r.width - 12; ++col, x += columnWidth) {
            if (static_cast<int>(row) == selectedRow_ && static_cast<int>(col) == selectedCol_) {
                RECT sr{x - 2, y - 2, x + columnWidth - 4, y + 20};
                HBRUSH b = CreateSolidBrush(rgb(theme.hover));
                FillRect(drawable, &sr, b);
                DeleteObject(b);
            }
            drawText(d, drawable, gc, theme, (*model_)[row][col], x, y);
        }
    }
    drawScrollbars(d, drawable, gc, theme);
}

void Neu_ListView::handleXEvent(XEvent& ev)
{
    Neu_Control::handleXEvent(ev);
    if (ev.message == WM_LBUTTONUP && contains(ev.x, ev.y) && model_) {
        Neu_Rect r = bounds();
        const int row = (ev.y - r.y + scrollY_) / 22;
        const int col = (ev.x - r.x + scrollX_) / 110;
        if (row >= 0
            && row < static_cast<int>(model_->size())
            && col >= 0
            && col < static_cast<int>((*model_)[static_cast<size_t>(row)].size())) {
            selectedRow_ = row;
            selectedCol_ = col;
            if (callbacks_.onSelectionChanged) {
                callbacks_.onSelectionChanged(this,
                                              row,
                                              col,
                                              (*model_)[static_cast<size_t>(row)][static_cast<size_t>(col)].c_str(),
                                              callbacks_.userData);
            }
            requestRedraw();
        }
    }
}

void Neu_TreeView::expandAll()
{
    collapsedPaths_.clear();
    requestRedraw();
}

void Neu_TreeView::collapseAll()
{
    if (model()) {
        for (const auto& row : *model()) {
            if (!row.empty()) {
                collapsedPaths_.insert(row[0]);
            }
        }
    }
    requestRedraw();
}

void Neu_TreeView::toggleNodePath(const std::string& path)
{
    if (collapsedPaths_.count(path)) {
        collapsedPaths_.erase(path);
    } else {
        collapsedPaths_.insert(path);
    }
    requestRedraw();
}

bool Neu_TreeView::isPathCollapsed(const std::string& path) const
{
    return collapsedPaths_.count(path) != 0;
}

void Neu_TreeView::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_ListView::draw(d, drawable, gc, theme);
}

void Neu_TreeView::handleXEvent(XEvent& ev)
{
    Neu_ListView::handleXEvent(ev);
}

void Neu_Placement::add(std::shared_ptr<Neu_Control> child)
{
    if (!child) {
        return;
    }

    child->setParent(parent_);
    children_.push_back(child);
}

void Neu_Placement::setParent(Neu_Window* parent)
{
    Neu_Control::setParent(parent);
    for (auto& child : children_) {
        if (child) {
            child->setParent(parent);
        }
    }
}

void Neu_Placement::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    for (auto& c : children_) {
        if (c && c->visible()) {
            c->draw(d, drawable, gc, theme);
        }
    }
}

void Neu_Placement::handleXEvent(XEvent& ev)
{
    for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
        auto& child = *it;
        if (child && child->visible() && child->enabled() && child->contains(ev.x, ev.y)) {
            child->handleXEvent(ev);
            return;
        }
    }

    Neu_Control::handleXEvent(ev);
}

void Neu_PopWindowMenu::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Placement::draw(d, drawable, gc, theme);
}

void Neu_ScrollBar::setRange(int total, int page, int value)
{
    total_ = std::max(1, total);
    page_ = std::max(1, page);
    value_ = std::max(0, std::min(value, total_ - 1));
}

void Neu_ScrollBar::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
}

void Neu_ScrollBar::handleXEvent(XEvent& ev)
{
    Neu_Control::handleXEvent(ev);
}

void Neu_ScrollWindow::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Placement::draw(d, drawable, gc, theme);
    drawScrollbars(d, drawable, gc, theme);
}

void Neu_ScrollWindow::handleXEvent(XEvent& ev)
{
    Neu_Placement::handleXEvent(ev);
}

void Neu_Label::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
}

void Neu_MultilineLabel::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
}

void Neu_RichTextCode::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Multilinetextbox::draw(d, drawable, gc, theme);
}

void Neu_RichTextCode::handleXEvent(XEvent& ev)
{
    if (!readOnly_) {
        Neu_Multilinetextbox::handleXEvent(ev);
    } else {
        Neu_Control::handleXEvent(ev);
    }
}

void Neu_ProgressSquare::setProgress(float progress)
{
    progress_ = std::max(0.0f, std::min(1.0f, progress));
    requestRedraw();
}

void Neu_ProgressSquare::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    Neu_Rect r = bounds();
    HPEN pen = CreatePen(PS_SOLID, 4, rgb(progress_ > 0.85f ? Neu_Color{255, 215, 60, 255} : theme.accent));
    HGDIOBJ old = SelectObject(drawable, pen);
    const int inset = 5;
    const int maxTop = std::max(0, r.width - inset * 2);
    const int perimeter = 2 * ((r.width - inset * 2) + (r.height - inset * 2));
    const int n = static_cast<int>(perimeter * progress_);
    MoveToEx(drawable, r.x + inset, r.y + inset, nullptr);
    LineTo(drawable, r.x + inset + std::min(n, maxTop), r.y + inset);
    SelectObject(drawable, old);
    DeleteObject(pen);
}

void Neu_ImageView::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    Neu_Rect r = bounds();
    drawIconBmp(d, drawable, gc, r.x + 6, r.y + 6, std::min(r.width, r.height) - 12);
}

void Neu_ReadOnlyRichText::setIconList(const std::vector<std::string>& bmpIconPaths)
{
    iconPaths_ = bmpIconPaths;
}

size_t Neu_ReadOnlyRichText::iconIndexForText(const std::string& text) const
{
    size_t c = 0;
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '#' && (i == 0 || text[i - 1] != '\\')) {
            ++c;
        }
    }
    return iconPaths_.empty() ? 0 : std::min(c, iconPaths_.size() - 1);
}

void Neu_ReadOnlyRichText::addLabel(const std::string& text)
{
    const int y = 12 + static_cast<int>(children().size()) * 34;
    auto l = std::make_shared<Neu_Label>(Neu_Layout{layout().left + 12,
                                                    layout().top + y,
                                                    std::max(40, layout().width - 36),
                                                    28});
    l->setText(unescapeHashesWin32(text));
    if (!iconPaths_.empty()) {
        l->setIconBmp(iconPaths_[iconIndexForText(text)]);
    }
    add(l);
    setContentSize(layout().width, y + 50);
}

void Neu_ReadOnlyRichText::addMultilineLabel(const std::string& text)
{
    const int y = 12 + static_cast<int>(children().size()) * 62;
    auto l = std::make_shared<Neu_MultilineLabel>(Neu_Layout{layout().left + 12,
                                                             layout().top + y,
                                                             std::max(40, layout().width - 36),
                                                             56});
    l->setText(unescapeHashesWin32(text));
    l->setAutoScroll(true);
    if (!iconPaths_.empty()) {
        l->setIconBmp(iconPaths_[iconIndexForText(text)]);
    }
    add(l);
    setContentSize(layout().width, y + 80);
}

Neu_Window::Neu_Window(Neu_Application& app, int width, int height, const std::string& title)
    : app_(app),
      display_(nullptr),
      width_(std::max(1, width)),
      height_(std::max(1, height)),
      title_(title)
{
}

Neu_Window::~Neu_Window()
{
    close();
}

bool Neu_Window::create()
{
    const DWORD style = WS_OVERLAPPEDWINDOW;
    const DWORD exStyle = WS_EX_APPWINDOW;
    RECT wr{0, 0, width_, height_};
    AdjustWindowRectEx(&wr, style, FALSE, exStyle);

    HWND hwnd = CreateWindowExW(exStyle,
                                kNeuWindowClass,
                                toWide(title_).c_str(),
                                style,
                                CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                wr.right - wr.left,
                                wr.bottom - wr.top,
                                nullptr,
                                nullptr,
                                g_instance ? g_instance : GetModuleHandleW(nullptr),
                                this);
    if (!hwnd) {
        return false;
    }

    window_ = hwnd;
    RECT client{};
    GetClientRect(hwnd, &client);
    width_ = std::max(1, static_cast<int>(client.right - client.left));
    height_ = std::max(1, static_cast<int>(client.bottom - client.top));
    gc_ = GetDC(hwnd);
    app_.registerWindow(this);
    return true;
}

void Neu_Window::show()
{
    ShowWindow(window_, SW_SHOW);
    UpdateWindow(window_);
}

void Neu_Window::releaseBuffers()
{
    if (oldBitmap_ && memoryDc_) {
        SelectObject(memoryDc_, oldBitmap_);
    }
    if (backBitmap_) {
        DeleteObject(backBitmap_);
    }
    if (memoryDc_) {
        DeleteDC(memoryDc_);
    }
    oldBitmap_ = nullptr;
    backBitmap_ = nullptr;
    memoryDc_ = nullptr;
    bufferWidth_ = 0;
    bufferHeight_ = 0;
}

void Neu_Window::close()
{
    if (!window_ || closing_) {
        return;
    }

    closing_ = true;
    setFocusedControl(nullptr);
    captureControl_ = nullptr;
    releaseBuffers();

    HWND hwnd = window_;
    HDC dc = gc_;
    window_ = nullptr;
    gc_ = nullptr;
    app_.unregisterWindow(this);

    if (dc) {
        ReleaseDC(hwnd, dc);
    }

    DestroyWindow(hwnd);

    if (!app_.hasWindows()) {
        app_.quit();
    }
}

void Neu_Window::ensureBuffers()
{
    if (!window_ || width_ <= 0 || height_ <= 0) {
        return;
    }
    if (bufferWidth_ == width_ && bufferHeight_ == height_ && memoryDc_ && backBitmap_) {
        return;
    }

    releaseBuffers();
    HDC dc = GetDC(window_);
    memoryDc_ = CreateCompatibleDC(dc);
    backBitmap_ = CreateCompatibleBitmap(dc, width_, height_);
    oldBitmap_ = static_cast<HBITMAP>(SelectObject(memoryDc_, backBitmap_));
    ReleaseDC(window_, dc);
    bufferWidth_ = width_;
    bufferHeight_ = height_;
}

void Neu_Window::drawScene(Drawable target)
{
    RECT rc{0, 0, width_, height_};
    HBRUSH b = CreateSolidBrush(rgb(theme_.background));
    FillRect(target, &rc, b);
    DeleteObject(b);

    for (auto& c : controls_) {
        if (c->visible()) {
            c->drawShadow(nullptr, target, target, theme_);
        }
    }
    for (auto& c : controls_) {
        if (c->visible()) {
            c->draw(nullptr, target, target, theme_);
        }
    }
    for (auto& c : controls_) {
        if (c->visible()) {
            c->drawHintPopup(nullptr, target, target, theme_);
        }
    }
}

void Neu_Window::paint(Drawable target)
{
    if (!target || width_ <= 0 || height_ <= 0) {
        return;
    }

    if (multiStageDoubleBuffering_ && g_options.multiStageDoubleBuffering) {
        ensureBuffers();
        if (memoryDc_) {
            drawScene(memoryDc_);
            BitBlt(target, 0, 0, width_, height_, memoryDc_, 0, 0, SRCCOPY);
            return;
        }
    }

    drawScene(target);
}

void Neu_Window::redraw()
{
    if (!window_) {
        return;
    }
    HDC dc = GetDC(window_);
    paint(dc);
    ReleaseDC(window_, dc);
}

void Neu_Window::invalidate()
{
    if (window_) {
        InvalidateRect(window_, nullptr, FALSE);
    }
}

void Neu_Window::requestRedraw()
{
    invalidate();
}

void Neu_Window::setMultiStageDoubleBuffering(bool enabled)
{
    multiStageDoubleBuffering_ = enabled;
    if (!enabled) {
        releaseBuffers();
    }
    requestRedraw();
}

void Neu_Window::add(std::shared_ptr<Neu_Control> control)
{
    if (control) {
        control->setParent(this);
        controls_.push_back(control);
    }
}

Neu_Control* Neu_Window::hitTest(int x, int y)
{
    auto hitOne = [&](auto&& self, const std::shared_ptr<Neu_Control>& control) -> Neu_Control* {
        if (!control || !control->visible() || !control->enabled()) {
            return nullptr;
        }

        if (auto placement = dynamic_cast<Neu_Placement*>(control.get())) {
            const auto& children = placement->children();
            for (auto child = children.rbegin(); child != children.rend(); ++child) {
                Neu_Control* hit = self(self, *child);
                if (hit) {
                    return hit;
                }
            }
        }

        return control->contains(x, y) ? control.get() : nullptr;
    };

    for (auto it = controls_.rbegin(); it != controls_.rend(); ++it) {
        Neu_Control* hit = hitOne(hitOne, *it);
        if (hit) {
            return hit;
        }
    }
    return nullptr;
}

void Neu_Window::setFocusedControl(Neu_Control* control)
{
    if (focusedControl_ == control) {
        return;
    }
    if (focusedControl_) {
        focusedControl_->setFocused(false);
    }
    focusedControl_ = control;
    if (focusedControl_) {
        focusedControl_->setFocused(true);
    }
}

void Neu_Window::handleXEvent(XEvent& ev)
{
    if (ev.message == WM_CLOSE) {
        if (onClose_) {
            onClose_(this, closeUserData_);
        }
        close();
        return;
    }

    if (ev.message == WM_SIZE) {
        width_ = std::max(1, static_cast<int>(LOWORD(ev.lParam)));
        height_ = std::max(1, static_cast<int>(HIWORD(ev.lParam)));
        releaseBuffers();
        requestRedraw();
        return;
    }

    if (ev.message == WM_MOUSEMOVE) {
        TRACKMOUSEEVENT tme{};
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = window_;
        TrackMouseEvent(&tme);

        Neu_Control* target = hitTest(ev.x, ev.y);
        if (target != hoveredControl_) {
            if (hoveredControl_) {
                XEvent leave = ev;
                leave.message = WM_MOUSELEAVE;
                hoveredControl_->handleXEvent(leave);
            }
            hoveredControl_ = target;
        }
        if (target) {
            target->handleXEvent(ev);
        }
        return;
    }

    if (ev.message == WM_MOUSELEAVE) {
        if (hoveredControl_) {
            hoveredControl_->handleXEvent(ev);
            hoveredControl_ = nullptr;
        }
        return;
    }

    if (ev.message == WM_LBUTTONDOWN) {
        SetFocus(window_);
        SetCapture(window_);
        Neu_Control* target = hitTest(ev.x, ev.y);
        setFocusedControl(target);
        captureControl_ = target;
        if (target) {
            target->handleXEvent(ev);
        }
        return;
    }

    if (ev.message == WM_LBUTTONUP) {
        ReleaseCapture();
        Neu_Control* target = captureControl_ ? captureControl_ : hitTest(ev.x, ev.y);
        if (target) {
            target->handleXEvent(ev);
        }
        captureControl_ = nullptr;
        return;
    }

    if (ev.message == WM_MOUSEWHEEL || ev.message == WM_MOUSEHWHEEL) {
        Neu_Control* target = hitTest(ev.x, ev.y);
        if (!target) {
            target = focusedControl_;
        }
        if (target) {
            target->handleXEvent(ev);
        }
        return;
    }

    if (ev.message == WM_KEYDOWN || ev.message == WM_CHAR) {
        if (focusedControl_ && focusedControl_->visible() && focusedControl_->enabled()) {
            focusedControl_->handleXEvent(ev);
        }
        return;
    }
}

} // namespace neutrino
#endif
