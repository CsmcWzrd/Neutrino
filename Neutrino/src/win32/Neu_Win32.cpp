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

static HFONT createTextFont(bool bold, bool italic, bool underline, bool strikeOut, bool monospace, int headingLevel)
{
    const int height = headingLevel > 0 ? -(34 - headingLevel * 2) : -16;
    return CreateFontW(height,
                       0,
                       0,
                       0,
                       bold ? FW_BOLD : FW_NORMAL,
                       italic ? TRUE : FALSE,
                       underline ? TRUE : FALSE,
                       strikeOut ? TRUE : FALSE,
                       DEFAULT_CHARSET,
                       OUT_OUTLINE_PRECIS,
                       CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY,
                       (monospace ? FIXED_PITCH : VARIABLE_PITCH) | FF_DONTCARE,
                       monospace ? L"Consolas" : L"Segoe UI");
}

static std::vector<std::string> logicalLinesWin32(const std::string& text)
{
    std::vector<std::string> out;
    std::string current;
    current.reserve(text.size());

    for (size_t i = 0; i < text.size(); ++i) {
        const char ch = text[i];
        if (ch == '\r') {
            if (i + 1 < text.size() && text[i + 1] == '\n') {
                ++i;
            }
            out.push_back(current);
            current.clear();
        } else if (ch == '\n') {
            out.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }

    out.push_back(current);
    if (out.empty()) {
        out.push_back({});
    }
    return out;
}

static std::vector<std::string> splitPreserveLinesWin32(const std::string& text)
{
    return logicalLinesWin32(text);
}

static int lineHeightForHeadingWin32(int headingLevel)
{
    if (headingLevel > 0) {
        return std::max(22, 38 - headingLevel * 2);
    }
    return 20;
}

static bool ctrlDownWin32()
{
    return (GetKeyState(VK_CONTROL) & 0x8000) != 0;
}

static bool shiftDownWin32()
{
    return (GetKeyState(VK_SHIFT) & 0x8000) != 0;
}

static int neuMaxIntWin32(int a, int b)
{
    return a > b ? a : b;
}

static int neuMinIntWin32(int a, int b)
{
    return a < b ? a : b;
}

static RECT safeRectWin32(int left, int top, int right, int bottom)
{
    if (right < left) {
        right = left;
    }
    if (bottom < top) {
        bottom = top;
    }
    return RECT{left, top, right, bottom};
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
    } else if (msg == WM_TIMER) {
        (void)wp;
        (void)lp;
        x = 0;
        y = 0;
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
    case WM_TIMER:
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

void Neu_Control::drawText(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme, const std::string& s, int x, int y)
{
    drawTextColored(d, drawable, gc, theme, s, x, y, theme.text);
}

void Neu_Control::drawTextColored(Display*,
                                  Drawable drawable,
                                  GC,
                                  const Neu_Theme&,
                                  const std::string& s,
                                  int x,
                                  int y,
                                  const Neu_Color& color,
                                  bool bold,
                                  bool italic,
                                  bool underline,
                                  bool strikethrough,
                                  bool doubleStrikethrough,
                                  bool monospace,
                                  int headingLevel)
{
    SetBkMode(drawable, TRANSPARENT);
    SetTextColor(drawable, rgb(color));
    HFONT font = createTextFont(bold, italic, underline, strikethrough || doubleStrikethrough, monospace, headingLevel);
    HGDIOBJ old = SelectObject(drawable, font ? font : uiFont());
    std::wstring w = toWide(s);
    TextOutW(drawable, x, y, w.c_str(), static_cast<int>(w.size()));

    SIZE sz{};
    GetTextExtentPoint32W(drawable, w.c_str(), static_cast<int>(w.size()), &sz);
    if (doubleStrikethrough) {
        HPEN pen = CreatePen(PS_SOLID, 1, rgb(color));
        HGDIOBJ oldPen = SelectObject(drawable, pen);
        MoveToEx(drawable, x, y + 7, nullptr);
        LineTo(drawable, x + sz.cx, y + 7);
        MoveToEx(drawable, x, y + 10, nullptr);
        LineTo(drawable, x + sz.cx, y + 10);
        SelectObject(drawable, oldPen);
        DeleteObject(pen);
    }
    SelectObject(drawable, old);
    if (font) {
        DeleteObject(font);
    }
}

int Neu_Control::measureTextWidth(Display*,
                                  Drawable drawable,
                                  GC,
                                  const Neu_Theme&,
                                  const std::string& s,
                                  bool bold,
                                  bool italic,
                                  bool monospace,
                                  int headingLevel) const
{
    HDC hdc = drawable;
    if (!hdc) {
        return static_cast<int>(s.size()) * (monospace ? 8 : 7);
    }
    HFONT font = createTextFont(bold, italic, false, false, monospace, headingLevel);
    HGDIOBJ old = SelectObject(hdc, font ? font : uiFont());
    std::wstring w = toWide(s);
    SIZE sz{};
    GetTextExtentPoint32W(hdc, w.c_str(), static_cast<int>(w.size()), &sz);
    SelectObject(hdc, old);
    if (font) {
        DeleteObject(font);
    }
    return std::max(0, static_cast<int>(sz.cx));
}

std::vector<std::string> Neu_Control::wrapTextToWidth(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme, const std::string& text, int maxWidth) const
{
    std::vector<std::string> wrapped;
    if (maxWidth <= 0) {
        wrapped.push_back({});
        return wrapped;
    }
    for (const auto& logical : logicalLinesWin32(text)) {
        std::string current;
        std::string word;
        std::istringstream input(logical);
        while (input >> word) {
            const std::string candidate = current.empty() ? word : current + " " + word;
            if (!current.empty() && measureTextWidth(d, drawable, gc, theme, candidate) > maxWidth) {
                wrapped.push_back(current);
                current.clear();
            }
            if (measureTextWidth(d, drawable, gc, theme, word) > maxWidth) {
                std::string part;
                for (char ch : word) {
                    std::string candidatePart = part + ch;
                    if (!part.empty() && measureTextWidth(d, drawable, gc, theme, candidatePart) > maxWidth) {
                        wrapped.push_back(part);
                        part.clear();
                    }
                    part.push_back(ch);
                }
                if (!part.empty()) {
                    if (!current.empty()) {
                        wrapped.push_back(current);
                    }
                    current = part;
                }
            } else {
                current = current.empty() ? word : current + " " + word;
            }
        }
        wrapped.push_back(current);
    }
    if (wrapped.empty()) {
        wrapped.push_back({});
    }
    return wrapped;
}

std::string Neu_Control::truncateTextToWidth(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme, const std::string& text, int maxWidth) const
{
    if (maxWidth <= 0 || text.empty()) {
        return {};
    }
    if (measureTextWidth(d, drawable, gc, theme, text) <= maxWidth) {
        return text;
    }
    const std::string ellipsis = "...";
    std::string out;
    for (char ch : text) {
        std::string candidate = out + ch + ellipsis;
        if (measureTextWidth(d, drawable, gc, theme, candidate) > maxWidth) {
            break;
        }
        out.push_back(ch);
    }
    return out + ellipsis;
}

int Neu_Control::alignedTextX(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme, const std::string& text, int left, int width) const
{
    const int textWidth = measureTextWidth(d, drawable, gc, theme, text);
    if (textAlignment_ == Neu_TextAlignment::Center) {
        return left + std::max(0, (width - textWidth) / 2);
    }
    if (textAlignment_ == Neu_TextAlignment::Right) {
        return left + std::max(0, width - textWidth);
    }
    return left;
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

void Neu_Control::drawHintPopup(Display*, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    if (!g_options.drawHints || !hover_ || !hoverHintArmed_ || hintText_.empty()) {
        return;
    }

    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - hoverStartTime_).count();
    if (elapsed < 5000) {
        return;
    }

    Neu_Rect r = bounds();
    constexpr int padding = 12;
    constexpr int lineHeight = 20;
    int boxWidth = 400;
    if (parent_) {
        boxWidth = std::min(boxWidth, std::max(140, parent_->width() - 12));
    }
    const int contentWidth = std::max(40, boxWidth - 2 * padding - 18);
    const auto lines = wrapTextToWidth(nullptr, drawable, gc, theme, hintText_, contentWidth);
    const int naturalHeight = static_cast<int>(lines.size()) * lineHeight + 2 * padding;
    const int visibleHeight = std::max(44, std::min(500, naturalHeight));
    const int visibleLines = std::max(1, (visibleHeight - 2 * padding) / lineHeight);
    const bool needsScrollbar = naturalHeight > visibleHeight;
    const bool needsDropDown = lines.size() > 3U;
    int boxX = r.x + r.width + 12;
    int boxY = r.y;
    if (parent_) {
        if (boxX + boxWidth > parent_->width()) {
            boxX = std::max(6, r.x - boxWidth - 12);
        }
        if (boxY + visibleHeight > parent_->height()) {
            boxY = std::max(6, parent_->height() - visibleHeight - 6);
        }
    }

    RECT box{boxX, boxY, boxX + boxWidth, boxY + visibleHeight};
    HBRUSH b = CreateSolidBrush(rgb(theme.hintBackground));
    FillRect(drawable, &box, b);
    DeleteObject(b);
    FrameRect(drawable, &box, reinterpret_cast<HBRUSH>(GetStockObject(GRAY_BRUSH)));

    const int clipLeft = box.left + padding;
    const int clipTop = box.top + padding;
    const int clipRight = box.right - padding - (needsScrollbar ? 12 : 2);
    const int clipBottom = box.bottom - padding;
    int saved = SaveDC(drawable);
    IntersectClipRect(drawable, clipLeft, clipTop, clipRight, clipBottom);
    const int count = hintExpanded_ ? std::min(static_cast<int>(lines.size()), visibleLines) : std::min(3, visibleLines);
    const int drawLeft = clipLeft + 4;
    const int drawWidth = std::max(1, clipRight - drawLeft - 2);
    for (int i = 0; i < count; ++i) {
        const std::string line = truncateTextToWidth(nullptr, drawable, gc, theme, lines[static_cast<size_t>(i)], drawWidth);
        drawText(nullptr, drawable, gc, theme, line, drawLeft, clipTop + i * lineHeight + 14);
    }
    RestoreDC(drawable, saved);

    if (needsDropDown) {
        drawText(nullptr, drawable, gc, theme, hintExpanded_ ? "^" : "v", box.right - 24, box.bottom - 24);
    }
    if (needsScrollbar) {
        RECT track{box.right - 10, box.top + 12, box.right - 6, box.bottom - 12};
        HBRUSH tr = CreateSolidBrush(RGB(205, 210, 170));
        FillRect(drawable, &track, tr);
        DeleteObject(tr);
        RECT thumb{box.right - 11, box.top + 12, box.right - 5, box.top + 12 + std::max(20, visibleHeight / 4)};
        HBRUSH th = CreateSolidBrush(rgb(theme.hintBorder));
        FillRect(drawable, &thumb, th);
        DeleteObject(th);
    }
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

    const std::string cls = className();
    const bool baseText = cls == "Neu_Button" || cls == "Neu_Control" || cls == "Neu_MenuItem" || cls == "Neu_FlatButton";
    if (baseText && !text_.empty()) {
        const int textLeft = r.x + (icon_.width() > 0 ? r.height : 10);
        const int textWidth = std::max(1, r.width - (textLeft - r.x) - 12);
        int saved = SaveDC(drawable);
        IntersectClipRect(drawable, textLeft, r.y + 4, r.x + r.width - 8, r.y + r.height - 4);
        const std::string visible = truncateTextToWidth(d, drawable, gc, theme, text_, textWidth);
        drawText(d,
                 drawable,
                 gc,
                 theme,
                 visible,
                 alignedTextX(d, drawable, gc, theme, visible, textLeft, textWidth),
                 r.y + std::max(4, (r.height - 16) / 2));
        RestoreDC(drawable, saved);
    }
    drawScrollbars(d, drawable, gc, theme);
}

void Neu_Control::handleXEvent(XEvent& ev)
{
    const bool inside = contains(ev.x, ev.y);

    auto resetHintAnchor = [&]() {
        hoverAnchorX_ = ev.x;
        hoverAnchorY_ = ev.y;
        hoverStartTime_ = std::chrono::steady_clock::now();
        hoverHintArmed_ = true;
    };

    auto updateScrollbarDrag = [&]() -> bool {
        if (!autoScroll_ || activeScrollDrag_ == 0) {
            return false;
        }
        Neu_Rect r = bounds();
        if (activeScrollDrag_ == 1) {
            const int trackY = r.y + 8;
            const int trackH = std::max(1, r.height - 16);
            const int maxY = std::max(1, virtualSize_.height - r.height);
            setScrollOffset(scrollX_, (ev.y - trackY) * maxY / trackH);
        } else if (activeScrollDrag_ == 2) {
            const int trackX = r.x + 8;
            const int trackW = std::max(1, r.width - 16);
            const int maxX = std::max(1, virtualSize_.width - r.width);
            setScrollOffset((ev.x - trackX) * maxX / trackW, scrollY_);
        }
        return true;
    };

    if (ev.message == WM_MOUSEMOVE) {
        if (updateScrollbarDrag()) {
            return;
        }
        if (inside) {
            if (!hover_) {
                hover_ = true;
                resetHintAnchor();
                if (callbacks_.onFocus) {
                    callbacks_.onFocus(this, callbacks_.userData);
                }
                requestRedraw();
            } else if (std::abs(ev.x - hoverAnchorX_) > 4 || std::abs(ev.y - hoverAnchorY_) > 4) {
                resetHintAnchor();
                requestRedraw();
            }
        } else if (hover_) {
            hover_ = false;
            hoverHintArmed_ = false;
            if (callbacks_.onBlur) {
                callbacks_.onBlur(this, callbacks_.userData);
            }
            requestRedraw();
        }
        return;
    }

    if (ev.message == WM_MOUSELEAVE) {
        if (hover_) {
            hover_ = false;
            hoverHintArmed_ = false;
            if (callbacks_.onBlur) {
                callbacks_.onBlur(this, callbacks_.userData);
            }
            requestRedraw();
        }
        activeScrollDrag_ = 0;
        return;
    }

    if (ev.message == WM_LBUTTONDOWN && inside && autoScroll_) {
        Neu_Rect r = bounds();
        const bool needVertical = virtualSize_.height > r.height;
        const bool needHorizontal = virtualSize_.width > r.width;
        if (needVertical && ev.x >= r.x + r.width - 14) {
            activeScrollDrag_ = 1;
            updateScrollbarDrag();
            return;
        }
        if (needHorizontal && ev.y >= r.y + r.height - 14) {
            activeScrollDrag_ = 2;
            updateScrollbarDrag();
            return;
        }
    }

    if (ev.message == WM_LBUTTONUP && activeScrollDrag_ != 0) {
        updateScrollbarDrag();
        activeScrollDrag_ = 0;
        return;
    }

    if (ev.message == WM_LBUTTONDOWN && inside) {
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
    const int textLeft = r.x + 10;
    const int textTop = r.y + std::max(4, (r.height - 16) / 2);
    const int textWidth = std::max(1, r.width - 22);

    const size_t caretBytes = std::min(cursor_, text_.size());
    const std::string prefix = text_.substr(0, caretBytes);
    const int prefixWidth = measureTextWidth(d, drawable, gc, theme, prefix);
    const int fullWidth = measureTextWidth(d, drawable, gc, theme, text_);
    const int maxScroll = std::max(0, fullWidth - textWidth + 4);
    if (focused_) {
        if (prefixWidth - textScrollX_ > textWidth - 2) {
            textScrollX_ = prefixWidth - textWidth + 2;
        }
        if (prefixWidth - textScrollX_ < 0) {
            textScrollX_ = prefixWidth;
        }
    }
    textScrollX_ = std::max(0, std::min(textScrollX_, maxScroll));

    int saved = SaveDC(drawable);
    IntersectClipRect(drawable, r.x + 6, r.y + 4, r.x + r.width - 6, r.y + r.height - 4);
    drawText(d,
             drawable,
             gc,
             theme,
             text_,
             textLeft - textScrollX_,
             textTop);
    if (focused_) {
        const int caretX = textLeft + prefixWidth - textScrollX_;
        if (caretX >= r.x + 6 && caretX <= r.x + r.width - 6) {
            HPEN pen = CreatePen(PS_SOLID, 1, rgb(theme.accent));
            HGDIOBJ old = SelectObject(drawable, pen);
            MoveToEx(drawable, caretX, r.y + 8, nullptr);
            LineTo(drawable, caretX, r.y + r.height - 8);
            SelectObject(drawable, old);
            DeleteObject(pen);
        }
    }
    RestoreDC(drawable, saved);
    drawHintPopup(d, drawable, gc, theme);
}

void Neu_Textbox::handleXEvent(XEvent& ev)
{
    Neu_Control::handleXEvent(ev);

    if (ev.message == WM_LBUTTONDOWN && contains(ev.x, ev.y)) {
        Neu_Rect r = bounds();
        const int textLeft = r.x + 10;
        const int localX = ev.x - textLeft + textScrollX_;
        cursor_ = 0;
        HDC hdc = parent_ && parent_->xid() ? GetDC(parent_->xid()) : nullptr;
        HGDIOBJ oldFont = nullptr;
        if (hdc) {
            oldFont = SelectObject(hdc, uiFont());
        }
        for (size_t i = 1; i <= text_.size(); ++i) {
            int w = static_cast<int>(i) * 8;
            if (hdc) {
                const std::wstring prefix = toWide(text_.substr(0, i));
                SIZE sz{};
                GetTextExtentPoint32W(hdc, prefix.c_str(), static_cast<int>(prefix.size()), &sz);
                w = sz.cx;
            }
            if (w <= localX) {
                cursor_ = i;
            } else {
                break;
            }
        }
        if (hdc) {
            if (oldFont) {
                SelectObject(hdc, oldFont);
            }
            ReleaseDC(parent_->xid(), hdc);
        }
        requestRedraw();
        return;
    }

    if (!focused_ || !enabled_) {
        return;
    }

    bool changed = false;
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
                changed = true;
            }
            break;
        default:
            break;
        }
        (void)changed;
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
    const int contentWidth = std::max(1, r.width - 24);
    const int lineHeight = 20;
    std::vector<std::string> lines;
    if (wordWrap_) {
        auto logical = logicalLinesWin32(text_);
        for (const auto& baseLine : logical) {
            auto wrapped = wrapTextToWidth(d, drawable, gc, theme, baseLine, contentWidth);
            lines.insert(lines.end(), wrapped.begin(), wrapped.end());
        }
    } else {
        lines = logicalLinesWin32(text_);
    }
    int maxWidth = r.width;
    for (const auto& line : lines) {
        maxWidth = std::max(maxWidth, measureTextWidth(d, drawable, gc, theme, line) + 24);
    }
    setAutoScroll(true);
    setVirtualSize(std::max(r.width, maxWidth), std::max(r.height, static_cast<int>(lines.size()) * lineHeight + 16));

    int saved = SaveDC(drawable);
    IntersectClipRect(drawable, r.x + 4, r.y + 4, r.x + r.width - 14, r.y + r.height - 14);
    int y = r.y + 8 - scrollY_;
    for (const auto& line : lines) {
        if (y >= r.y + 6 && y < r.y + r.height - 10) {
            if (wordWrap_) {
                drawText(d, drawable, gc, theme, truncateTextToWidth(d, drawable, gc, theme, line, contentWidth), r.x + 10, y);
            } else {
                drawText(d, drawable, gc, theme, line, r.x + 10 - scrollX_, y);
            }
        }
        y += lineHeight;
    }

    if (focused_) {
        size_t lineIndex = 0;
        size_t lineStart = 0;
        const size_t caretBytes = std::min(cursor_, text_.size());
        for (size_t i = 0; i < caretBytes; ++i) {
            if (text_[i] == '\n') {
                ++lineIndex;
                lineStart = i + 1;
            }
        }
        const std::string prefix = text_.substr(lineStart, caretBytes - lineStart);
        const int caretX = r.x + 10 + measureTextWidth(d, drawable, gc, theme, prefix) - (wordWrap_ ? 0 : scrollX_);
        const int caretY = r.y + 8 + static_cast<int>(lineIndex) * lineHeight - scrollY_;
        if (caretY >= r.y + 6 && caretY < r.y + r.height - 10) {
            HPEN pen = CreatePen(PS_SOLID, 1, rgb(theme.accent));
            HGDIOBJ old = SelectObject(drawable, pen);
            MoveToEx(drawable, caretX, caretY - 2, nullptr);
            LineTo(drawable, caretX, caretY + lineHeight - 4);
            SelectObject(drawable, old);
            DeleteObject(pen);
        }
    }
    RestoreDC(drawable, saved);
    drawScrollbars(d, drawable, gc, theme);
    drawHintPopup(d, drawable, gc, theme);
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
    setAutoScroll(true);
    setVirtualSize(r.width, std::max(r.height, static_cast<int>(items_.size()) * rowHeight + 16));
    int saved = SaveDC(drawable);
    IntersectClipRect(drawable, r.x + 4, r.y + 4, r.x + r.width - 14, r.y + r.height - 14);
    int y = r.y + 18 - scrollY_;
    for (size_t i = 0; i < items_.size(); ++i, y += rowHeight) {
        if (y < r.y + 8) {
            continue;
        }
        if (y >= r.y + r.height - 8) {
            break;
        }
        const bool selected = multiSelect_ ? selectedIndices_.count(static_cast<int>(i)) != 0U : static_cast<int>(i) == selected_;
        const bool hovered = static_cast<int>(i) == hoveredIndex_;
        if (selected || hovered) {
            RECT sr{r.x + 4, y - 16, r.x + r.width - 14, y + 4};
            HBRUSH b = CreateSolidBrush(rgb(selected ? theme.pressed : theme.hover));
            FillRect(drawable, &sr, b);
            DeleteObject(b);
        }
        RECT textClip{r.x + 9, y - 16, r.x + r.width - 16, y + 5};
        int cellSaved = SaveDC(drawable);
        IntersectClipRect(drawable, textClip.left, textClip.top, textClip.right, textClip.bottom);
        drawText(d, drawable, gc, theme, truncateTextToWidth(d, drawable, gc, theme, items_[i], r.width - 28), r.x + 10, y - 14);
        RestoreDC(drawable, cellSaved);
    }
    RestoreDC(drawable, saved);
    drawScrollbars(d, drawable, gc, theme);
    drawHintPopup(d, drawable, gc, theme);
}

void Neu_Listbox::handleXEvent(XEvent& ev)
{
    Neu_Control::handleXEvent(ev);
    if (!contains(ev.x, ev.y)) {
        if (ev.message == WM_MOUSELEAVE && hoveredIndex_ != -1) {
            hoveredIndex_ = -1;
            requestRedraw();
        }
        return;
    }

    Neu_Rect r = bounds();
    const int row = (ev.y - r.y + scrollY_) / 22;
    const bool validRow = row >= 0 && row < static_cast<int>(items_.size());

    if (ev.message == WM_MOUSEMOVE) {
        const int newHover = validRow ? row : -1;
        if (newHover != hoveredIndex_) {
            hoveredIndex_ = newHover;
            requestRedraw();
        }
        return;
    }

    if (ev.message == WM_LBUTTONUP && validRow) {
        selected_ = row;
        if (multiSelect_) {
            if (shiftDownWin32() && anchorIndex_ >= 0) {
                selectedIndices_.clear();
                const int a = std::min(anchorIndex_, row);
                const int b = std::max(anchorIndex_, row);
                for (int i = a; i <= b; ++i) {
                    selectedIndices_.insert(i);
                }
            } else if (ctrlDownWin32()) {
                if (selectedIndices_.count(row) != 0U) {
                    selectedIndices_.erase(row);
                } else {
                    selectedIndices_.insert(row);
                }
                anchorIndex_ = row;
            } else {
                selectedIndices_.clear();
                selectedIndices_.insert(row);
                anchorIndex_ = row;
            }
        } else {
            selectedIndices_.clear();
            selectedIndices_.insert(row);
            anchorIndex_ = row;
        }
        if (callbacks_.onSelectionChanged) {
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
    constexpr int columnWidth = 140;
    const int viewportLeft = r.x + 4;
    const int viewportRight = r.x + r.width - 14;
    const int viewportTop = r.y + 4;
    const int viewportBottom = r.y + r.height - 14;
    size_t maxCols = 0;
    for (const auto& row : *model_) {
        maxCols = std::max(maxCols, row.size());
    }
    setAutoScroll(true);
    setVirtualSize(std::max(r.width, static_cast<int>(maxCols) * columnWidth + 20),
                   std::max(r.height, static_cast<int>(model_->size()) * rowHeight + 16));

    int saved = SaveDC(drawable);
    IntersectClipRect(drawable, viewportLeft, viewportTop, viewportRight, viewportBottom);
    int y = r.y + 20 - scrollY_;
    for (size_t row = 0; row < model_->size(); ++row, y += rowHeight) {
        if (y < r.y + 8) {
            continue;
        }
        if (y >= r.y + r.height - 8) {
            break;
        }
        const bool rowSelected = multiSelect_ ? selectedRows_.count(static_cast<int>(row)) != 0U : static_cast<int>(row) == selectedRow_;
        const bool rowHovered = static_cast<int>(row) == hoveredRow_;
        if (rowSelected || rowHovered) {
            RECT rr{viewportLeft, y - 18, viewportRight, y + 3};
            HBRUSH b = CreateSolidBrush(rgb(rowSelected ? theme.pressed : theme.hover));
            FillRect(drawable, &rr, b);
            DeleteObject(b);
        }
        int x = r.x + 10 - scrollX_;
        for (size_t col = 0; col < (*model_)[row].size(); ++col, x += columnWidth) {
            if (x + columnWidth < viewportLeft) {
                continue;
            }
            if (x >= viewportRight) {
                break;
            }
            RECT cell{x - 3, y - 18, x + columnWidth - 4, y + 3};
            const int clippedCellLeft = neuMaxIntWin32(static_cast<int>(cell.left), viewportLeft);
            const int clippedCellTop = neuMaxIntWin32(static_cast<int>(cell.top), viewportTop);
            const int clippedCellRight = neuMinIntWin32(static_cast<int>(cell.right), viewportRight);
            const int clippedCellBottom = neuMinIntWin32(static_cast<int>(cell.bottom), viewportBottom);
            RECT visibleCell = safeRectWin32(clippedCellLeft, clippedCellTop, clippedCellRight, clippedCellBottom);
            if (static_cast<int>(row) == selectedRow_ && static_cast<int>(col) == selectedCol_) {
                HBRUSH b = CreateSolidBrush(rgb(theme.hover));
                FillRect(drawable, &visibleCell, b);
                DeleteObject(b);
            }
            FrameRect(drawable, &visibleCell, reinterpret_cast<HBRUSH>(GetStockObject(GRAY_BRUSH)));
            if (visibleCell.right > visibleCell.left + 4) {
                int cellSaved = SaveDC(drawable);
                IntersectClipRect(drawable,
                                  static_cast<int>(visibleCell.left) + 2,
                                  static_cast<int>(visibleCell.top),
                                  static_cast<int>(visibleCell.right) - 2,
                                  static_cast<int>(visibleCell.bottom));
                const int drawX = neuMaxIntWin32(x, static_cast<int>(visibleCell.left) + 2);
                const int visibleWidth = neuMaxIntWin32(1, static_cast<int>(visibleCell.right) - drawX - 2);
                const std::string cellText = truncateTextToWidth(d, drawable, gc, theme, (*model_)[row][col], visibleWidth);
                drawText(d, drawable, gc, theme, cellText, drawX, y - 14);
                RestoreDC(drawable, cellSaved);
            }
        }
    }
    RestoreDC(drawable, saved);
    drawScrollbars(d, drawable, gc, theme);
    drawHintPopup(d, drawable, gc, theme);
}

void Neu_ListView::handleXEvent(XEvent& ev)
{
    Neu_Control::handleXEvent(ev);
    if (!model_) {
        return;
    }
    if (!contains(ev.x, ev.y)) {
        if (ev.message == WM_MOUSELEAVE && (hoveredRow_ != -1 || hoveredCol_ != -1)) {
            hoveredRow_ = -1;
            hoveredCol_ = -1;
            requestRedraw();
        }
        return;
    }

    Neu_Rect r = bounds();
    const int row = (ev.y - r.y + scrollY_) / 22;
    const int col = (ev.x - r.x - 10 + scrollX_) / 140;
    const bool validRow = row >= 0 && row < static_cast<int>(model_->size());
    const bool validCol = validRow && col >= 0 && col < static_cast<int>((*model_)[static_cast<size_t>(row)].size());

    if (ev.message == WM_MOUSEMOVE) {
        const int newRow = validRow ? row : -1;
        const int newCol = validCol ? col : -1;
        if (newRow != hoveredRow_ || newCol != hoveredCol_) {
            hoveredRow_ = newRow;
            hoveredCol_ = newCol;
            requestRedraw();
        }
        return;
    }

    if (ev.message == WM_LBUTTONUP && validCol) {
        selectedRow_ = row;
        selectedCol_ = col;
        if (multiSelect_) {
            if (shiftDownWin32() && anchorRow_ >= 0) {
                selectedRows_.clear();
                const int a = std::min(anchorRow_, row);
                const int b = std::max(anchorRow_, row);
                for (int i = a; i <= b; ++i) {
                    selectedRows_.insert(i);
                }
            } else if (ctrlDownWin32()) {
                if (selectedRows_.count(row) != 0U) {
                    selectedRows_.erase(row);
                } else {
                    selectedRows_.insert(row);
                }
                anchorRow_ = row;
            } else {
                selectedRows_.clear();
                selectedRows_.insert(row);
                anchorRow_ = row;
            }
        } else {
            selectedRows_.clear();
            selectedRows_.insert(row);
            anchorRow_ = row;
        }
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

namespace {
struct TreeRowInfoWin {
    size_t modelRow{0};
    int depth{0};
    std::string label;
    std::string path;
    bool hasChildren{false};
};

static std::vector<std::string> rowPathWin(const std::vector<std::string>& row)
{
    std::vector<std::string> path;
    for (const auto& cell : row) {
        if (!cell.empty()) {
            path.push_back(cell);
        }
    }
    return path;
}

static std::string joinPathWin(const std::vector<std::string>& path, size_t count)
{
    std::string result;
    for (size_t index = 0; index < count && index < path.size(); ++index) {
        if (!result.empty()) {
            result += '/';
        }
        result += path[index];
    }
    return result;
}

static bool collapsedAncestorWin(const std::vector<std::string>& path, const std::set<std::string>& collapsed)
{
    for (size_t depth = 1; depth < path.size(); ++depth) {
        if (collapsed.count(joinPathWin(path, depth)) != 0U) {
            return true;
        }
    }
    return false;
}

static std::vector<TreeRowInfoWin> buildTreeRowsWin(const Neu_StringTable* model, const std::set<std::string>& collapsed)
{
    std::vector<TreeRowInfoWin> rows;
    if (!model) {
        return rows;
    }
    std::map<std::string, bool> hasChild;
    std::vector<std::vector<std::string>> paths;
    for (const auto& source : *model) {
        auto path = rowPathWin(source);
        for (size_t depth = 1; depth < path.size(); ++depth) {
            hasChild[joinPathWin(path, depth)] = true;
        }
        paths.push_back(path);
    }
    for (size_t i = 0; i < paths.size(); ++i) {
        const auto& path = paths[i];
        if (path.empty() || collapsedAncestorWin(path, collapsed)) {
            continue;
        }
        const std::string key = joinPathWin(path, path.size());
        rows.push_back({i, static_cast<int>(path.size() - 1), path.back(), key, hasChild.count(key) != 0U});
    }
    return rows;
}
}

void Neu_TreeView::expandAll()
{
    collapsedPaths_.clear();
    requestRedraw();
}

void Neu_TreeView::collapseAll()
{
    collapsedPaths_.clear();
    if (model()) {
        for (const auto& row : *model()) {
            auto path = rowPathWin(row);
            for (size_t depth = 1; depth <= path.size(); ++depth) {
                collapsedPaths_.insert(joinPathWin(path, depth));
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
    Neu_Control::draw(d, drawable, gc, theme);
    const auto rows = buildTreeRowsWin(model(), collapsedPaths_);
    Neu_Rect r = bounds();
    constexpr int rowHeight = 22;
    int maxWidth = r.width;
    for (const auto& row : rows) {
        const int indent = row.depth * 18;
        const std::string label = (row.hasChildren ? (isPathCollapsed(row.path) ? "+ " : "- ") : "  ") + row.label;
        maxWidth = std::max(maxWidth, indent + measureTextWidth(d, drawable, gc, theme, label) + 36);
    }
    setAutoScroll(true);
    setVirtualSize(std::max(r.width, maxWidth), std::max(r.height, static_cast<int>(rows.size()) * rowHeight + 16));

    const int viewportLeft = r.x + 4;
    const int viewportRight = r.x + r.width - 14;
    const int viewportTop = r.y + 4;
    const int viewportBottom = r.y + r.height - 14;
    int saved = SaveDC(drawable);
    IntersectClipRect(drawable, viewportLeft, viewportTop, viewportRight, viewportBottom);
    int y = r.y + 20 - scrollY_;
    for (size_t i = 0; i < rows.size(); ++i, y += rowHeight) {
        if (y < r.y + 8) {
            continue;
        }
        if (y >= r.y + r.height - 8) {
            break;
        }
        const bool selected = multiSelect_ ? selectedVisibleRows_.count(static_cast<int>(i)) != 0U : static_cast<int>(i) == selectedVisibleRow_;
        const bool hovered = static_cast<int>(i) == hoveredVisibleRow_;
        if (selected || hovered) {
            RECT rr{viewportLeft, y - 18, viewportRight, y + 3};
            HBRUSH b = CreateSolidBrush(rgb(selected ? theme.pressed : theme.hover));
            FillRect(drawable, &rr, b);
            DeleteObject(b);
        }
        const auto& row = rows[i];
        const int indent = row.depth * 18;
        const int textX = r.x + 10 + indent - scrollX_;
        const std::string label = (row.hasChildren ? (isPathCollapsed(row.path) ? "+ " : "- ") : "  ") + row.label;
        RECT textClip = safeRectWin32(neuMaxIntWin32(textX, viewportLeft), y - 18, viewportRight, y + 3);
        if (textClip.right > textClip.left + 2) {
            int cellSaved = SaveDC(drawable);
            const int clipTop = neuMaxIntWin32(static_cast<int>(textClip.top), viewportTop);
            const int clipBottom = neuMinIntWin32(static_cast<int>(textClip.bottom), viewportBottom);
            IntersectClipRect(drawable, static_cast<int>(textClip.left), clipTop, static_cast<int>(textClip.right), clipBottom);
            const int drawX = neuMaxIntWin32(textX, static_cast<int>(textClip.left) + 2);
            const int visibleWidth = neuMaxIntWin32(1, static_cast<int>(textClip.right) - drawX - 2);
            drawText(d,
                     drawable,
                     gc,
                     theme,
                     truncateTextToWidth(d, drawable, gc, theme, label, visibleWidth),
                     drawX,
                     y - 14);
            RestoreDC(drawable, cellSaved);
        }
    }
    RestoreDC(drawable, saved);
    drawScrollbars(d, drawable, gc, theme);
    drawHintPopup(d, drawable, gc, theme);
}

void Neu_TreeView::handleXEvent(XEvent& ev)
{
    Neu_Control::handleXEvent(ev);
    if (!model()) {
        return;
    }
    if (!contains(ev.x, ev.y)) {
        if (ev.message == WM_MOUSELEAVE && hoveredVisibleRow_ != -1) {
            hoveredVisibleRow_ = -1;
            requestRedraw();
        }
        return;
    }

    Neu_Rect r = bounds();
    const int rowIndex = (ev.y - r.y + scrollY_) / 22;
    const auto rows = buildTreeRowsWin(model(), collapsedPaths_);
    const bool validRow = rowIndex >= 0 && rowIndex < static_cast<int>(rows.size());

    if (ev.message == WM_MOUSEMOVE) {
        const int newHover = validRow ? rowIndex : -1;
        if (newHover != hoveredVisibleRow_) {
            hoveredVisibleRow_ = newHover;
            requestRedraw();
        }
        return;
    }

    if (ev.message == WM_LBUTTONUP && validRow) {
        selectedVisibleRow_ = rowIndex;
        if (multiSelect_) {
            if (shiftDownWin32() && anchorVisibleRow_ >= 0) {
                selectedVisibleRows_.clear();
                const int a = std::min(anchorVisibleRow_, rowIndex);
                const int b = std::max(anchorVisibleRow_, rowIndex);
                for (int i = a; i <= b; ++i) {
                    selectedVisibleRows_.insert(i);
                }
            } else if (ctrlDownWin32()) {
                if (selectedVisibleRows_.count(rowIndex) != 0U) {
                    selectedVisibleRows_.erase(rowIndex);
                } else {
                    selectedVisibleRows_.insert(rowIndex);
                }
                anchorVisibleRow_ = rowIndex;
            } else {
                selectedVisibleRows_.clear();
                selectedVisibleRows_.insert(rowIndex);
                anchorVisibleRow_ = rowIndex;
            }
        } else {
            selectedVisibleRows_.clear();
            selectedVisibleRows_.insert(rowIndex);
            anchorVisibleRow_ = rowIndex;
        }

        const auto& row = rows[static_cast<size_t>(rowIndex)];
        if (row.hasChildren && !ctrlDownWin32() && !shiftDownWin32()) {
            toggleNodePath(row.path);
        }
        if (callbacks_.onSelectionChanged) {
            callbacks_.onSelectionChanged(this,
                                          static_cast<int>(row.modelRow),
                                          0,
                                          row.label.c_str(),
                                          callbacks_.userData);
        }
        requestRedraw();
    }
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
    value_ = std::max(0, std::min(value, std::max(0, total_ - page_)));
    requestRedraw();
}

void Neu_ScrollBar::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    Neu_Rect r = bounds();
    const int track = std::max(1, vertical_ ? r.height : r.width);
    const int thumb = std::max(18, track * page_ / std::max(1, total_));
    const int maxValue = std::max(1, total_ - page_);
    const int pos = (track - thumb) * value_ / maxValue;
    HBRUSH b = CreateSolidBrush(rgb(theme.accent));
    if (vertical_) {
        RECT th{r.x + 2, r.y + pos, r.x + r.width - 2, r.y + pos + thumb};
        FillRect(drawable, &th, b);
    } else {
        RECT th{r.x + pos, r.y + 2, r.x + pos + thumb, r.y + r.height - 2};
        FillRect(drawable, &th, b);
    }
    DeleteObject(b);
}

void Neu_ScrollBar::handleXEvent(XEvent& ev)
{
    if ((ev.message == WM_LBUTTONDOWN || ev.message == WM_MOUSEMOVE) && (dragging_ || contains(ev.x, ev.y))) {
        Neu_Rect r = bounds();
        const int coordinate = vertical_ ? ev.y - r.y : ev.x - r.x;
        const int track = std::max(1, vertical_ ? r.height : r.width);
        const int maxValue = std::max(1, total_ - page_);
        if (ev.message == WM_LBUTTONDOWN) {
            dragging_ = true;
        }
        setRange(total_, page_, coordinate * maxValue / track);
        return;
    }
    if (ev.message == WM_LBUTTONUP) {
        dragging_ = false;
    }
    Neu_Control::handleXEvent(ev);
}

void Neu_ScrollWindow::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    Neu_Rect r = bounds();
    const int viewportLeft = r.x + 4;
    const int viewportTop = r.y + 4;
    const int viewportRight = r.x + r.width - 14;
    const int viewportBottom = r.y + r.height - 14;
    int saved = SaveDC(drawable);
    IntersectClipRect(drawable, viewportLeft, viewportTop, viewportRight, viewportBottom);
    int maxRight = r.width;
    int maxBottom = r.height;
    for (const auto& child : children()) {
        if (child && child->visible()) {
            Neu_Layout original = child->layout();
            Neu_Layout shifted = original;
            shifted.left -= scrollX();
            shifted.top -= scrollY();
            child->setLayout(shifted);
            Neu_Rect cr = child->bounds();
            const bool intersects = cr.x + cr.width >= viewportLeft
                                    && cr.y + cr.height >= viewportTop
                                    && cr.x <= viewportRight
                                    && cr.y <= viewportBottom;
            if (intersects) {
                int childSaved = SaveDC(drawable);
                IntersectClipRect(drawable, viewportLeft, viewportTop, viewportRight, viewportBottom);
                child->draw(d, drawable, gc, theme);
                RestoreDC(drawable, childSaved);
                IntersectClipRect(drawable, viewportLeft, viewportTop, viewportRight, viewportBottom);
            }
            maxRight = std::max(maxRight, original.left - r.x + original.width + 20);
            maxBottom = std::max(maxBottom, original.top - r.y + original.height + 20);
            child->setLayout(original);
        }
    }
    RestoreDC(drawable, saved);
    setAutoScroll(true);
    setVirtualSize(std::max(virtualSize().width, maxRight), std::max(virtualSize().height, maxBottom));
    drawScrollbars(d, drawable, gc, theme);
    drawHintPopup(d, drawable, gc, theme);
}

void Neu_ScrollWindow::handleXEvent(XEvent& ev)
{
    const bool inside = contains(ev.x, ev.y);
    Neu_Control::handleXEvent(ev);
    if (!inside || activeScrollDrag_ != 0 || ev.message == WM_MOUSEWHEEL || ev.message == WM_MOUSEHWHEEL) {
        return;
    }

    XEvent adjusted = ev;
    if (ev.message == WM_MOUSEMOVE || ev.message == WM_LBUTTONDOWN || ev.message == WM_LBUTTONUP) {
        adjusted.x += scrollX();
        adjusted.y += scrollY();
        for (auto it = children().rbegin(); it != children().rend(); ++it) {
            auto& child = *it;
            if (child && child->visible() && child->enabled() && child->contains(adjusted.x, adjusted.y)) {
                child->handleXEvent(adjusted);
                return;
            }
        }
    }
}

void Neu_Label::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Rect r = bounds();
    if (borderVisible_) {
        HPEN pen = CreatePen(PS_SOLID, 1, rgb(theme.border));
        HGDIOBJ oldPen = SelectObject(drawable, pen);
        HGDIOBJ oldBrush = SelectObject(drawable, GetStockObject(NULL_BRUSH));
        RoundRect(drawable, r.x, r.y, r.x + r.width, r.y + r.height, 8, 8);
        SelectObject(drawable, oldBrush);
        SelectObject(drawable, oldPen);
        DeleteObject(pen);
    }

    const int iconSpace = icon().width() > 0 ? 24 : 0;
    if (icon().width() > 0) {
        drawIconBmp(d, drawable, gc, r.x + 2, r.y + std::max(0, (r.height - 16) / 2), 16);
    }
    const int textLeft = r.x + iconSpace + 2;
    const int textRight = r.x + r.width - 2;
    const int width = std::max(1, textRight - textLeft);
    int saved = SaveDC(drawable);
    IntersectClipRect(drawable, textLeft, r.y, textRight, r.y + r.height);
    int x = textLeft;
    if (!richTextFragments().empty()) {
        for (const auto& f : richTextFragments()) {
            if (x >= textRight) {
                break;
            }
            const int remaining = std::max(1, textRight - x);
            const std::string visible = truncateTextToWidth(d, drawable, gc, theme, f.text, remaining);
            drawTextColored(d,
                            drawable,
                            gc,
                            theme,
                            visible,
                            x,
                            r.y + std::max(4, (r.height - 16) / 2),
                            f.useFontColor ? f.fontColor : theme.text,
                            f.bold,
                            f.italic,
                            f.underline,
                            f.strikethrough,
                            f.doubleStrikethrough,
                            f.monospace,
                            f.headingLevel);
            x += measureTextWidth(d, drawable, gc, theme, visible, f.bold, f.italic, f.monospace, f.headingLevel) + 2;
        }
    } else if (wordWrap_) {
        auto lines = wrapTextToWidth(d, drawable, gc, theme, text(), width);
        int y = r.y + 2;
        for (const auto& line : lines) {
            if (y > r.y + r.height - 12) {
                break;
            }
            const std::string visible = truncateTextToWidth(d, drawable, gc, theme, line, width);
            drawText(d, drawable, gc, theme, visible, alignedTextX(d, drawable, gc, theme, visible, textLeft, width), y);
            y += 18;
        }
    } else {
        const std::string visible = truncateTextToWidth(d, drawable, gc, theme, text(), width);
        drawText(d, drawable, gc, theme, visible, alignedTextX(d, drawable, gc, theme, visible, textLeft, width), r.y + std::max(4, (r.height - 16) / 2));
    }
    RestoreDC(drawable, saved);
    drawHintPopup(d, drawable, gc, theme);
}

void Neu_MultilineLabel::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Rect r = bounds();
    if (borderVisible_) {
        HPEN pen = CreatePen(PS_SOLID, 1, rgb(theme.border));
        HGDIOBJ oldPen = SelectObject(drawable, pen);
        HGDIOBJ oldBrush = SelectObject(drawable, GetStockObject(NULL_BRUSH));
        RoundRect(drawable, r.x, r.y, r.x + r.width, r.y + r.height, 8, 8);
        SelectObject(drawable, oldBrush);
        SelectObject(drawable, oldPen);
        DeleteObject(pen);
    }
    const int iconSpace = icon().width() > 0 ? 26 : 0;
    if (icon().width() > 0) {
        drawIconBmp(d, drawable, gc, r.x + 2, r.y + 4, 20);
    }
    const int textLeft = r.x + iconSpace + 2;
    const int textRight = r.x + r.width - 12;
    const int contentWidth = std::max(1, textRight - textLeft);
    int saved = SaveDC(drawable);
    IntersectClipRect(drawable, textLeft, r.y, textRight, r.y + r.height);
    std::vector<std::string> lines;
    auto logical = logicalLinesWin32(text());
    for (const auto& base : logical) {
        auto wrapped = wrapTextToWidth(d, drawable, gc, theme, base, contentWidth);
        lines.insert(lines.end(), wrapped.begin(), wrapped.end());
    }
    int y = r.y + 2 - scrollY();
    for (const auto& line : lines) {
        if (y >= r.y && y < r.y + r.height - 12) {
            const std::string visible = truncateTextToWidth(d, drawable, gc, theme, line, contentWidth);
            drawText(d, drawable, gc, theme, visible, textLeft, y);
        }
        y += 18;
    }
    RestoreDC(drawable, saved);
    setAutoScroll(true);
    setVirtualSize(r.width, std::max(r.height, static_cast<int>(lines.size()) * 18 + 12));
    drawScrollbars(d, drawable, gc, theme);
    drawHintPopup(d, drawable, gc, theme);
}

void Neu_RichTextCode::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    Neu_Rect r = bounds();
    const int toolbarH = toolbarVisible_ ? 34 : 0;
    if (toolbarVisible_) {
        RECT tb{r.x + 2, r.y + 2, r.x + r.width - 2, r.y + toolbarH};
        HBRUSH b = CreateSolidBrush(RGB(224, 232, 244));
        FillRect(drawable, &tb, b);
        DeleteObject(b);
        const char* tools[] = {"B", "I", "U", "S", "DS", "H1", "H2", "Mono", "Font", "Text", "BG", "HL", "Left", "Center", "Right", "Wrap"};
        int tx = r.x + 8;
        for (const char* tool : tools) {
            RECT br{tx, r.y + 7, tx + 42, r.y + 27};
            FrameRect(drawable, &br, reinterpret_cast<HBRUSH>(GetStockObject(GRAY_BRUSH)));
            drawText(d, drawable, gc, theme, tool, tx + 4, r.y + 9);
            tx += 46;
            if (tx > r.x + r.width - 50) {
                break;
            }
        }
    }

    const int contentLeft = r.x + 56;
    const int contentRight = r.x + r.width - 14;
    const int contentTop = r.y + toolbarH + 4;
    const int contentBottom = r.y + r.height - 14;
    const int contentWidth = std::max(1, contentRight - contentLeft);
    int y = contentTop + 2 - scrollY();
    int maxWidth = r.width;
    int logicalLineNo = 1;

    int saved = SaveDC(drawable);
    IntersectClipRect(drawable, r.x + 4, contentTop, contentRight, contentBottom);

    auto drawCodeLine = [&](const std::string& line,
                            const Neu_TextFragment* fragment,
                            int lineHeight,
                            int lineNo) {
        const std::vector<std::string> visualLines = wordWrap_ ? wrapTextToWidth(d, drawable, gc, theme, line, contentWidth)
                                                              : std::vector<std::string>{line};
        for (const auto& visualLine : visualLines) {
            const bool visibleY = y >= contentTop && y < contentBottom;
            const bool bold = fragment ? fragment->bold : false;
            const bool italic = fragment ? fragment->italic : false;
            const bool underline = fragment ? fragment->underline : false;
            const bool strike = fragment ? fragment->strikethrough : false;
            const bool dstrike = fragment ? fragment->doubleStrikethrough : false;
            const bool mono = fragment ? fragment->monospace : true;
            const int heading = fragment ? fragment->headingLevel : 0;
            const Neu_Color color = fragment && fragment->useFontColor ? fragment->fontColor : defaultFontColor_;
            const int measured = measureTextWidth(d, drawable, gc, theme, visualLine, bold, italic, mono, heading);
            maxWidth = std::max(maxWidth, measured + 82);
            if (visibleY) {
                drawText(d, drawable, gc, theme, std::to_string(lineNo), r.x + 8, y);
                int textSaved = SaveDC(drawable);
                IntersectClipRect(drawable, contentLeft, contentTop, contentRight, contentBottom);
                if (fragment && fragment->useHighlightColor) {
                    HBRUSH hb = CreateSolidBrush(rgb(fragment->highlightColor));
                    RECT hr{contentLeft - scrollX(), y - lineHeight + 5, std::min(contentRight, contentLeft - scrollX() + measured + 4), y + 4};
                    FillRect(drawable, &hr, hb);
                    DeleteObject(hb);
                }
                const std::string rendered = wordWrap_ ? visualLine : truncateTextToWidth(d, drawable, gc, theme, visualLine, std::max(contentWidth, measured));
                int tx = contentLeft - (wordWrap_ ? 0 : scrollX());
                if (textAlignment_ != Neu_TextAlignment::Left && wordWrap_) {
                    tx = alignedTextX(d, drawable, gc, theme, rendered, contentLeft, contentWidth);
                }
                drawTextColored(d, drawable, gc, theme, rendered, tx, y, color, bold, italic, underline, strike, dstrike, mono, heading);
                RestoreDC(drawable, textSaved);
            }
            y += lineHeight;
        }
    };

    if (!richTextFragments().empty()) {
        for (const auto& f : richTextFragments()) {
            const auto fragmentLines = logicalLinesWin32(f.text);
            const int lineHeight = lineHeightForHeadingWin32(f.headingLevel);
            for (const auto& line : fragmentLines) {
                drawCodeLine(line, &f, lineHeight, logicalLineNo++);
            }
        }
    } else {
        const auto lines = logicalLinesWin32(text_);
        for (const auto& line : lines) {
            drawCodeLine(line, nullptr, 20, logicalLineNo++);
        }
    }

    if (!readOnly_ && focused_) {
        const size_t caretBytes = std::min(cursor_, text_.size());
        size_t lineIndex = 0;
        size_t lineStart = 0;
        for (size_t i = 0; i < caretBytes; ++i) {
            if (text_[i] == '\n') {
                ++lineIndex;
                lineStart = i + 1;
            }
        }
        const std::string prefix = text_.substr(lineStart, caretBytes - lineStart);
        const int caretX = contentLeft + measureTextWidth(d, drawable, gc, theme, prefix, false, false, true) - (wordWrap_ ? 0 : scrollX());
        const int caretY = contentTop + 2 + static_cast<int>(lineIndex) * 20 - scrollY();
        if (caretY >= contentTop && caretY < contentBottom) {
            HPEN pen = CreatePen(PS_SOLID, 1, rgb(theme.accent));
            HGDIOBJ old = SelectObject(drawable, pen);
            MoveToEx(drawable, caretX, caretY - 2, nullptr);
            LineTo(drawable, caretX, caretY + 16);
            SelectObject(drawable, old);
            DeleteObject(pen);
        }
    }

    RestoreDC(drawable, saved);
    setAutoScroll(true);
    setVirtualSize(std::max(r.width, maxWidth), std::max(r.height, y - r.y + scrollY() + 20));
    drawScrollbars(d, drawable, gc, theme);
    drawHintPopup(d, drawable, gc, theme);
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
    const int side = std::max(8, std::min(r.width, r.height) - 18);
    const int x = r.x + (r.width - side) / 2;
    const int y = r.y + (r.height - side) / 2;
    const int cx = x + side / 2;
    const int perimeter = side / 2 + side + side + side + side / 2;
    int remaining = static_cast<int>(perimeter * progress_);
    HPEN pen = CreatePen(PS_SOLID, 5, rgb(progress_ > 0.85f ? Neu_Color{255, 215, 60, 255} : theme.accent));
    HGDIOBJ old = SelectObject(drawable, pen);
    auto drawPart = [&](int x1, int y1, int x2, int y2) {
        if (remaining <= 0) return;
        int len = std::max(std::abs(x2 - x1), std::abs(y2 - y1));
        int used = std::min(remaining, len);
        int ex = x1 + (len ? (x2 - x1) * used / len : 0);
        int ey = y1 + (len ? (y2 - y1) * used / len : 0);
        MoveToEx(drawable, x1, y1, nullptr);
        LineTo(drawable, ex, ey);
        remaining -= used;
    };
    drawPart(cx, y, x + side, y);
    drawPart(x + side, y, x + side, y + side);
    drawPart(x + side, y + side, x, y + side);
    drawPart(x, y + side, x, y);
    drawPart(x, y, cx, y);
    SelectObject(drawable, old);
    DeleteObject(pen);
    drawText(d, drawable, gc, theme, std::to_string(static_cast<int>(progress_ * 100.0f)) + "%", x + side / 2 - 16, y + side / 2 - 8);
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

void Neu_ReadOnlyRichText::crlf()
{
    contentCursorX_ = 12;
    contentCursorY_ += 30 + labelLineSpacing_;
    appendNextInline_ = false;
}

void Neu_ReadOnlyRichText::addLabel(const std::string& text)
{
    const std::string clean = unescapeHashesWin32(text);
    const int available = std::max(40, layout().width - 36);
    int width = std::min(available, std::max(60, static_cast<int>(clean.size()) * 8 + 30));
    if (!appendNextInline_) {
        contentCursorX_ = 12;
    }
    if (contentCursorX_ + width > layout().width - 12) {
        contentCursorX_ = 12;
        contentCursorY_ += 30 + labelLineSpacing_;
    }
    auto l = std::make_shared<Neu_Label>(Neu_Layout{layout().left + contentCursorX_, layout().top + contentCursorY_, width, 28});
    l->setText(clean);
    l->setTextTruncation(true);
    if (!iconPaths_.empty()) {
        l->setIconBmp(iconPaths_[iconIndexForText(text)]);
    }
    add(l);
    if (appendNextInline_) {
        contentCursorX_ += width + labelSpacing_;
        appendNextInline_ = false;
    } else {
        contentCursorX_ = 12;
        contentCursorY_ += 30 + labelLineSpacing_;
    }
    setContentSize(layout().width, contentCursorY_ + 50);
}

void Neu_ReadOnlyRichText::addMultilineLabel(const std::string& text)
{
    const std::string clean = unescapeHashesWin32(text);
    const int available = std::max(40, layout().width - 36);
    if (!appendNextInline_) {
        contentCursorX_ = 12;
    }
    int estimatedLines = 1;
    for (char ch : clean) {
        if (ch == '\n') ++estimatedLines;
    }
    estimatedLines += static_cast<int>(clean.size()) / std::max(1, available / 8);
    const int height = std::max(56, estimatedLines * (18 + labelLineSpacing_) + 12);
    auto l = std::make_shared<Neu_MultilineLabel>(Neu_Layout{layout().left + contentCursorX_, layout().top + contentCursorY_, available, height});
    l->setText(clean);
    l->setWordWrap(true);
    l->setAutoScroll(true);
    if (!iconPaths_.empty()) {
        l->setIconBmp(iconPaths_[iconIndexForText(text)]);
    }
    add(l);
    if (appendNextInline_) {
        contentCursorX_ += available + labelSpacing_;
        appendNextInline_ = false;
    } else {
        contentCursorX_ = 12;
        contentCursorY_ += height + labelLineSpacing_;
    }
    setContentSize(layout().width, contentCursorY_ + 50);
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
    auto hitOne = [&](auto&& self, const std::shared_ptr<Neu_Control>& control, int px, int py) -> Neu_Control* {
        if (!control || !control->visible() || !control->enabled()) {
            return nullptr;
        }
        if (!control->contains(px, py)) {
            return nullptr;
        }

        if (auto scroll = dynamic_cast<Neu_ScrollWindow*>(control.get())) {
            const int childX = px + scroll->scrollX();
            const int childY = py + scroll->scrollY();
            const auto& children = scroll->children();
            for (auto child = children.rbegin(); child != children.rend(); ++child) {
                Neu_Control* hit = self(self, *child, childX, childY);
                if (hit) {
                    return hit;
                }
            }
            return scroll;
        }

        if (auto placement = dynamic_cast<Neu_Placement*>(control.get())) {
            const auto& children = placement->children();
            for (auto child = children.rbegin(); child != children.rend(); ++child) {
                Neu_Control* hit = self(self, *child, px, py);
                if (hit) {
                    return hit;
                }
            }
        }

        return control.get();
    };

    for (auto it = controls_.rbegin(); it != controls_.rend(); ++it) {
        Neu_Control* hit = hitOne(hitOne, *it, x, y);
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

    if (ev.message == WM_TIMER) {
        if (hoveredControl_) {
            requestRedraw();
        } else if (window_) {
            KillTimer(window_, 1);
        }
        return;
    }

    if (ev.message == WM_MOUSEMOVE) {
        TRACKMOUSEEVENT tme{};
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = window_;
        TrackMouseEvent(&tme);

        Neu_Control* hoverTarget = hitTest(ev.x, ev.y);
        Neu_Control* target = captureControl_ ? captureControl_ : hoverTarget;
        if (hoverTarget != hoveredControl_) {
            if (hoveredControl_) {
                XEvent leave = ev;
                leave.message = WM_MOUSELEAVE;
                hoveredControl_->handleXEvent(leave);
            }
            hoveredControl_ = hoverTarget;
        }
        if (target) {
            target->handleXEvent(ev);
            SetTimer(window_, 1, 5000, nullptr);
        } else {
            KillTimer(window_, 1);
        }
        return;
    }

    if (ev.message == WM_MOUSELEAVE) {
        if (hoveredControl_) {
            hoveredControl_->handleXEvent(ev);
            hoveredControl_ = nullptr;
        }
        KillTimer(window_, 1);
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
