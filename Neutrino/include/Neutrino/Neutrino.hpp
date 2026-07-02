#pragma once
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
using Display = void;
using Drawable = HDC;
using GC = HDC;
using Window = HWND;
using Atom = UINT;
using KeySym = unsigned int;
struct XEvent { UINT message{0}; WPARAM wParam{0}; LPARAM lParam{0}; int x{0}; int y{0}; };
#else
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif
#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include <memory>
#include <map>
#include <set>
#include <variant>
#include <algorithm>
#include <sstream>
#include <cmath>
#include <cstring>
#include <cstdlib>

namespace neutrino {

struct Neu_Point { int x{0}; int y{0}; };
struct Neu_Size { int width{0}; int height{0}; };
struct Neu_Rect { int x{0}; int y{0}; int width{0}; int height{0}; };

struct Neu_Layout {
    int left{0};
    int top{0};
    int width{100};
    int height{30};
    float scale{1.0f};
    int maxWidth{0};
    int maxHeight{0};
    Neu_Rect resolved() const {
        int w = static_cast<int>(std::round(width * scale));
        int h = static_cast<int>(std::round(height * scale));
        if (maxWidth > 0) w = std::min(w, maxWidth);
        if (maxHeight > 0) h = std::min(h, maxHeight);
        return {left, top, w, h};
    }
};

struct Neu_Color { uint8_t r{0}, g{0}, b{0}, a{255}; };

enum class Neu_TextAlign {
    Left,
    Center,
    Right
};

enum Neu_TextStyleFlags : uint32_t {
    Neu_TextStyle_Normal = 0,
    Neu_TextStyle_Bold = 1u << 0,
    Neu_TextStyle_Italic = 1u << 1,
    Neu_TextStyle_Underline = 1u << 2,
    Neu_TextStyle_Strikethrough = 1u << 3,
    Neu_TextStyle_DoubleStrikethrough = 1u << 4,
    Neu_TextStyle_Monospaced = 1u << 5
};

struct Neu_RichTextFragment {
    std::string text;
    uint32_t style{Neu_TextStyle_Normal};
    int headingLevel{0};
    std::string fontName;
    Neu_Color fontColor{20, 28, 38, 255};
    bool hasFontColor{false};
    Neu_Color backgroundColor{255, 255, 255, 255};
    bool hasBackgroundColor{false};
    Neu_Color highlightColor{255, 245, 150, 180};
    bool hasHighlight{false};
};

inline Neu_RichTextFragment Neu_MakeRichTextFragment(const std::string& text,
                                                     uint32_t style = Neu_TextStyle_Normal,
                                                     int headingLevel = 0,
                                                     const std::string& fontName = std::string(),
                                                     const Neu_Color* fontColor = nullptr,
                                                     const Neu_Color* backgroundColor = nullptr,
                                                     const Neu_Color* highlightColor = nullptr)
{
    Neu_RichTextFragment fragment;
    fragment.text = text;
    fragment.style = style;
    fragment.headingLevel = std::max(0, std::min(7, headingLevel));
    fragment.fontName = fontName;
    if (fontColor) {
        fragment.fontColor = *fontColor;
        fragment.hasFontColor = true;
    }
    if (backgroundColor) {
        fragment.backgroundColor = *backgroundColor;
        fragment.hasBackgroundColor = true;
    }
    if (highlightColor) {
        fragment.highlightColor = *highlightColor;
        fragment.hasHighlight = true;
    }
    return fragment;
}

struct Neu_TextLayoutOptions {
    bool wordWrap{false};
    bool truncate{true};
    Neu_TextAlign align{Neu_TextAlign::Left};
    int lineHeight{18};
    int padding{0};
};
enum class Neu_GraphicsBackend {
    X11Basic,              // plain Xlib primitives
    SoftwareAntialias,     // Cairo-free CPU supersampling pushed through XImage
    XRenderAntialias       // use XRender when present, fallback to software AA
};

struct Neu_SmoothGraphicsOptions {
    bool enabled{true};
    Neu_GraphicsBackend backend{Neu_GraphicsBackend::XRenderAntialias};
    int supersample{4};
    bool drawShadows{true};
    bool drawHints{true};
    bool repaintOnMouseMove{true};
    bool cacheRoundedRects{true};
    bool vmFriendly{false};
    bool multiStageDoubleBuffering{true};
    int bufferStages{3};
};

struct Neu_Theme {
    Neu_Color background{245,248,252,255};
    Neu_Color glass{238,246,255,210};
    Neu_Color border{150,175,205,255};
    Neu_Color text{20,28,38,255};
    Neu_Color accent{70,135,220,255};
    Neu_Color hover{225,238,255,255};
    Neu_Color pressed{190,215,250,255};
    Neu_Color shadow{36, 52, 70, 85};
    Neu_Color hintBackground{255, 255, 232, 245};
    Neu_Color hintBorder{118, 132, 72, 255};
    int radius{12};
    int shadowSize{7};
    int shadowOffsetX{3};
    int shadowOffsetY{4};
    std::string fontName{"DejaVu Sans:size=10:antialias=true:hinting=true:hintstyle=hintfull:rgba=rgb:lcdfilter=lcddefault"};
    static Neu_Theme Light();
    static Neu_Theme Dark();
    static Neu_Theme BlueGlass();
};

class Neu_Control;
class Neu_Window;

using Neu_EventCallback = void (*)(Neu_Control* sender, void* user_data);
using Neu_TextChangedCallback = void (*)(Neu_Control* sender, const char* text, void* user_data);
using Neu_SelectionChangedCallback = void (*)(Neu_Control* sender, int row, int column, const char* value, void* user_data);
using Neu_WindowCallback = void (*)(Neu_Window* sender, void* user_data);
using Neu_KeyCallback = void (*)(Neu_Control* sender, KeySym key, unsigned int state, void* user_data);
using Neu_ScrollCallback = void (*)(Neu_Control* sender, int scroll_x, int scroll_y, void* user_data);

struct Neu_Callbacks {
    Neu_EventCallback onClick{nullptr};
    Neu_EventCallback onFocus{nullptr};
    Neu_EventCallback onBlur{nullptr};
    Neu_TextChangedCallback onTextChanged{nullptr};
    Neu_SelectionChangedCallback onSelectionChanged{nullptr};
    Neu_KeyCallback onKeyDown{nullptr};
    Neu_ScrollCallback onScroll{nullptr};
    void* userData{nullptr};
};

class Neu_IconBmp {
public:
    bool load(const std::string& path);
    int width() const { return w_; }
    int height() const { return h_; }
    const std::vector<uint32_t>& pixels() const { return pixels_; }
private:
    int w_{0};
    int h_{0};
    std::vector<uint32_t> pixels_;
};

enum class Neu_CellType {
    String, UtfString, Int64, UInt64, Float, Double, Binary8, Binary16, Binary32,
    Binary64, Binary128, Hex, Boolean, TriState, Enum, ImageBmp, Checkbox
};

struct Neu_TypedValue {
    Neu_CellType type{Neu_CellType::String};
    std::string original;
    std::variant<std::string, int64_t, uint64_t, float, double, bool> value{std::string{}};
};

class Neu_TypeInterpreter {
public:
    static Neu_TypedValue interpret(const std::string& text);
};

class Neu_Application {
public:
    Neu_Application();
    ~Neu_Application();
    bool open();
    Display* display() const { return display_; }
    int screen() const { return screen_; }
    void run();
    void quit() { running_ = false; }
    void registerWindow(Neu_Window* win);
    void unregisterWindow(Neu_Window* win);
    static Neu_Application* current();
    bool xrenderAvailable() const { return xrenderAvailable_; }
private:
    bool detectXRender();
#ifdef _WIN32
    Display* display_{nullptr};
    int screen_{0};
    bool xrenderAvailable_{false};
#else
    Display* display_{nullptr};
    int screen_{0};
    bool xrenderAvailable_{false};
#endif
    bool running_{false};
    std::vector<Neu_Window*> windows_;
    static Neu_Application* current_;
};

class Neu_Control {
public:
    explicit Neu_Control(Neu_Layout layout = {});
    virtual ~Neu_Control() = default;
    virtual void draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme);
    virtual void handleXEvent(XEvent& ev);
    virtual const char* className() const { return "Neu_Control"; }
    void setLayout(const Neu_Layout& layout) { layout_ = layout; }
    Neu_Layout layout() const { return layout_; }
    Neu_Rect bounds() const { return layout_.resolved(); }
    bool contains(int x, int y) const;
    void setText(const std::string& text);
    const std::string& text() const { return text_; }
    void setWordWrap(bool enabled) { wordWrap_ = enabled; requestRedraw(); }
    bool wordWrap() const { return wordWrap_; }
    void setTruncateText(bool enabled) { truncateText_ = enabled; requestRedraw(); }
    bool truncateText() const { return truncateText_; }
    void setTextAlign(Neu_TextAlign align) { textAlign_ = align; requestRedraw(); }
    Neu_TextAlign textAlign() const { return textAlign_; }
    void setFontName(const std::string& fontName) { fontName_ = fontName; requestRedraw(); }
    const std::string& fontName() const { return fontName_; }
    void setFontColor(const Neu_Color& color) { fontColor_ = color; hasFontColor_ = true; requestRedraw(); }
    void clearFontColor() { hasFontColor_ = false; requestRedraw(); }
    void setBackgroundColor(const Neu_Color& color) { backgroundColor_ = color; hasBackgroundColor_ = true; requestRedraw(); }
    void clearBackgroundColor() { hasBackgroundColor_ = false; requestRedraw(); }
    void setHighlightColor(const Neu_Color& color) { highlightColor_ = color; hasHighlight_ = true; requestRedraw(); }
    void clearHighlightColor() { hasHighlight_ = false; requestRedraw(); }
    void clearRichTextFragments() { richTextFragments_.clear(); requestRedraw(); }
    void setRichTextFragments(const std::vector<Neu_RichTextFragment>& fragments) { richTextFragments_ = fragments; requestRedraw(); }
    void addTextFragment(const Neu_RichTextFragment& fragment) { richTextFragments_.push_back(fragment); requestRedraw(); }
    void addRichText(const std::string& text, uint32_t style = Neu_TextStyle_Normal, int headingLevel = 0, const std::string& fontName = std::string())
    {
        richTextFragments_.push_back(Neu_MakeRichTextFragment(text, style, headingLevel, fontName));
        requestRedraw();
    }
    const std::vector<Neu_RichTextFragment>& richTextFragments() const { return richTextFragments_; }
    void setHintText(const std::string& text) { hintText_ = text; }
    const std::string& hintText() const { return hintText_; }
    void setHintExpanded(bool expanded) { hintExpanded_ = expanded; requestRedraw(); }
    bool hintExpanded() const { return hintExpanded_; }
    void setAutoScroll(bool enabled) { autoScroll_ = enabled; requestRedraw(); }
    bool autoScroll() const { return autoScroll_; }
    void setScrollOffset(int x, int y);
    int scrollX() const { return scrollX_; }
    int scrollY() const { return scrollY_; }
    void setVirtualSize(int width, int height);
    Neu_Size virtualSize() const { return virtualSize_; }
    bool hovered() const { return hover_; }
    void setIconBmp(const std::string& path) { icon_.load(path); iconPath_ = path; }
    const Neu_IconBmp& icon() const { return icon_; }
    void setCallbacks(const Neu_Callbacks& callbacks) { callbacks_ = callbacks; }
    Neu_Callbacks& callbacks() { return callbacks_; }
    virtual void setParent(Neu_Window* parent) { parent_ = parent; }
    Neu_Window* parent() const { return parent_; }
    void setVisible(bool visible) { visible_ = visible; }
    bool visible() const { return visible_; }
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool enabled() const { return enabled_; }
    void requestRedraw();
    virtual void drawShadow(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme);
    virtual void drawHintPopup(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme);
    virtual void drawScrollbars(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme);
protected:
    void drawText(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme, const std::string& s, int x, int y);
    int approximateTextWidth(const std::string& text, uint32_t style = Neu_TextStyle_Normal, int headingLevel = 0) const;
    std::vector<std::string> wrapTextToWidth(const std::string& text, int maxWidth) const;
    std::string truncateTextToWidth(const std::string& text, int maxWidth) const;
    void drawTextInRect(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme, const std::string& text, const Neu_Rect& rect, const Neu_TextLayoutOptions& options);
    void drawRichTextFragments(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme, const std::vector<Neu_RichTextFragment>& fragments, const Neu_Rect& rect, const Neu_TextLayoutOptions& options);
    bool handleScrollMouseEvent(XEvent& event);
    void drawIconBmp(Display* d, Drawable drawable, GC gc, int x, int y, int maxSize);
    void invokeClick();
    void invokeTextChanged();
    Neu_Layout layout_;
    std::string text_;
    std::string hintText_;
    bool hintExpanded_{false};
    Neu_IconBmp icon_;
    std::string iconPath_;
    Neu_Callbacks callbacks_;
    Neu_Window* parent_{nullptr};
    bool visible_{true};
    bool enabled_{true};
    bool hover_{false};
    bool pressed_{false};
    bool focused_{false};
    bool autoScroll_{false};
    bool wordWrap_{false};
    bool truncateText_{true};
    Neu_TextAlign textAlign_{Neu_TextAlign::Left};
    std::string fontName_;
    Neu_Color fontColor_{20, 28, 38, 255};
    bool hasFontColor_{false};
    Neu_Color backgroundColor_{255, 255, 255, 255};
    bool hasBackgroundColor_{false};
    Neu_Color highlightColor_{255, 245, 150, 180};
    bool hasHighlight_{false};
    std::vector<Neu_RichTextFragment> richTextFragments_;
    bool scrollDrag_{false};
    bool scrollDragVertical_{true};
    int scrollDragAnchor_{0};
    int scrollDragStartValue_{0};
    int scrollX_{0};
    int scrollY_{0};
    Neu_Size virtualSize_{0, 0};
};

