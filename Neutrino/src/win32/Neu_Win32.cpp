#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Neutrino/Neutrino.hpp"
#include <windowsx.h>
#include <commctrl.h>
#include <algorithm>
#include <cctype>
#include <sstream>

namespace neutrino {

static Neu_SmoothGraphicsOptions g_options{};
static const wchar_t* kNeuWindowClass = L"Neutrino_Neu_Window";
static HINSTANCE g_instance = nullptr;

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

static std::string toUtf8(wchar_t ch)
{
    char buffer[8]{};
    int n = WideCharToMultiByte(CP_UTF8, 0, &ch, 1, buffer, static_cast<int>(sizeof(buffer)), nullptr, nullptr);
    return n > 0 ? std::string(buffer, static_cast<size_t>(n)) : std::string();
}

static COLORREF rgb(const Neu_Color& c)
{
    return RGB(c.r, c.g, c.b);
}

static Neu_Rect intersectRects(const Neu_Rect& a, const Neu_Rect& b)
{
    const int left = std::max(a.x, b.x);
    const int top = std::max(a.y, b.y);
    const int right = std::min(a.x + a.width, b.x + b.width);
    const int bottom = std::min(a.y + a.height, b.y + b.height);
    return Neu_Rect{left, top, std::max(0, right - left), std::max(0, bottom - top)};
}

static HFONT makeFont(const Neu_Theme& theme, uint32_t style = Neu_TextStyle_Normal, int headingLevel = 0, const std::string& overrideName = std::string())
{
    int height = headingLevel > 0 ? -(28 - std::min(7, headingLevel) * 2) : -16;
    int weight = ((style & Neu_TextStyle_Bold) != 0U || headingLevel > 0) ? FW_BOLD : FW_NORMAL;
    BOOL italic = (style & Neu_TextStyle_Italic) != 0U ? TRUE : FALSE;
    BOOL underline = (style & Neu_TextStyle_Underline) != 0U ? TRUE : FALSE;
    BOOL strike = (style & Neu_TextStyle_Strikethrough) != 0U || (style & Neu_TextStyle_DoubleStrikethrough) != 0U ? TRUE : FALSE;
    std::wstring face = L"Segoe UI";
    if ((style & Neu_TextStyle_Monospaced) != 0U) {
        face = L"Consolas";
    } else if (!overrideName.empty()) {
        face = toWide(overrideName);
    } else if (!theme.fontName.empty() && theme.fontName.find(':') == std::string::npos) {
        face = toWide(theme.fontName);
    }
    return CreateFontW(height, 0, 0, 0, weight, italic, underline, strike, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
                       CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, face.c_str());
}

static int textWidthHdc(HDC hdc, const std::string& text, HFONT font)
{
    HGDIOBJ old = SelectObject(hdc, font);
    SIZE size{};
    std::wstring w = toWide(text);
    GetTextExtentPoint32W(hdc, w.c_str(), static_cast<int>(w.size()), &size);
    SelectObject(hdc, old);
    return size.cx;
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

static void withClip(HDC hdc, const Neu_Rect& r, const std::function<void()>& fn)
{
    HRGN region = CreateRectRgn(r.x, r.y, r.x + std::max(0, r.width), r.y + std::max(0, r.height));
    SelectClipRgn(hdc, region);
    fn();
    SelectClipRgn(hdc, nullptr);
    DeleteObject(region);
}

static LRESULT CALLBACK Neu_WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    Neu_Window* win = reinterpret_cast<Neu_Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lp);
        win = reinterpret_cast<Neu_Window*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(win));
    }
    if (!win) {
        return DefWindowProc(hwnd, msg, wp, lp);
    }

    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);
        win->redraw();
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
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
        if (msg == WM_MOUSEWHEEL || msg == WM_MOUSEHWHEEL) {
            POINT pt{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
            ScreenToClient(hwnd, &pt);
            ev.x = pt.x;
            ev.y = pt.y;
        } else {
            ev.x = GET_X_LPARAM(lp);
            ev.y = GET_Y_LPARAM(lp);
        }
        win->handleXEvent(ev);
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwnd, msg, wp, lp);
    }
}

Neu_Application* Neu_Application::current_ = nullptr;

Neu_Application::Neu_Application()
{
    current_ = this;
}

Neu_Application::~Neu_Application()
{
    current_ = nullptr;
}

bool Neu_Application::open()
{
    g_instance = GetModuleHandle(nullptr);
    INITCOMMONCONTROLSEX icc{};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);

    SetProcessDPIAware();
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = Neu_WindowProc;
    wc.hInstance = g_instance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = kNeuWindowClass;
    RegisterClassExW(&wc);
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

void Neu_Application::registerWindow(Neu_Window* win)
{
    if (std::find(windows_.begin(), windows_.end(), win) == windows_.end()) {
        windows_.push_back(win);
    }
}

void Neu_Application::unregisterWindow(Neu_Window* win)
{
    windows_.erase(std::remove(windows_.begin(), windows_.end(), win), windows_.end());
    if (windows_.empty()) {
        running_ = false;
        PostQuitMessage(0);
    }
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
    return Neu_Color{static_cast<uint8_t>(GetRValue(pixel)), static_cast<uint8_t>(GetGValue(pixel)), static_cast<uint8_t>(GetBValue(pixel)), 255};
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
    }
}

void Neu_EnableMultiStageDoubleBuffering(bool enabled)
{
    g_options.multiStageDoubleBuffering = enabled;
}

void Neu_DrawRoundedRect(Display*, Drawable drawable, GC, int x, int y, int w, int h, int radius, bool fill)
{
    fillRound(drawable, Neu_Rect{x, y, w, h}, radius, fill ? RGB(236, 244, 255) : RGB(255, 255, 255), RGB(110, 130, 160));
}

void Neu_DrawSmoothRoundedRect(Display*, Drawable drawable, GC, const Neu_Color& color, const Neu_Color&, int x, int y, int w, int h, int radius, bool, int)
{
    fillRound(drawable, Neu_Rect{x, y, w, h}, radius, rgb(color), RGB(110, 130, 160));
}