class Neu_Button : public Neu_Control {
public:
    using Neu_Control::Neu_Control;
    const char* className() const override { return "Neu_Button"; }
    void draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme) override;
    void handleXEvent(XEvent& ev) override;
};
using Neu_FlatButton = Neu_Button;
using Neu_MenuItem = Neu_Button;

class Neu_Textbox : public Neu_Control {
public:
    using Neu_Control::Neu_Control;
    const char* className() const override { return "Neu_Textbox"; }
    void draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme) override;
    void handleXEvent(XEvent& ev) override;
protected:
    size_t cursor_{0};
};

class Neu_Passwordbox : public Neu_Textbox {
public:
    using Neu_Textbox::Neu_Textbox;
    const char* className() const override { return "Neu_Passwordbox"; }
    void draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme) override;
};

class Neu_Multilinetextbox : public Neu_Textbox {
public:
    using Neu_Textbox::Neu_Textbox;
    const char* className() const override { return "Neu_Multilinetextbox"; }
    void draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme) override;
    void handleXEvent(XEvent& ev) override;
};

class Neu_Listbox : public Neu_Control {
public:
    using Neu_Control::Neu_Control;
    const char* className() const override { return "Neu_Listbox"; }
    void setItems(const std::vector<std::string>& items) { items_ = items; }
    const std::vector<std::string>& items() const { return items_; }
    int selectedIndex() const { return selected_; }
    void draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme) override;
    void handleXEvent(XEvent& ev) override;
protected:
    std::vector<std::string> items_;
    int selected_{-1};
    int hoverIndex_{-1};
};

class Neu_ComboBox : public Neu_Listbox {
public:
    using Neu_Listbox::Neu_Listbox;
    const char* className() const override { return "Neu_ComboBox"; }
    void draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme) override;
    void handleXEvent(XEvent& ev) override;
private:
    bool open_{false};
};

using Neu_StringTable = std::vector<std::vector<std::string>>;

class Neu_ListView : public Neu_Control {
public:
    using Neu_Control::Neu_Control;
    const char* className() const override { return "Neu_ListView"; }
    void bind(Neu_StringTable* model) { model_ = model; }
    Neu_StringTable* model() const { return model_; }
    Neu_TypedValue cellValue(size_t r, size_t c) const;
    void draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme) override;
    void handleXEvent(XEvent& ev) override;
private:
    Neu_StringTable* model_{nullptr};
    int selectedRow_{-1};
    int selectedCol_{-1};
    int hoverRow_{-1};
    int hoverCol_{-1};
};

class Neu_TreeView : public Neu_ListView {
public:
    using Neu_ListView::Neu_ListView;
    const char* className() const override { return "Neu_TreeView"; }
    void expandAll();
    void collapseAll();
    void toggleNodePath(const std::string& path);
    bool isPathCollapsed(const std::string& path) const;
    void draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme) override;
    void handleXEvent(XEvent& ev) override;