void Neu_DrawSmoothDropShadow(Display*, Drawable drawable, GC, const Neu_Color& shadow, const Neu_Color&, int x, int y, int w, int h, int radius, int, int offsetX, int offsetY)
{
    fillRound(drawable, Neu_Rect{x + offsetX, y + offsetY, w, h}, radius, rgb(shadow), rgb(shadow));
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
    HANDLE f = CreateFileW(toWide(path).c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE) {
        return false;
    }
    DWORD size = GetFileSize(f, nullptr);
    if (size < 54) {
        CloseHandle(f);
        return false;
    }
    std::vector<uint8_t> bytes(size);
    DWORD read = 0;
    ReadFile(f, bytes.data(), size, &read, nullptr);
    CloseHandle(f);
    if (read < 54 || bytes[0] != 'B' || bytes[1] != 'M') {
        return false;
    }
    uint32_t off = *reinterpret_cast<uint32_t*>(&bytes[10]);
    int32_t bw = *reinterpret_cast<int32_t*>(&bytes[18]);
    int32_t bh = *reinterpret_cast<int32_t*>(&bytes[22]);
    uint16_t bpp = *reinterpret_cast<uint16_t*>(&bytes[28]);
    if (bw <= 0 || bh == 0 || (bpp != 24 && bpp != 32) || off >= read) {
        return false;
    }
    w_ = bw;
    h_ = std::abs(bh);
    pixels_.assign(static_cast<size_t>(w_ * h_), 0xff000000u);
    int stride = ((w_ * bpp + 31) / 32) * 4;
    bool bottomUp = bh > 0;
    for (int y = 0; y < h_; ++y) {
        int sy = bottomUp ? (h_ - 1 - y) : y;
        const uint8_t* row = bytes.data() + off + static_cast<size_t>(sy * stride);
        for (int x = 0; x < w_; ++x) {
            uint8_t b = row[x * (bpp / 8) + 0];
            uint8_t g = row[x * (bpp / 8) + 1];
            uint8_t r = row[x * (bpp / 8) + 2];
            uint8_t a = bpp == 32 ? row[x * 4 + 3] : 255;
            pixels_[static_cast<size_t>(y * w_ + x)] = (static_cast<uint32_t>(a) << 24) | (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) | b;
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
        double d = std::strtod(text.c_str(), &end);
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

Neu_Control::Neu_Control(Neu_Layout layout) : layout_(layout) {}

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

void Neu_Control::requestRedraw()
{
    if (parent_) {
        parent_->redraw();
    }
}

void Neu_Control::drawText(Display*, Drawable drawable, GC, const Neu_Theme& theme, const std::string& s, int x, int y)
{
    SetBkMode(drawable, TRANSPARENT);
    SetTextColor(drawable, rgb(theme.text));
    HFONT font = makeFont(theme);
    HGDIOBJ old = SelectObject(drawable, font);
    std::wstring w = toWide(s);
    TextOutW(drawable, x, y, w.c_str(), static_cast<int>(w.size()));
    SelectObject(drawable, old);
    DeleteObject(font);
}

int Neu_Control::approximateTextWidth(const std::string& text, uint32_t style, int headingLevel) const
{
    int cw = (style & Neu_TextStyle_Monospaced) != 0U ? 8 : 7;
    if ((style & Neu_TextStyle_Bold) != 0U) {
        ++cw;
    }
    if (headingLevel > 0) {
        cw += std::max(0, 8 - headingLevel);
    }
    return static_cast<int>(text.size()) * cw;
}

std::vector<std::string> Neu_Control::wrapTextToWidth(const std::string& text, int maxWidth) const
{
    std::vector<std::string> lines;
    const size_t maxChars = static_cast<size_t>(std::max(1, maxWidth / 7));
    std::stringstream paragraphs(text);
    std::string paragraph;
    while (std::getline(paragraphs, paragraph)) {
        std::istringstream words(paragraph);
        std::string word;
        std::string line;
        while (words >> word) {
            while (word.size() > maxChars) {
                if (!line.empty()) {
                    lines.push_back(line);
                    line.clear();
                }
                lines.push_back(word.substr(0, maxChars));
                word.erase(0, maxChars);
            }
            if (!line.empty() && line.size() + 1U + word.size() > maxChars) {
                lines.push_back(line);
                line.clear();
            }
            if (!line.empty()) {
                line += ' ';
            }
            line += word;
        }
        if (!line.empty()) {
            lines.push_back(line);
        }
        if (paragraph.empty()) {
            lines.emplace_back();
        }
    }
    if (lines.empty()) {
        lines.emplace_back();
    }
    return lines;
}

std::string Neu_Control::truncateTextToWidth(const std::string& text, int maxWidth) const
{
    if (approximateTextWidth(text) <= maxWidth) {
        return text;
    }
    const size_t keep = static_cast<size_t>(std::max(0, maxWidth / 7 - 3));
    return text.substr(0, std::min(keep, text.size())) + "...";
}

void Neu_Control::drawTextInRect(Display*, Drawable drawable, GC, const Neu_Theme& theme, const std::string& text, const Neu_Rect& rect, const Neu_TextLayoutOptions& options)
{
    if (rect.width <= 0 || rect.height <= 0) {
        return;
    }

    const Neu_Rect clip = intersectRects(rect, bounds());
    if (clip.width <= 0 || clip.height <= 0) {
        return;
    }

    RECT clipRc{clip.x, clip.y, clip.x + clip.width, clip.y + clip.height};
    if (hasBackgroundColor_) {
        HBRUSH brush = CreateSolidBrush(rgb(backgroundColor_));
        FillRect(drawable, &clipRc, brush);
        DeleteObject(brush);
    }
    if (hasHighlight_) {
        HBRUSH brush = CreateSolidBrush(rgb(highlightColor_));
        FillRect(drawable, &clipRc, brush);
        DeleteObject(brush);
    }

    Neu_Theme localTheme = theme;
    if (hasFontColor_) {
        localTheme.text = fontColor_;
    }
    SetTextColor(drawable, rgb(localTheme.text));
    SetBkMode(drawable, TRANSPARENT);
    HFONT font = makeFont(localTheme, Neu_TextStyle_Normal, 0, fontName_);
    HGDIOBJ old = SelectObject(drawable, font);
    DWORD flags = DT_NOPREFIX;
    flags |= options.wordWrap ? DT_WORDBREAK : DT_SINGLELINE;
    flags |= options.truncate ? DT_END_ELLIPSIS : 0;
    flags |= options.align == Neu_TextAlign::Center ? DT_CENTER : (options.align == Neu_TextAlign::Right ? DT_RIGHT : DT_LEFT);
    RECT rc{rect.x + options.padding, rect.y + options.padding, rect.x + rect.width - options.padding, rect.y + rect.height - options.padding};
    std::wstring w = toWide(text);
    withClip(drawable, clip, [&]() {
        DrawTextW(drawable, w.c_str(), static_cast<int>(w.size()), &rc, flags);
    });
    SelectObject(drawable, old);
    DeleteObject(font);
}

void Neu_Control::drawRichTextFragments(Display*, Drawable drawable, GC, const Neu_Theme& theme, const std::vector<Neu_RichTextFragment>& fragments, const Neu_Rect& rect, const Neu_TextLayoutOptions& options)
{
    if (fragments.empty() || options.wordWrap) {
        std::string combined;
        for (const auto& fragment : fragments) {
            combined += fragment.text;
        }
        drawTextInRect(nullptr, drawable, drawable, theme, combined.empty() ? text_ : combined, rect, options);
        return;
    }

    const Neu_Rect clip = intersectRects(rect, bounds());
    if (clip.width <= 0 || clip.height <= 0) {
        return;
    }

    withClip(drawable, clip, [&]() {
        int totalWidth = 0;
        for (const auto& fragment : fragments) {
            totalWidth += approximateTextWidth(fragment.text, fragment.style, fragment.headingLevel);
        }
        int x = rect.x + options.padding;
        if (options.align == Neu_TextAlign::Center) {
            x = rect.x + std::max(0, (rect.width - totalWidth) / 2);
        } else if (options.align == Neu_TextAlign::Right) {
            x = rect.x + rect.width - options.padding - totalWidth;
        }
        int y = rect.y + options.padding + 2;
        for (const auto& fragment : fragments) {
            if (x >= clip.x + clip.width) {
                break;
            }
            Neu_Theme localTheme = theme;
            if (fragment.hasFontColor) {
                localTheme.text = fragment.fontColor;
            }
            std::string t = options.truncate ? truncateTextToWidth(fragment.text, rect.x + rect.width - x) : fragment.text;
            int w = approximateTextWidth(t, fragment.style, fragment.headingLevel);
            RECT bg{x, rect.y + 2, x + w, rect.y + options.lineHeight + 2};
            if (fragment.hasBackgroundColor || fragment.hasHighlight) {
                HBRUSH brush = CreateSolidBrush(rgb(fragment.hasHighlight ? fragment.highlightColor : fragment.backgroundColor));
                FillRect(drawable, &bg, brush);
                DeleteObject(brush);
            }
            SetBkMode(drawable, TRANSPARENT);
            SetTextColor(drawable, rgb(localTheme.text));
            HFONT font = makeFont(localTheme, fragment.style, fragment.headingLevel, fragment.fontName);
            HGDIOBJ old = SelectObject(drawable, font);
            std::wstring wt = toWide(t);
            TextOutW(drawable, x, y, wt.c_str(), static_cast<int>(wt.size()));
            SelectObject(drawable, old);
            DeleteObject(font);
            HPEN pen = CreatePen(PS_SOLID, 1, rgb(localTheme.text));
            HGDIOBJ oldPen = SelectObject(drawable, pen);
            if ((fragment.style & Neu_TextStyle_Underline) != 0U) { MoveToEx(drawable, x, y + 17, nullptr); LineTo(drawable, x + w, y + 17); }
            if ((fragment.style & Neu_TextStyle_Strikethrough) != 0U) { MoveToEx(drawable, x, y + 8, nullptr); LineTo(drawable, x + w, y + 8); }
            if ((fragment.style & Neu_TextStyle_DoubleStrikethrough) != 0U) { MoveToEx(drawable, x, y + 6, nullptr); LineTo(drawable, x + w, y + 6); MoveToEx(drawable, x, y + 10, nullptr); LineTo(drawable, x + w, y + 10); }
            SelectObject(drawable, oldPen);
            DeleteObject(pen);
            x += w;
        }
    });
}

void Neu_Control::drawIconBmp(Display*, Drawable drawable, GC, int x, int y, int maxSize)
{
    if (icon_.width() <= 0 || icon_.height() <= 0) {
        return;
    }
    int sz = std::min(maxSize, std::min(icon_.width(), icon_.height()));
    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = icon_.width();
    bmi.bmiHeader.biHeight = -icon_.height();
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    StretchDIBits(drawable, x, y, sz, sz, 0, 0, icon_.width(), icon_.height(), icon_.pixels().data(), &bmi, DIB_RGB_COLORS, SRCCOPY);
}

void Neu_Control::drawShadow(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    if (!g_options.drawShadows) {
        return;
    }
    Neu_Rect r = bounds();
    Neu_DrawSmoothDropShadow(d, drawable, gc, theme.shadow, theme.background, r.x, r.y, r.width, r.height, theme.radius, theme.shadowSize, theme.shadowOffsetX, theme.shadowOffsetY);
}

void Neu_Control::drawHintPopup(Display*, Drawable drawable, GC, const Neu_Theme& theme)
{
    if (!g_options.drawHints || !hover_ || hintText_.empty()) {
        return;
    }
    Neu_Rect r = bounds();
    int w = 400;
    std::vector<std::string> lines = wrapTextToWidth(hintText_, w - 30);
    int naturalHeight = static_cast<int>(lines.size()) * 18 + 20;
    int h = std::min(500, naturalHeight);
    int x = r.x + r.width + 10;
    int y = r.y;
    if (parent_ && x + w > parent_->width()) {
        x = std::max(6, r.x - w - 10);
    }
    if (parent_ && y + h > parent_->height()) {
        y = std::max(6, parent_->height() - h - 6);
    }
    RECT box{x, y, x + w, y + h};
    HBRUSH b = CreateSolidBrush(rgb(theme.hintBackground));
    FillRect(drawable, &box, b);
    DeleteObject(b);
    FrameRect(drawable, &box, reinterpret_cast<HBRUSH>(GetStockObject(GRAY_BRUSH)));
    withClip(drawable, Neu_Rect{x + 8, y + 8, w - 28, h - 16}, [&]() {
        int cy = y + 8;
        int count = hintExpanded_ ? static_cast<int>(lines.size()) : std::min(3, static_cast<int>(lines.size()));
        for (int i = 0; i < count && cy < y + h - 8; ++i, cy += 18) {
            drawTextInRect(nullptr, drawable, drawable, theme, lines[static_cast<size_t>(i)], Neu_Rect{x + 8, cy, w - 34, 18}, Neu_TextLayoutOptions{false, true, Neu_TextAlign::Left, 18, 0});
        }
    });
    if (lines.size() > 3U) {
        drawText(nullptr, drawable, drawable, theme, hintExpanded_ ? "^" : "v", x + w - 24, y + h - 22);
    }
    if (naturalHeight > 500) {
        RECT track{x + w - 12, y + 12, x + w - 7, y + h - 12};
        HBRUSH tb = CreateSolidBrush(rgb(theme.border));
        FillRect(drawable, &track, tb);
        DeleteObject(tb);
    }
}

void Neu_Control::drawScrollbars(Display*, Drawable drawable, GC, const Neu_Theme& theme)
{
    if (!autoScroll_) {
        return;
    }
    Neu_Rect r = bounds();
    bool needV = virtualSize_.height > r.height;
    bool needH = virtualSize_.width > r.width;
    if (needV) {
        int trackH = std::max(1, r.height - 16);
        int thumbH = std::max(20, trackH * r.height / std::max(1, virtualSize_.height));
        int maxY = std::max(1, virtualSize_.height - r.height);
        int thumbY = r.y + 8 + (trackH - thumbH) * scrollY_ / maxY;
        RECT tr{r.x + r.width - 10, r.y + 8, r.x + r.width - 5, r.y + r.height - 8};
        RECT th{r.x + r.width - 11, thumbY, r.x + r.width - 4, thumbY + thumbH};
        HBRUSH b = CreateSolidBrush(RGB(205, 215, 226));
        FillRect(drawable, &tr, b);
        DeleteObject(b);
        b = CreateSolidBrush(rgb(theme.accent));
        FillRect(drawable, &th, b);
        DeleteObject(b);
    }
    if (needH) {
        int trackW = std::max(1, r.width - 16);
        int thumbW = std::max(20, trackW * r.width / std::max(1, virtualSize_.width));
        int maxX = std::max(1, virtualSize_.width - r.width);
        int thumbX = r.x + 8 + (trackW - thumbW) * scrollX_ / maxX;
        RECT tr{r.x + 8, r.y + r.height - 10, r.x + r.width - 8, r.y + r.height - 5};
        RECT th{thumbX, r.y + r.height - 11, thumbX + thumbW, r.y + r.height - 4};
        HBRUSH b = CreateSolidBrush(RGB(205, 215, 226));
        FillRect(drawable, &tr, b);
        DeleteObject(b);
        b = CreateSolidBrush(rgb(theme.accent));
        FillRect(drawable, &th, b);
        DeleteObject(b);
    }
}

bool Neu_Control::handleScrollMouseEvent(XEvent& ev)
{
    if (!autoScroll_) {
        return false;
    }
    Neu_Rect r = bounds();
    if ((ev.message == WM_MOUSEWHEEL || ev.message == WM_MOUSEHWHEEL) && contains(ev.x, ev.y)) {
        int delta = GET_WHEEL_DELTA_WPARAM(ev.wParam);
        if (ev.message == WM_MOUSEHWHEEL || (GetKeyState(VK_SHIFT) & 0x8000) != 0) {
            setScrollOffset(scrollX_ - delta / 4, scrollY_);
        } else {
            setScrollOffset(scrollX_, scrollY_ - delta / 4);
        }
        return true;
    }
    bool needV = virtualSize_.height > r.height;
    bool needH = virtualSize_.width > r.width;
    if (ev.message == WM_LBUTTONDOWN && contains(ev.x, ev.y)) {
        if (needV && ev.x >= r.x + r.width - 14) {
            scrollDrag_ = true;
            scrollDragVertical_ = true;
            scrollDragAnchor_ = ev.y;
            scrollDragStartValue_ = scrollY_;
            SetCapture(parent_ ? parent_->xid() : nullptr);
            return true;
        }
        if (needH && ev.y >= r.y + r.height - 14) {
            scrollDrag_ = true;
            scrollDragVertical_ = false;
            scrollDragAnchor_ = ev.x;
            scrollDragStartValue_ = scrollX_;
            SetCapture(parent_ ? parent_->xid() : nullptr);
            return true;
        }
    }
    if (ev.message == WM_MOUSEMOVE && scrollDrag_) {
        if (scrollDragVertical_ && needV) {
            int trackH = std::max(1, r.height - 16);
            int maxY = std::max(1, virtualSize_.height - r.height);
            setScrollOffset(scrollX_, scrollDragStartValue_ + (ev.y - scrollDragAnchor_) * maxY / trackH);
        } else if (needH) {
            int trackW = std::max(1, r.width - 16);
            int maxX = std::max(1, virtualSize_.width - r.width);
            setScrollOffset(scrollDragStartValue_ + (ev.x - scrollDragAnchor_) * maxX / trackW, scrollY_);
        }
        return true;
    }
    if (ev.message == WM_LBUTTONUP && scrollDrag_) {
        scrollDrag_ = false;
        ReleaseCapture();
        return true;
    }
    return false;
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

void Neu_Control::setScrollOffset(int x, int y)
{
    Neu_Rect r = bounds();
    int maxX = std::max(0, virtualSize_.width - r.width);
    int maxY = std::max(0, virtualSize_.height - r.height);
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
    Neu_Rect r = bounds();
    scrollX_ = std::max(0, std::min(scrollX_, std::max(0, virtualSize_.width - r.width)));
    scrollY_ = std::max(0, std::min(scrollY_, std::max(0, virtualSize_.height - r.height)));
}

void Neu_Control::draw(Display*, Drawable drawable, GC, const Neu_Theme& theme)
{
    Neu_Rect r = bounds();
    fillRound(drawable, r, theme.radius, rgb(hover_ ? theme.hover : (hasBackgroundColor_ ? backgroundColor_ : theme.glass)), rgb(theme.border));
}

void Neu_Control::handleXEvent(XEvent& ev)
{
    if (handleScrollMouseEvent(ev)) {
        return;
    }
    bool inside = contains(ev.x, ev.y);
    if (ev.message == WM_MOUSEMOVE) {
        if (inside && !hover_) {
            hover_ = true;
            if (callbacks_.onFocus) callbacks_.onFocus(this, callbacks_.userData);
            requestRedraw();
        } else if (!inside && hover_) {
            hover_ = false;
            if (callbacks_.onBlur) callbacks_.onBlur(this, callbacks_.userData);
            requestRedraw();
        }
    } else if (ev.message == WM_LBUTTONDOWN) {
        focused_ = inside;
        if (inside) {
            pressed_ = true;
        }
        requestRedraw();
    } else if (ev.message == WM_LBUTTONUP) {
        bool wasPressed = pressed_;
        pressed_ = false;
        if (inside && wasPressed) invokeClick();
        requestRedraw();
    } else if (ev.message == WM_MOUSELEAVE) {
        if (hover_) {
            hover_ = false;
            if (callbacks_.onBlur) callbacks_.onBlur(this, callbacks_.userData);
            requestRedraw();
        }
    }
}

void Neu_Button::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    Neu_Rect r = bounds();
    int tx = r.x + 10;
    if (icon().width() > 0) {
        drawIconBmp(d, drawable, gc, r.x + 8, r.y + std::max(2, (r.height - 18) / 2), 18);
        tx += 24;
    }
    drawTextInRect(d, drawable, gc, theme, text_, Neu_Rect{tx, r.y + 4, r.width - (tx - r.x) - 8, r.height - 8}, Neu_TextLayoutOptions{false, true, Neu_TextAlign::Center, 18, 0});
}

void Neu_Button::handleXEvent(XEvent& ev)
{
    Neu_Control::handleXEvent(ev);
}

void Neu_Textbox::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    Neu_Rect r = bounds();
    Neu_Rect tr{r.x + 8, r.y + 5, r.width - 16, r.height - 10};

    SetBkMode(drawable, TRANSPARENT);
    SetTextColor(drawable, rgb(hasFontColor_ ? fontColor_ : theme.text));
    HFONT font = makeFont(theme, Neu_TextStyle_Normal, 0, fontName_);
    HGDIOBJ oldFont = SelectObject(drawable, font);
    std::wstring w = toWide(text_);
    withClip(drawable, tr, [&]() {
        TextOutW(drawable, tr.x - scrollX_, tr.y + 2, w.c_str(), static_cast<int>(w.size()));
    });

    if (focused_) {
        int prefix = textWidthHdc(drawable, text_.substr(0, std::min(cursor_, text_.size())), font);
        int caretX = std::max(tr.x, std::min(tr.x + tr.width - 1, tr.x + prefix - scrollX_));
        HPEN pen = CreatePen(PS_SOLID, 1, rgb(theme.accent));
        HGDIOBJ oldPen = SelectObject(drawable, pen);
        MoveToEx(drawable, caretX, r.y + 6, nullptr);
        LineTo(drawable, caretX, r.y + r.height - 7);
        SelectObject(drawable, oldPen);
        DeleteObject(pen);
    }

    SelectObject(drawable, oldFont);
    DeleteObject(font);
}

void Neu_Textbox::handleXEvent(XEvent& ev)
{
    Neu_Control::handleXEvent(ev);
    if (ev.message == WM_LBUTTONDOWN && focused_) {
        Neu_Rect r = bounds();
        Neu_Rect tr{r.x + 8, r.y + 5, r.width - 16, r.height - 10};
        HFONT font = makeFont(Neu_Theme::Light(), Neu_TextStyle_Normal, 0, fontName_);
        HDC measureDc = parent_ ? GetDC(parent_->xid()) : GetDC(nullptr);
        int localX = std::max(0, ev.x - tr.x + scrollX_);
        cursor_ = text_.size();
        for (size_t i = 0; i <= text_.size(); ++i) {
            const int currentWidth = textWidthHdc(measureDc, text_.substr(0, i), font);
            if (currentWidth >= localX) {
                cursor_ = i;
                break;
            }
        }
        if (parent_) ReleaseDC(parent_->xid(), measureDc); else ReleaseDC(nullptr, measureDc);
        DeleteObject(font);
    }
    if (!focused_) {
        return;
    }
    if (ev.message == WM_KEYDOWN) {
        switch (ev.wParam) {
        case VK_LEFT: if (cursor_ > 0) --cursor_; break;
        case VK_RIGHT: if (cursor_ < text_.size()) ++cursor_; break;
        case VK_HOME: cursor_ = 0; break;
        case VK_END: cursor_ = text_.size(); break;
        case VK_DELETE:
            if (cursor_ < text_.size()) {
                text_.erase(cursor_, 1);
                invokeTextChanged();
            }
            break;
        default: break;
        }
        requestRedraw();
    } else if (ev.message == WM_CHAR) {
        wchar_t ch = static_cast<wchar_t>(ev.wParam);
        if (ch == 8) {
            if (cursor_ > 0 && !text_.empty()) {
                text_.erase(cursor_ - 1, 1);
                --cursor_;
            }
        } else if (ch >= 32) {
            std::string utf = toUtf8(ch);
            text_.insert(cursor_, utf);
            cursor_ += utf.size();
        }
        invokeTextChanged();
        requestRedraw();
    }
}

void Neu_Passwordbox::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    std::string original = text_;
    text_.assign(original.size(), '*');
    Neu_Textbox::draw(d, drawable, gc, theme);
    text_ = original;
}

void Neu_Multilinetextbox::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    Neu_Rect r = bounds();
    setVirtualSize(std::max(r.width, approximateTextWidth(text_) + 20), std::max(r.height, 24 + static_cast<int>(wrapTextToWidth(text_, r.width - 20).size()) * 18));
    drawTextInRect(d, drawable, gc, theme, text_, Neu_Rect{r.x + 8 - scrollX_, r.y + 6 - scrollY_, r.width - 20 + scrollX_, r.height - 18 + scrollY_}, Neu_TextLayoutOptions{wordWrap_, truncateText_, textAlign_, 18, 0});
    drawScrollbars(d, drawable, gc, theme);
}