private:
    std::set<std::string> collapsedPaths_;
    int selectedVisibleRow_{-1};
    int hoverVisibleRow_{-1};
};

class Neu_Placement : public Neu_Control {
public:
    using Neu_Control::Neu_Control;
    const char* className() const override { return "Neu_Placement"; }
    void setParent(Neu_Window* parent) override;
    void add(std::shared_ptr<Neu_Control> child);
    const std::vector<std::shared_ptr<Neu_Control>>& children() const { return children_; }
    void draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme) override;
    void handleXEvent(XEvent& ev) override;
private:
    std::vector<std::shared_ptr<Neu_Control>> children_;
};

class Neu_PopWindowMenu : public Neu_Placement {
public:
    using Neu_Placement::Neu_Placement;
    const char* className() const override { return "Neu_PopWindowMenu"; }
    void setCategories(const std::vector<std::string>& categories) { categories_ = categories; }
    void setItems(const std::string& category, const std::vector<std::string>& items) { items_[category] = items; }
    void draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme) override;
private:
    std::vector<std::string> categories_;
    std::map<std::string, std::vector<std::string>> items_;
    int selectedCategory_{0};
};


class Neu_ScrollBar : public Neu_Control {
public:
    using Neu_Control::Neu_Control;
    const char* className() const override { return "Neu_ScrollBar"; }
    void setVertical(bool vertical) { vertical_ = vertical; }
    void setRange(int total, int page, int value);
    int value() const { return value_; }
    void draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme) override;
    void handleXEvent(XEvent& ev) override;