void Neu_Multilinetextbox::handleXEvent(XEvent& ev)
{
    if (focused_ && ev.message == WM_CHAR && ev.wParam == 13) {
        text_.insert(cursor_, "\n");
        ++cursor_;
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
    autoScroll_ = autoScroll_ || static_cast<int>(items_.size()) * 22 > r.height;
    setVirtualSize(r.width, std::max(r.height, static_cast<int>(items_.size()) * 22 + 10));
    withClip(drawable, Neu_Rect{r.x + 2, r.y + 2, r.width - 14, r.height - 14}, [&]() {
        int y = r.y + 8 - scrollY_;
        for (size_t i = 0; i < items_.size() && y < r.y + r.height - 10; ++i, y += 22) {
            if (y + 22 < r.y) continue;
            if (static_cast<int>(i) == selected_ || static_cast<int>(i) == hoverIndex_) {
                RECT sr{r.x + 4, y, r.x + r.width - 14, y + 22};
                HBRUSH b = CreateSolidBrush(rgb(static_cast<int>(i) == selected_ ? theme.pressed : theme.hover));
                FillRect(drawable, &sr, b);
                DeleteObject(b);
            }
            drawTextInRect(d, drawable, gc, theme, items_[i], Neu_Rect{r.x + 10, y + 2, r.width - 24, 20}, Neu_TextLayoutOptions{false, true, Neu_TextAlign::Left, 18, 0});
        }
    });
    drawScrollbars(d, drawable, gc, theme);
}

void Neu_Listbox::handleXEvent(XEvent& ev)
{
    Neu_Control::handleXEvent(ev);
    Neu_Rect r = bounds();
    if (ev.message == WM_MOUSEMOVE) {
        int hover = contains(ev.x, ev.y) ? (ev.y - r.y + scrollY_) / 22 : -1;
        if (hover < 0 || hover >= static_cast<int>(items_.size())) hover = -1;
        if (hover != hoverIndex_) {
            hoverIndex_ = hover;
            requestRedraw();
        }
    }
    if (ev.message == WM_LBUTTONUP && contains(ev.x, ev.y)) {
        int idx = (ev.y - r.y + scrollY_) / 22;
        if (idx >= 0 && idx < static_cast<int>(items_.size())) {
            selected_ = idx;
            if (callbacks_.onSelectionChanged) callbacks_.onSelectionChanged(this, selected_, 0, items_[static_cast<size_t>(selected_)].c_str(), callbacks_.userData);
            requestRedraw();
        }
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
    if (!model_ || r >= model_->size() || c >= (*model_)[r].size()) return Neu_TypeInterpreter::interpret("");
    return Neu_TypeInterpreter::interpret((*model_)[r][c]);
}

void Neu_ListView::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    if (!model_) return;
    Neu_Rect r = bounds();
    size_t cols = 0;
    for (const auto& row : *model_) cols = std::max(cols, row.size());
    autoScroll_ = true;
    setVirtualSize(std::max(r.width, static_cast<int>(cols) * 150 + 20), std::max(r.height, static_cast<int>(model_->size()) * 22 + 16));
    withClip(drawable, Neu_Rect{r.x + 2, r.y + 2, r.width - 14, r.height - 14}, [&]() {
        int y = r.y + 8 - scrollY_;
        for (size_t row = 0; row < model_->size() && y < r.y + r.height - 8; ++row, y += 22) {
            if (y + 22 < r.y) continue;
            if (static_cast<int>(row) == selectedRow_ || static_cast<int>(row) == hoverRow_) {
                RECT rr{r.x + 4, y, r.x + r.width - 14, y + 22};
                HBRUSH b = CreateSolidBrush(rgb(static_cast<int>(row) == selectedRow_ ? theme.pressed : theme.hover));
                FillRect(drawable, &rr, b);
                DeleteObject(b);
            }
            int x = r.x + 8 - scrollX_;
            for (size_t col = 0; col < (*model_)[row].size(); ++col, x += 150) {
                if (x >= r.x + r.width - 8) break;
                if (x + 150 < r.x) continue;
                Rectangle(drawable, x, y, x + 146, y + 22);
                drawTextInRect(d, drawable, gc, theme, (*model_)[row][col], Neu_Rect{x + 4, y + 2, 140, 20}, Neu_TextLayoutOptions{false, true, Neu_TextAlign::Left, 18, 0});
            }
        }
    });
    drawScrollbars(d, drawable, gc, theme);
}

void Neu_ListView::handleXEvent(XEvent& ev)
{
    Neu_Control::handleXEvent(ev);
    Neu_Rect r = bounds();
    if (ev.message == WM_MOUSEMOVE && model_) {
        int row = contains(ev.x, ev.y) ? (ev.y - r.y + scrollY_) / 22 : -1;
        int col = contains(ev.x, ev.y) ? (ev.x - r.x + scrollX_ - 8) / 150 : -1;
        if (row < 0 || row >= static_cast<int>(model_->size())) { row = -1; col = -1; }
        else if (col < 0 || col >= static_cast<int>((*model_)[static_cast<size_t>(row)].size())) { col = -1; }
        if (row != hoverRow_ || col != hoverCol_) { hoverRow_ = row; hoverCol_ = col; requestRedraw(); }
    }
    if (ev.message == WM_LBUTTONUP && contains(ev.x, ev.y) && model_) {
        int row = (ev.y - r.y + scrollY_) / 22;
        int col = (ev.x - r.x + scrollX_ - 8) / 150;
        if (row >= 0 && row < static_cast<int>(model_->size()) && col >= 0 && col < static_cast<int>((*model_)[static_cast<size_t>(row)].size())) {
            selectedRow_ = row;
            selectedCol_ = col;
            if (callbacks_.onSelectionChanged) callbacks_.onSelectionChanged(this, row, col, (*model_)[static_cast<size_t>(row)][static_cast<size_t>(col)].c_str(), callbacks_.userData);
            requestRedraw();
        }
    }
}

void Neu_TreeView::expandAll() { collapsedPaths_.clear(); requestRedraw(); }
void Neu_TreeView::collapseAll() { if (model()) for (const auto& r : *model()) if (!r.empty()) collapsedPaths_.insert(r[0]); requestRedraw(); }
void Neu_TreeView::toggleNodePath(const std::string& path) { if (collapsedPaths_.count(path)) collapsedPaths_.erase(path); else collapsedPaths_.insert(path); requestRedraw(); }
bool Neu_TreeView::isPathCollapsed(const std::string& path) const { return collapsedPaths_.count(path) != 0; }
void Neu_TreeView::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme) { Neu_ListView::draw(d, drawable, gc, theme); }
void Neu_TreeView::handleXEvent(XEvent& ev) { Neu_ListView::handleXEvent(ev); }

void Neu_Placement::setParent(Neu_Window* parent)
{
    Neu_Control::setParent(parent);
    for (auto& c : children_) if (c) c->setParent(parent);
}

void Neu_Placement::add(std::shared_ptr<Neu_Control> child)
{
    if (child) { child->setParent(parent_); children_.push_back(child); }
}

void Neu_Placement::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    Neu_Rect r = bounds();
    withClip(drawable, Neu_Rect{r.x + 1, r.y + 1, r.width - 2, r.height - 2}, [&]() {
        for (auto& c : children_) if (c->visible()) c->draw(d, drawable, gc, theme);
    });
}

void Neu_Placement::handleXEvent(XEvent& ev)
{
    for (auto& c : children_) if (c->visible()) c->handleXEvent(ev);
    Neu_Control::handleXEvent(ev);
}

void Neu_PopWindowMenu::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Placement::draw(d, drawable, gc, theme);
}

void Neu_ScrollBar::setRange(int total, int page, int value)
{
    total_ = std::max(1, total); page_ = std::max(1, page); value_ = std::max(0, std::min(value, std::max(0, total_ - page_)));
}

void Neu_ScrollBar::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    Neu_Rect r = bounds();
    int track = vertical_ ? r.height : r.width;
    int thumb = std::max(18, track * page_ / std::max(1, total_));
    int maxValue = std::max(1, total_ - page_);
    int pos = (track - thumb) * value_ / maxValue;
    HBRUSH b = CreateSolidBrush(rgb(theme.accent));
    RECT th{};
    if (vertical_) {
        th = RECT{r.x + 2, r.y + pos, r.x + r.width - 2, r.y + pos + thumb};
    } else {
        th = RECT{r.x + pos, r.y + 2, r.x + pos + thumb, r.y + r.height - 2};
    }
    FillRect(drawable, &th, b);
    DeleteObject(b);
}