private:
    bool vertical_{true};
    int total_{100};
    int page_{10};
    int value_{0};
};

class Neu_ScrollWindow : public Neu_Placement {
public:
    using Neu_Placement::Neu_Placement;
    const char* className() const override { return "Neu_ScrollWindow"; }
    void setContentSize(int width, int height) { setVirtualSize(width, height); setAutoScroll(true); }
    void draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme) override;
    void handleXEvent(XEvent& ev) override;
};

class Neu_Label : public Neu_Control {
public:
    using Neu_Control::Neu_Control;
    const char* className() const override { return "Neu_Label"; }
    void draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme) override;
};

class Neu_MultilineLabel : public Neu_Label {
public:
    explicit Neu_MultilineLabel(Neu_Layout layout = {});
    const char* className() const override { return "Neu_MultilineLabel"; }
    void draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme) override;
};

class Neu_RichTextCode : public Neu_Multilinetextbox {
public:
    using Neu_Multilinetextbox::Neu_Multilinetextbox;
    const char* className() const override { return "Neu_RichTextCode"; }
    void setReadOnly(bool readOnly) { readOnly_ = readOnly; }
    bool readOnly() const { return readOnly_; }
    void setLanguageName(const std::string& language) { languageName_ = language; }
    void setToolbarVisible(bool visible) { toolbarVisible_ = visible; requestRedraw(); }
    bool toolbarVisible() const { return toolbarVisible_; }
    void draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme) override;
    void handleXEvent(XEvent& ev) override;
private:
    bool readOnly_{false};
    bool toolbarVisible_{true};
    std::string languageName_{"C++17"};
};

class Neu_ProgressSquare : public Neu_Control {
public:
    using Neu_Control::Neu_Control;
    const char* className() const override { return "Neu_ProgressSquare"; }
    void setProgress(float progress);
    float progress() const { return progress_; }
    void draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme) override;
private:
    float progress_{0.0f};
};

class Neu_ImageView : public Neu_Control {
public:
    using Neu_Control::Neu_Control;
    const char* className() const override { return "Neu_ImageView"; }
    bool loadBmp(const std::string& path) { setIconBmp(path); return icon().width() > 0; }
    void draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme) override;
};