void Neu_ScrollBar::handleXEvent(XEvent& ev)
{
    if (ev.message == WM_LBUTTONDOWN && contains(ev.x, ev.y)) {
        Neu_Rect r = bounds();
        int coordinate = vertical_ ? ev.y - r.y : ev.x - r.x;
        int track = vertical_ ? r.height : r.width;
        int maxValue = std::max(1, total_ - page_);
        setRange(total_, page_, coordinate * maxValue / std::max(1, track));
    }
    Neu_Control::handleXEvent(ev);
}

void Neu_ScrollWindow::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    Neu_Rect r = bounds();
    withClip(drawable, Neu_Rect{r.x + 2, r.y + 2, r.width - 14, r.height - 14}, [&]() {
        for (auto& c : children()) {
            if (c->visible()) {
                auto old = c->layout();
                auto shifted = old;
                shifted.left -= scrollX_;
                shifted.top -= scrollY_;
                c->setLayout(shifted);
                c->draw(d, drawable, gc, theme);
                c->setLayout(old);
            }
        }
    });
    drawScrollbars(d, drawable, gc, theme);
}

void Neu_ScrollWindow::handleXEvent(XEvent& ev)
{
    if (handleScrollMouseEvent(ev)) return;
    XEvent translated = ev;
    translated.x += scrollX_;
    translated.y += scrollY_;
    for (auto& c : children()) if (c->visible()) c->handleXEvent(translated);
    Neu_Control::handleXEvent(ev);
}

void Neu_Label::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Rect r = bounds();
    int tx = r.x;
    int tw = r.width;
    if (icon().width() > 0) { drawIconBmp(d, drawable, gc, r.x, r.y + std::max(0, (r.height - 16) / 2), 16); tx += 22; tw -= 22; }
    if (!richTextFragments_.empty()) drawRichTextFragments(d, drawable, gc, theme, richTextFragments_, Neu_Rect{tx, r.y, tw, r.height}, Neu_TextLayoutOptions{wordWrap_, truncateText_, textAlign_, 18, 0});
    else drawTextInRect(d, drawable, gc, theme, text_, Neu_Rect{tx, r.y, tw, r.height}, Neu_TextLayoutOptions{wordWrap_, truncateText_, textAlign_, 18, 0});
}

Neu_MultilineLabel::Neu_MultilineLabel(Neu_Layout layout) : Neu_Label(layout)
{
    wordWrap_ = true;
    truncateText_ = false;
    autoScroll_ = true;
}

void Neu_MultilineLabel::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Rect r = bounds();
    setVirtualSize(r.width, std::max(r.height, static_cast<int>(wrapTextToWidth(text_, r.width - 8).size()) * 18 + 8));
    drawTextInRect(d, drawable, gc, theme, text_, Neu_Rect{r.x + 3 - scrollX_, r.y + 3 - scrollY_, r.width - 14 + scrollX_, r.height - 14 + scrollY_}, Neu_TextLayoutOptions{wordWrap_, truncateText_, textAlign_, 18, 0});
    drawScrollbars(d, drawable, gc, theme);
}