class Neu_ReadOnlyRichText : public Neu_ScrollWindow {
public:
    using Neu_ScrollWindow::Neu_ScrollWindow;
    const char* className() const override { return "Neu_ReadOnlyRichText"; }
    void setIconList(const std::vector<std::string>& bmpIconPaths);
    void setLabelSpacing(int pixels) { labelSpacing_ = std::max(0, pixels); }
    void setLabelLineSpacing(int pixels) { labelLineSpacing_ = std::max(0, pixels); }
    void no_crlf() { appendNextWithoutCrLf_ = true; }
    void crlf() { appendNextWithoutCrLf_ = false; cursorX_ = 12; cursorY_ += labelSpacing_; }
    void addLabel(const std::string& text);
    void addLabel(const std::vector<Neu_RichTextFragment>& fragments);
    void addMultilineLabel(const std::string& text);
private:
    std::vector<std::string> iconPaths_;
    int labelSpacing_{8};
    int labelLineSpacing_{4};
    int cursorX_{12};
    int cursorY_{12};
    bool appendNextWithoutCrLf_{false};
    size_t iconIndexForText(const std::string& text) const;
};

class Neu_Window {
public:
    Neu_Window(Neu_Application& app, int width, int height, const std::string& title);
    ~Neu_Window();
    bool create();
    void show();
    void close();
    void redraw();
    void setMultiStageDoubleBuffering(bool enabled);
    bool multiStageDoubleBuffering() const { return multiStageDoubleBuffering_; }
    void add(std::shared_ptr<Neu_Control> control);
    void handleXEvent(XEvent& ev);
    Window xid() const { return window_; }
    Neu_Theme& theme() { return theme_; }
    void setTheme(const Neu_Theme& t) { theme_ = t; }
    void setOnClose(Neu_WindowCallback cb, void* user_data) { onClose_ = cb; closeUserData_ = user_data; }
    int width() const { return width_; }
    int height() const { return height_; }
private:
    Neu_Application& app_;
    Display* display_{nullptr};
    Window window_{};
    GC gc_{};
    Atom wmDelete_{};
    int width_{0};
    int height_{0};
    std::string title_;
    Neu_Theme theme_;
    std::vector<std::shared_ptr<Neu_Control>> controls_;
    Neu_WindowCallback onClose_{nullptr};
    void* closeUserData_{nullptr};
    bool multiStageDoubleBuffering_{true};
#ifdef _WIN32
    HDC memoryDc_{nullptr};
    HBITMAP backBitmap_{nullptr};
    HBITMAP oldBitmap_{nullptr};
#else
    Pixmap stageBackground_{0};
    Pixmap stageCompose_{0};
    Pixmap stageFinal_{0};
#endif
    int bufferWidth_{0};
    int bufferHeight_{0};
    void ensureBuffers();
    void releaseBuffers();
    void drawScene(Drawable target);
};

unsigned long Neu_Pixel(Display* d, const Neu_Color& color);
Neu_Color Neu_PixelToColor(Display* d, unsigned long pixel);
void Neu_SetSmoothGraphicsOptions(const Neu_SmoothGraphicsOptions& options);
Neu_SmoothGraphicsOptions Neu_GetSmoothGraphicsOptions();
void Neu_EnableAntialiasing(bool enabled);
void Neu_UseVirtualMachineFriendlyDefaults(bool enabled);
void Neu_EnableMultiStageDoubleBuffering(bool enabled);
void Neu_DrawRoundedRect(Display* d, Drawable drawable, GC gc, int x, int y, int w, int h, int radius, bool fill);
void Neu_DrawSmoothRoundedRect(Display* d, Drawable drawable, GC gc, const Neu_Color& color, const Neu_Color& background, int x, int y, int w, int h, int radius, bool fill, int supersample = 4);
void Neu_DrawSmoothDropShadow(Display* d, Drawable drawable, GC gc, const Neu_Color& shadow, const Neu_Color& background, int x, int y, int w, int h, int radius, int blur, int offsetX, int offsetY);

} // namespace neutrino