void Neu_RichTextCode::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    Neu_Rect r = bounds();
    int toolbarH = toolbarVisible_ ? 30 : 0;
    if (toolbarVisible_) drawTextInRect(d, drawable, gc, theme, "B I U S SS H1 H2 H3 H4 H5 H6 H7 Normal Mono Font Fg Bg Highlight Left Center Right Wrap", Neu_Rect{r.x + 8, r.y + 5, r.width - 16, 22}, Neu_TextLayoutOptions{false, true, Neu_TextAlign::Left, 18, 0});
    if (!richTextFragments_.empty()) drawRichTextFragments(d, drawable, gc, theme, richTextFragments_, Neu_Rect{r.x + 8 - scrollX_, r.y + toolbarH + 6 - scrollY_, r.width - 22 + scrollX_, r.height - toolbarH - 18 + scrollY_}, Neu_TextLayoutOptions{wordWrap_, truncateText_, textAlign_, 18, 0});
    else drawTextInRect(d, drawable, gc, theme, text_, Neu_Rect{r.x + 8 - scrollX_, r.y + toolbarH + 6 - scrollY_, r.width - 22 + scrollX_, r.height - toolbarH - 18 + scrollY_}, Neu_TextLayoutOptions{wordWrap_, truncateText_, Neu_TextAlign::Left, 18, 0});
    autoScroll_ = true;
    setVirtualSize(std::max(r.width, approximateTextWidth(text_) + 30), std::max(r.height, 80 + static_cast<int>(wrapTextToWidth(text_, r.width - 20).size()) * 18));
    drawScrollbars(d, drawable, gc, theme);
}

void Neu_RichTextCode::handleXEvent(XEvent& ev)
{
    if (!readOnly_) Neu_Multilinetextbox::handleXEvent(ev); else Neu_Control::handleXEvent(ev);
}

void Neu_ProgressSquare::setProgress(float progress) { progress_ = std::max(0.0f, std::min(1.0f, progress)); requestRedraw(); }

void Neu_ProgressSquare::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    Neu_Rect r = bounds();
    int side = std::max(8, std::min(r.width, r.height) - 18);
    int x = r.x + (r.width - side) / 2;
    int y = r.y + 8;
    Rectangle(drawable, x, y, x + side, y + side);
    HPEN pen = CreatePen(PS_SOLID, 5, rgb(progress_ > 0.82f ? Neu_Color{255, 225, 72, 255} : theme.accent));
    HGDIOBJ old = SelectObject(drawable, pen);
    int remaining = static_cast<int>(4 * side * progress_);
    auto segment = [&](int x1, int y1, int x2, int y2, int len) {
        if (remaining <= 0) return;
        int use = std::min(remaining, len);
        int ex = x1 + (x2 - x1) * use / std::max(1, len);
        int ey = y1 + (y2 - y1) * use / std::max(1, len);
        MoveToEx(drawable, x1, y1, nullptr);
        LineTo(drawable, ex, ey);
        remaining -= use;
    };
    int half = side / 2;
    segment(x + half, y, x + side, y, side - half);
    segment(x + side, y, x + side, y + side, side);
    segment(x + side, y + side, x, y + side, side);
    segment(x, y + side, x, y, side);
    segment(x, y, x + half, y, half);
    SelectObject(drawable, old);
    DeleteObject(pen);
    drawText(d, drawable, gc, theme, std::to_string(static_cast<int>(progress_ * 100.0f)) + "%", x + side / 2 - 14, y + side / 2 - 8);
}

void Neu_ImageView::draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme)
{
    Neu_Control::draw(d, drawable, gc, theme);
    Neu_Rect r = bounds();
    drawIconBmp(d, drawable, gc, r.x + 6, r.y + 6, std::min(r.width, r.height) - 12);
}

static size_t countHashes(const std::string& text)
{
    size_t c = 0;
    bool esc = false;
    for (char ch : text) { if (esc) { esc = false; continue; } if (ch == '\\') { esc = true; continue; } if (ch == '#') ++c; }
    return c;
}

static std::string unescapeHashText(const std::string& text)
{
    std::string out;
    bool esc = false;
    for (char ch : text) { if (esc) { out.push_back(ch); esc = false; } else if (ch == '\\') esc = true; else out.push_back(ch); }
    return out;
}

void Neu_ReadOnlyRichText::setIconList(const std::vector<std::string>& bmpIconPaths) { iconPaths_ = bmpIconPaths; }
size_t Neu_ReadOnlyRichText::iconIndexForText(const std::string& text) const { return iconPaths_.empty() ? 0 : std::min(countHashes(text), iconPaths_.size() - 1); }

void Neu_ReadOnlyRichText::addLabel(const std::string& text)
{
    std::string clean = unescapeHashText(text);
    if (!appendNextWithoutCrLf_) cursorX_ = 12;
    int width = std::min(std::max(60, static_cast<int>(clean.size()) * 8 + 28), std::max(60, layout().width - 36));
    auto l = std::make_shared<Neu_Label>(Neu_Layout{layout().left + cursorX_, layout().top + cursorY_, width, 28});
    l->setText(clean);
    if (!iconPaths_.empty()) l->setIconBmp(iconPaths_[iconIndexForText(text)]);
    add(l);
    if (appendNextWithoutCrLf_) cursorX_ += width + labelSpacing_; else cursorY_ += 28 + labelSpacing_;
    setContentSize(layout().width, cursorY_ + 40);
}

void Neu_ReadOnlyRichText::addLabel(const std::vector<Neu_RichTextFragment>& fragments)
{
    std::string text;
    for (const auto& f : fragments) text += f.text;
    if (!appendNextWithoutCrLf_) cursorX_ = 12;
    int width = std::min(std::max(80, static_cast<int>(text.size()) * 8 + 28), std::max(80, layout().width - 36));
    auto l = std::make_shared<Neu_Label>(Neu_Layout{layout().left + cursorX_, layout().top + cursorY_, width, 28});
    for (const auto& f : fragments) l->addTextFragment(f);
    if (!iconPaths_.empty()) l->setIconBmp(iconPaths_[iconIndexForText(text)]);
    add(l);
    if (appendNextWithoutCrLf_) cursorX_ += width + labelSpacing_; else cursorY_ += 28 + labelSpacing_;
    setContentSize(layout().width, cursorY_ + 40);
}

void Neu_ReadOnlyRichText::addMultilineLabel(const std::string& text)
{
    std::string clean = unescapeHashText(text);
    if (!appendNextWithoutCrLf_) cursorX_ = 12;
    int width = std::max(80, layout().width - 36 - cursorX_);
    int h = std::max(42, static_cast<int>(clean.size()) / std::max(1, width / 7) * (18 + labelLineSpacing_) + 42);
    auto l = std::make_shared<Neu_MultilineLabel>(Neu_Layout{layout().left + cursorX_, layout().top + cursorY_, width, h});
    l->setText(clean);
    if (!iconPaths_.empty()) l->setIconBmp(iconPaths_[iconIndexForText(text)]);
    add(l);
    if (appendNextWithoutCrLf_) cursorX_ += width + labelSpacing_; else cursorY_ += h + labelSpacing_;
    setContentSize(layout().width, cursorY_ + h + labelSpacing_);
}

Neu_Window::Neu_Window(Neu_Application& app, int width, int height, const std::string& title) : app_(app), display_(nullptr), width_(width), height_(height), title_(title) {}
Neu_Window::~Neu_Window() { close(); }

bool Neu_Window::create()
{
    DWORD style = WS_OVERLAPPEDWINDOW;
    RECT rc{0, 0, width_, height_};
    AdjustWindowRectEx(&rc, style, FALSE, WS_EX_APPWINDOW);
    HWND hwnd = CreateWindowExW(WS_EX_APPWINDOW, kNeuWindowClass, toWide(title_).c_str(), style, CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, g_instance ? g_instance : GetModuleHandle(nullptr), this);
    if (!hwnd) return false;
    window_ = hwnd;
    gc_ = GetDC(hwnd);
    app_.registerWindow(this);
    return true;
}

void Neu_Window::show() { ShowWindow(window_, SW_SHOW); UpdateWindow(window_); }

void Neu_Window::releaseBuffers()
{
    if (oldBitmap_ && memoryDc_) SelectObject(memoryDc_, oldBitmap_);
    if (backBitmap_) DeleteObject(backBitmap_);
    if (memoryDc_) DeleteDC(memoryDc_);
    oldBitmap_ = nullptr; backBitmap_ = nullptr; memoryDc_ = nullptr; bufferWidth_ = bufferHeight_ = 0;
}

void Neu_Window::close()
{
    if (!window_) return;
    releaseBuffers();
    app_.unregisterWindow(this);
    if (gc_) ReleaseDC(window_, gc_);
    HWND old = window_;
    window_ = nullptr;
    gc_ = nullptr;
    DestroyWindow(old);
}

void Neu_Window::ensureBuffers()
{
    if (!window_ || (bufferWidth_ == width_ && bufferHeight_ == height_ && memoryDc_ && backBitmap_)) return;
    releaseBuffers();
    HDC dc = GetDC(window_);
    memoryDc_ = CreateCompatibleDC(dc);
    backBitmap_ = CreateCompatibleBitmap(dc, width_, height_);
    oldBitmap_ = static_cast<HBITMAP>(SelectObject(memoryDc_, backBitmap_));
    ReleaseDC(window_, dc);
    bufferWidth_ = width_; bufferHeight_ = height_;
}

void Neu_Window::drawScene(Drawable target)
{
    RECT rc{0, 0, width_, height_};
    HBRUSH b = CreateSolidBrush(rgb(theme_.background));
    FillRect(target, &rc, b);
    DeleteObject(b);
    for (auto& c : controls_) if (c->visible()) c->drawShadow(nullptr, target, target, theme_);
    for (auto& c : controls_) if (c->visible()) c->draw(nullptr, target, target, theme_);
    for (auto& c : controls_) if (c->visible()) c->drawHintPopup(nullptr, target, target, theme_);
}

void Neu_Window::redraw()
{
    if (!window_) return;
    HDC dc = GetDC(window_);
    if (multiStageDoubleBuffering_ && g_options.multiStageDoubleBuffering) {
        ensureBuffers();
        drawScene(memoryDc_);
        BitBlt(dc, 0, 0, width_, height_, memoryDc_, 0, 0, SRCCOPY);
    } else {
        drawScene(dc);
    }
    ReleaseDC(window_, dc);
}

void Neu_Window::setMultiStageDoubleBuffering(bool enabled) { multiStageDoubleBuffering_ = enabled; redraw(); }
void Neu_Window::add(std::shared_ptr<Neu_Control> control) { if (control) { control->setParent(this); controls_.push_back(control); } }

void Neu_Window::handleXEvent(XEvent& ev)
{
    if (ev.message == WM_CLOSE) {
        if (onClose_) onClose_(this, closeUserData_);
        close();
        return;
    }
    if (ev.message == WM_SIZE) {
        width_ = LOWORD(ev.lParam);
        height_ = HIWORD(ev.lParam);
        releaseBuffers();
        redraw();
        return;
    }
    if (ev.message == WM_MOUSEMOVE) {
        TRACKMOUSEEVENT tme{};
        tme.cbSize = sizeof(tme); tme.dwFlags = TME_LEAVE; tme.hwndTrack = window_;
        TrackMouseEvent(&tme);
    }
    for (auto& c : controls_) if (c->visible() && c->enabled()) c->handleXEvent(ev);
}

} // namespace neutrino
#endif
