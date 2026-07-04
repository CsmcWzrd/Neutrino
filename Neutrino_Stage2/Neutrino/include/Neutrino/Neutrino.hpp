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
#include <chrono>

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

enum class Neu_TextAlignment { Left, Center, Right };

enum class Neu_CornerStyle {
    RoundedCorner,
    EdgeCorner
};

enum class Neu_AntiAliasMode {
    DAA,   // default device/font/XRender antialiasing
    MSAA,  // multi-sample shape antialiasing
    SSAA   // super-sample shape antialiasing
};

enum class Neu_FontFamily {
    Sans,
    Serif,
    SansSerif,
    Monospace
};

enum class Neu_Backend {
    Auto,
    Wayland,
    X11,
    Win32
};

struct Neu_TextEditSnapshot {
    std::string text;
    size_t cursor{0};
    size_t selectionStart{0};
    size_t selectionEnd{0};
    int scrollX{0};
    int scrollY{0};
};

inline std::string Neu_FontFamilyName(Neu_FontFamily family)
{
    switch (family) {
    case Neu_FontFamily::Serif: return "Serif";
    case Neu_FontFamily::SansSerif: return "SansSerif";
    case Neu_FontFamily::Monospace: return "Monospace";
    case Neu_FontFamily::Sans:
    default: return "Sans";
    }
}

struct Neu_TextOffset {
    int top{2};
    int right{4};
    int bottom{2};
    int left{6};
};

struct Neu_TextFragment {
    std::string text;
    bool bold{false};
    bool italic{false};
    bool underline{false};
    bool strikethrough{false};
    bool doubleStrikethrough{false};
    bool monospace{false};
    int headingLevel{0};
    std::string fontName;
    Neu_Color fontColor{20,28,38,255};
    Neu_Color backgroundColor{255,255,255,0};
    Neu_Color highlightColor{255,240,120,0};
    bool useFontColor{false};
    bool useBackgroundColor{false};
    bool useHighlightColor{false};
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
    bool repaintOnMouseMove{false};
    bool cacheRoundedRects{true};
    bool vmFriendly{false};
    bool multiStageDoubleBuffering{true};
    int bufferStages{3};
};

struct Neu_Theme {
    // Material-dark is the default theme used by new windows.
    Neu_Color background{18,18,18,255};
    Neu_Color glass{30,30,34,235};
    Neu_Color border{62,66,74,255};
    Neu_Color text{232,234,237,255};
    Neu_Color accent{144,202,249,255};
    Neu_Color hover{42,44,50,255};
    Neu_Color pressed{56,60,68,255};
    Neu_Color highlight{64,76,92,255};
    Neu_Color focus{42,112,178,255};
    Neu_Color controlGradientTop{58,62,70,255};
    Neu_Color controlGradientBottom{22,24,30,255};
    Neu_Color shadow{0, 0, 0, 135};
    Neu_Color hintBackground{34, 34, 38, 250};
    Neu_Color hintBorder{144, 202, 249, 255};
    int radius{10};
    int edgeSize{8};
    bool gradientControls{true};
    Neu_CornerStyle topLeftCorner{Neu_CornerStyle::EdgeCorner};
    Neu_CornerStyle topRightCorner{Neu_CornerStyle::RoundedCorner};
    Neu_CornerStyle bottomLeftCorner{Neu_CornerStyle::RoundedCorner};
    Neu_CornerStyle bottomRightCorner{Neu_CornerStyle::EdgeCorner};
    Neu_AntiAliasMode antiAliasMode{Neu_AntiAliasMode::DAA};
    int antiAliasSamples{3};
    int shadowSize{7};
    int shadowOffsetX{3};
    int shadowOffsetY{4};
    std::string fontName{"DejaVu Sans:size=10:antialias=true:hinting=true:hintstyle=hintfull:rgba=rgb:lcdfilter=lcddefault"};
    Neu_FontFamily fontFamily{Neu_FontFamily::Sans};
    void setFontFamily(Neu_FontFamily family) { fontFamily = family; fontName = Neu_FontFamilyName(family); }
    void setFontFamily(Neu_FontFamily family, const std::string& concreteFontName) { fontFamily = family; fontName = concreteFontName.empty() ? Neu_FontFamilyName(family) : concreteFontName; }
    void setAllCorners(Neu_CornerStyle style) { topLeftCorner = style; topRightCorner = style; bottomLeftCorner = style; bottomRightCorner = style; }
    void setCornerStyles(Neu_CornerStyle topLeft, Neu_CornerStyle topRight, Neu_CornerStyle bottomLeft, Neu_CornerStyle bottomRight) { topLeftCorner = topLeft; topRightCorner = topRight; bottomLeftCorner = bottomLeft; bottomRightCorner = bottomRight; }
    void setRoundedCorners() { setAllCorners(Neu_CornerStyle::RoundedCorner); }
    void setDefaultEdgeCorners() { setCornerStyles(Neu_CornerStyle::EdgeCorner, Neu_CornerStyle::RoundedCorner, Neu_CornerStyle::RoundedCorner, Neu_CornerStyle::EdgeCorner); }
    static Neu_Theme Light();
    static Neu_Theme Dark();
    static Neu_Theme BlueGlass();
    static Neu_Theme Win95();
    static Neu_Theme WinXP();
    static Neu_Theme Win10();
    static Neu_Theme Win11();
    static Neu_Theme ClassicMotif();
    static Neu_Theme SolarizedLight();
    static Neu_Theme SolarizedDark();
    static Neu_Theme Nord();
    static Neu_Theme Dracula();
    static Neu_Theme GruvboxLight();
    static Neu_Theme GruvboxDark();
    static Neu_Theme HighContrastLight();
    static Neu_Theme HighContrastDark();
    static Neu_Theme UbuntuAubergine();
    static Neu_Theme KDEBreeze();
    static Neu_Theme MacAqua();
    static Neu_Theme MaterialLight();
    static Neu_Theme MaterialDark();
    static Neu_Theme Ocean();
    static Neu_Theme Forest();
    static Neu_Theme Rose();
    static Neu_Theme Amber();
    static Neu_Theme Slate();
    static Neu_Theme Candy();
    static Neu_Theme TerminalGreen();
    static Neu_Theme CorporateBlue();
    static std::vector<std::string> BuiltInThemeNames();
    static Neu_Theme BuiltInThemeByName(const std::string& name);
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
    void quit();
    void registerWindow(Neu_Window* win);
    void unregisterWindow(Neu_Window* win);
    size_t windowCount() const { return windows_.size(); }
    bool hasWindows() const { return !windows_.empty(); }
    static Neu_Application* current();
    bool xrenderAvailable() const { return xrenderAvailable_; }
    Neu_Backend backend() const { return backend_; }
    bool usingWayland() const { return backend_ == Neu_Backend::Wayland; }
    const std::string& backendName() const { return backendName_; }
private:
    bool detectXRender();
    bool detectWayland();
#ifdef _WIN32
    Display* display_{nullptr};
    int screen_{0};
    bool xrenderAvailable_{false};
#else
    Display* display_{nullptr};
    int screen_{0};
    bool xrenderAvailable_{false};
    void* xrenderLibrary_{nullptr};
#endif
    Neu_Backend backend_{Neu_Backend::Auto};
    std::string backendName_{"auto"};
    bool waylandAvailable_{false};
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
    void setHintText(const std::string& text) { hintText_ = text; }
    const std::string& hintText() const { return hintText_; }
    void setHintExpanded(bool expanded) { hintExpanded_ = expanded; requestRedraw(); }
    bool hintExpanded() const { return hintExpanded_; }
    void setWordWrap(bool enabled) { wordWrap_ = enabled; requestRedraw(); }
    bool wordWrap() const { return wordWrap_; }
    void setTextTruncation(bool enabled) { truncateText_ = enabled; requestRedraw(); }
    bool textTruncation() const { return truncateText_; }
    void setBorderVisible(bool enabled) { borderVisible_ = enabled; requestRedraw(); }
    bool borderVisible() const { return borderVisible_; }
    void setTextAlignment(Neu_TextAlignment alignment) { textAlignment_ = alignment; requestRedraw(); }
    Neu_TextAlignment textAlignment() const { return textAlignment_; }
    void setTextOffset(int top, int right, int bottom, int left) { textOffset_ = {std::max(0, top), std::max(0, right), std::max(0, bottom), std::max(0, left)}; requestRedraw(); }
    void setTextInsets(int left, int top, int right, int bottom) { setTextOffset(top, right, bottom, left); }
    Neu_TextOffset textOffset() const { return textOffset_; }
    void addRichTextFragment(const Neu_TextFragment& fragment) { richTextFragments_.push_back(fragment); requestRedraw(); }
    void clearRichTextFragments() { richTextFragments_.clear(); requestRedraw(); }
    const std::vector<Neu_TextFragment>& richTextFragments() const { return richTextFragments_; }
    void setAutoScroll(bool enabled) { if (autoScroll_ != enabled) { autoScroll_ = enabled; requestRedraw(); } }
    bool autoScroll() const { return autoScroll_; }
    void setScrollOffset(int x, int y);
    int scrollX() const { return scrollX_; }
    int scrollY() const { return scrollY_; }
    void setVirtualSize(int width, int height);
    Neu_Size virtualSize() const { return virtualSize_; }
    bool hovered() const { return hover_; }
    void setFocused(bool focused) { if (focused_ != focused) { focused_ = focused; requestRedraw(); } }
    bool focused() const { return focused_; }
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
    void drawTextColored(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme, const std::string& s, int x, int y, const Neu_Color& color, bool bold = false, bool italic = false, bool underline = false, bool strikethrough = false, bool doubleStrikethrough = false, bool monospace = false, int headingLevel = 0);
    int measureTextWidth(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme, const std::string& s, bool bold = false, bool italic = false, bool monospace = false, int headingLevel = 0) const;
    std::vector<std::string> wrapTextToWidth(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme, const std::string& s, int maxWidth) const;
    std::string truncateTextToWidth(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme, const std::string& s, int maxWidth) const;
    int alignedTextX(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme, const std::string& s, int left, int width) const;
    void drawIconBmp(Display* d, Drawable drawable, GC gc, int x, int y, int maxSize);
    void invokeClick();
    void invokeTextChanged();
    Neu_Layout layout_;
    std::string text_;
    std::string hintText_;
    bool hintExpanded_{false};
    bool wordWrap_{false};
    bool truncateText_{true};
    Neu_TextAlignment textAlignment_{Neu_TextAlignment::Left};
    Neu_TextOffset textOffset_{};
    std::vector<Neu_TextFragment> richTextFragments_;
    Neu_IconBmp icon_;
    std::string iconPath_;
    Neu_Callbacks callbacks_;
    Neu_Window* parent_{nullptr};
    bool visible_{true};
    bool enabled_{true};
    bool hover_{false};
    bool focused_{false};
    bool pressed_{false};
    bool borderVisible_{false};
    bool autoScroll_{false};
    int hoverAnchorX_{0};
    int hoverAnchorY_{0};
    std::chrono::steady_clock::time_point hoverStartTime_{};
    bool hoverHintArmed_{false};
    int activeScrollDrag_{0};
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
    void selectAll();
    void clearSelection();
    void setSelection(size_t start, size_t end);
    bool hasSelection() const;
    void undo();
    void redo();
    bool canUndo() const { return !undoStack_.empty(); }
    bool canRedo() const { return !redoStack_.empty(); }
    std::string selectedText() const;
    size_t selectionStart() const { return std::min(selectionStart_, selectionEnd_); }
    size_t selectionEnd() const { return std::max(selectionStart_, selectionEnd_); }
    void setInsertMode(bool enabled) { insertMode_ = enabled; requestRedraw(); }
    bool insertMode() const { return insertMode_; }
protected:
    void replaceSelectionWith(const std::string& text);
    void deleteSelection();
    void moveCursorWithSelection(size_t newCursor, bool extendSelection);
    void pushUndoSnapshot();
    void restoreEditSnapshot(const Neu_TextEditSnapshot& snapshot);
    size_t cursor_{0};
    int textScrollX_{0};
    size_t selectionStart_{0};
    size_t selectionEnd_{0};
    std::vector<Neu_TextEditSnapshot> undoStack_;
    std::vector<Neu_TextEditSnapshot> redoStack_;
    size_t undoLimit_{256};
    bool insertMode_{false};
    bool mouseSelecting_{false};
    size_t mouseSelectAnchor_{0};
    bool mouseDraggingSelectedText_{false};
    size_t dragSourceStart_{0};
    size_t dragSourceEnd_{0};
    size_t dragDropCursor_{0};
    std::string dragText_;
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
    void setMultiSelect(bool enabled) { multiSelect_ = enabled; requestRedraw(); }
    bool multiSelect() const { return multiSelect_; }
    const std::set<int>& selectedIndices() const { return selectedIndices_; }
    void draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme) override;
    void handleXEvent(XEvent& ev) override;
protected:
    std::vector<std::string> items_;
    int selected_{-1};
    int hoveredIndex_{-1};
    int anchorIndex_{-1};
    bool multiSelect_{true};
    std::set<int> selectedIndices_;
};

class Neu_ComboBox : public Neu_Listbox {
public:
    using Neu_Listbox::Neu_Listbox;
    const char* className() const override { return "Neu_ComboBox"; }
    bool isDropDownOpen() const { return open_; }
    void closeDropDown() { open_ = false; requestRedraw(); }
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
    void setColumnWidths(const std::vector<int>& widths);
    void setColumnWidth(size_t column, int width);
    int columnWidth(size_t column) const;
    void setHeaderResizable(bool enabled) { headerResizable_ = enabled; requestRedraw(); }
    bool headerResizable() const { return headerResizable_; }
    void setHeaderHeight(int height) { headerHeight_ = std::max(18, height); requestRedraw(); }
    int headerHeight() const { return headerHeight_; }
    void setMultiSelect(bool enabled) { multiSelect_ = enabled; requestRedraw(); }
    bool multiSelect() const { return multiSelect_; }
    const std::set<int>& selectedRows() const { return selectedRows_; }
    void draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme) override;
    void handleXEvent(XEvent& ev) override;
protected:
    int effectiveColumnWidth(size_t column, int controlWidth = 0) const;
    int totalColumnWidth(size_t columnCount, int controlWidth = 0) const;
    Neu_StringTable* model_{nullptr};
    std::vector<int> columnWidths_;
    int headerHeight_{24};
    bool headerResizable_{true};
    int resizingColumn_{-1};
    int resizeStartX_{0};
    int resizeStartWidth_{0};
    int selectedRow_{-1};
    int selectedCol_{-1};
    int hoveredRow_{-1};
    int hoveredCol_{-1};
    int anchorRow_{-1};
    bool multiSelect_{true};
    std::set<int> selectedRows_;
};

class Neu_TreeView : public Neu_ListView {
public:
    using Neu_ListView::Neu_ListView;
    const char* className() const override { return "Neu_TreeView"; }
    void expandAll();
    void collapseAll();
    void toggleNodePath(const std::string& path);
    bool isPathCollapsed(const std::string& path) const;
    void setMultiSelect(bool enabled) { multiSelect_ = enabled; requestRedraw(); }
    bool multiSelect() const { return multiSelect_; }
    const std::set<int>& selectedVisibleRows() const { return selectedVisibleRows_; }
    void setTreeColumnWidth(int width) { treeColumnWidth_ = std::max(96, width); requestRedraw(); }
    int treeColumnWidth() const { return treeColumnWidth_; }
    void draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme) override;
    void handleXEvent(XEvent& ev) override;

private:
    std::set<std::string> collapsedPaths_;
    int selectedVisibleRow_{-1};
    int hoveredVisibleRow_{-1};
    int anchorVisibleRow_{-1};
    bool multiSelect_{true};
    std::set<int> selectedVisibleRows_;
    int treeColumnWidth_{260};
    bool headerResizeActive_{false};
    int headerResizeStartX_{0};
    int headerResizeStartWidth_{0};
};

class Neu_Placement : public Neu_Control {
public:
    using Neu_Control::Neu_Control;
    const char* className() const override { return "Neu_Placement"; }
    void add(std::shared_ptr<Neu_Control> child);
    void setParent(Neu_Window* parent) override;
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
    void setCategories(const std::vector<std::string>& categories) { categories_ = categories; requestRedraw(); }
    void setItems(const std::string& category, const std::vector<std::string>& items) { items_[category] = items; requestRedraw(); }

    // Popup visibility helpers. The menu remains visible by default for
    // backwards compatibility with existing demos; applications can hide it
    // after construction and open it explicitly through show()/showAt().
    void show() { if (!visible_) { visible_ = true; requestRedraw(); } }
    void showAt(int x, int y) { layout_.left = x; layout_.top = y; show(); }
    void hide() { if (visible_) { visible_ = false; requestRedraw(); } }
    void toggle() { if (visible_) { hide(); } else { show(); } }
    bool isVisible() const { return visible_; }

    void draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme) override;
    void handleXEvent(XEvent& ev) override;
private:
    std::vector<std::string> categories_;
    std::map<std::string, std::vector<std::string>> items_;
    int selectedCategory_{0};
};

using Neu_PopupWindowMenu = Neu_PopWindowMenu;


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
    bool dragging_{false};
};

class Neu_ScrollWindow : public Neu_Placement {
public:
    using Neu_Placement::Neu_Placement;
    const char* className() const override { return "Neu_ScrollWindow"; }
    void add(std::shared_ptr<Neu_Control> child);
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
    using Neu_Label::Neu_Label;
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
    void setDefaultFontName(const std::string& fontName) { defaultFontName_ = fontName; requestRedraw(); }
    void setDefaultFontColor(const Neu_Color& color) { defaultFontColor_ = color; requestRedraw(); }
    void setDefaultBackgroundColor(const Neu_Color& color) { defaultBackgroundColor_ = color; requestRedraw(); }
    void setSketchHighlightColor(const Neu_Color& color) { sketchHighlightColor_ = color; requestRedraw(); }
    void applyToolbarAction(int actionIndex);
    void applyBold() { applyToolbarAction(0); }
    void applyItalic() { applyToolbarAction(1); }
    void applyUnderline() { applyToolbarAction(2); }
    void applyStrikethrough() { applyToolbarAction(3); }
    void applyDoubleStrikethrough() { applyToolbarAction(4); }
    void applyHeading(int level);
    void applyMonospace() { applyToolbarAction(7); }
    void cycleToolbarFont() { applyToolbarAction(8); }
    void applyFontColor(const Neu_Color& color);
    void applyBackgroundColor(const Neu_Color& color);
    void applyHighlightColor(const Neu_Color& color);
    void draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme) override;
    void handleXEvent(XEvent& ev) override;
private:
    void applyFragmentStyleToSelection(const Neu_TextFragment& style);
    bool readOnly_{false};
    bool toolbarVisible_{true};
    std::string languageName_{"C++17"};
    std::string defaultFontName_;
    Neu_Color defaultFontColor_{20,28,38,255};
    Neu_Color defaultBackgroundColor_{255,255,255,0};
    Neu_Color sketchHighlightColor_{255,240,120,160};
    std::vector<std::string> toolbarFonts_{"Sans", "Serif", "SansSerif", "Monospace"};
    int toolbarFontIndex_{0};
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
    void no_crlf() { appendNextInline_ = true; }
    void crlf();
    void addLabel(const std::string& text);
    void addMultilineLabel(const std::string& text);
private:
    std::vector<std::string> iconPaths_;
    int labelSpacing_{8};
    int labelLineSpacing_{4};
    int contentCursorX_{12};
    int contentCursorY_{12};
    bool appendNextInline_{false};
    size_t iconIndexForText(const std::string& text) const;
};


class Neu_CheckBox : public Neu_Control {
public:
    using Neu_Control::Neu_Control;
    const char* className() const override { return "Neu_CheckBox"; }
    void setChecked(bool checked) { checked_ = checked; requestRedraw(); }
    bool checked() const { return checked_; }
    void draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme) override;
    void handleXEvent(XEvent& ev) override;
protected:
    bool checked_{false};
};

class Neu_RadioButton : public Neu_CheckBox {
public:
    using Neu_CheckBox::Neu_CheckBox;
    const char* className() const override { return "Neu_RadioButton"; }
    void draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme) override;
};

class Neu_ToggleButton : public Neu_CheckBox {
public:
    using Neu_CheckBox::Neu_CheckBox;
    const char* className() const override { return "Neu_ToggleButton"; }
    void draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme) override;
};

class Neu_ProgressBar : public Neu_Control {
public:
    using Neu_Control::Neu_Control;
    const char* className() const override { return "Neu_ProgressBar"; }
    void setProgress(float progress) { progress_ = std::max(0.0f, std::min(1.0f, progress)); requestRedraw(); }
    float progress() const { return progress_; }
    void draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme) override;
private:
    float progress_{0.0f};
};

class Neu_Slider : public Neu_Control {
public:
    using Neu_Control::Neu_Control;
    const char* className() const override { return "Neu_Slider"; }
    void setRange(int minimum, int maximum) { min_ = minimum; max_ = std::max(minimum + 1, maximum); setValue(value_); }
    void setValue(int value) { value_ = std::max(min_, std::min(max_, value)); requestRedraw(); }
    int value() const { return value_; }
    void setVertical(bool vertical) { vertical_ = vertical; requestRedraw(); }
    bool vertical() const { return vertical_; }
    void draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme) override;
    void handleXEvent(XEvent& ev) override;
private:
    int min_{0};
    int max_{100};
    int value_{50};
    bool vertical_{false};
    bool dragging_{false};
};

class Neu_Spinner : public Neu_Control {
public:
    using Neu_Control::Neu_Control;
    const char* className() const override { return "Neu_Spinner"; }
    void setRange(int minimum, int maximum) { min_ = minimum; max_ = std::max(minimum, maximum); setValue(value_); }
    void setValue(int value) { value_ = std::max(min_, std::min(max_, value)); setText(std::to_string(value_)); }
    int value() const { return value_; }
    void draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme) override;
    void handleXEvent(XEvent& ev) override;
private:
    int min_{0};
    int max_{100};
    int value_{0};
};

class Neu_GroupBox : public Neu_Placement {
public:
    using Neu_Placement::Neu_Placement;
    const char* className() const override { return "Neu_GroupBox"; }
    void draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme) override;
};

class Neu_Separator : public Neu_Control {
public:
    using Neu_Control::Neu_Control;
    const char* className() const override { return "Neu_Separator"; }
    void setVertical(bool vertical) { vertical_ = vertical; requestRedraw(); }
    void draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme) override;
private:
    bool vertical_{false};
};

class Neu_LinkLabel : public Neu_Label {
public:
    using Neu_Label::Neu_Label;
    const char* className() const override { return "Neu_LinkLabel"; }
    void draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme) override;
    void handleXEvent(XEvent& ev) override;
};

class Neu_ToolBar : public Neu_Placement {
public:
    using Neu_Placement::Neu_Placement;
    const char* className() const override { return "Neu_ToolBar"; }
    void draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme) override;
};

class Neu_TabView : public Neu_Placement {
public:
    using Neu_Placement::Neu_Placement;
    const char* className() const override { return "Neu_TabView"; }
    int addTab(const std::string& title, std::shared_ptr<Neu_Placement> page);
    void setSelectedTab(int index) { selectedTab_ = std::max(0, std::min(index, static_cast<int>(pages_.size()) - 1)); requestRedraw(); }
    int selectedTab() const { return selectedTab_; }
    void setParent(Neu_Window* parent) override;
    void draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme) override;
    void handleXEvent(XEvent& ev) override;
private:
    std::vector<std::string> titles_;
    std::vector<std::shared_ptr<Neu_Placement>> pages_;
    int selectedTab_{0};
};

class Neu_Splitter : public Neu_Placement {
public:
    using Neu_Placement::Neu_Placement;
    const char* className() const override { return "Neu_Splitter"; }
    void setVertical(bool vertical) { vertical_ = vertical; requestRedraw(); }
    void setSplitPosition(int pixels) { splitPosition_ = std::max(24, pixels); requestRedraw(); }
    int splitPosition() const { return splitPosition_; }
    void draw(Display* d, Drawable drawable, GC gc, const Neu_Theme& theme) override;
    void handleXEvent(XEvent& ev) override;
private:
    bool vertical_{true};
    int splitPosition_{160};
    bool dragging_{false};
};

using Neu_TableView = Neu_ListView;
using Neu_Table = Neu_ListView;
using Neu_TextArea = Neu_Multilinetextbox;
using Neu_Panel = Neu_Placement;
using Neu_Composite = Neu_Placement;

class Neu_Window {
public:
    Neu_Window(Neu_Application& app, int width, int height, const std::string& title);
    ~Neu_Window();
    bool create();
    void show();
    void close();
    void redraw();
    void invalidate();
    void requestRedraw();
    bool hasPendingRedraw() const;
    void flushPendingRedraw();
    void paint(Drawable target);
    void setMultiStageDoubleBuffering(bool enabled);
    bool multiStageDoubleBuffering() const { return multiStageDoubleBuffering_; }
    void add(std::shared_ptr<Neu_Control> control);
    void handleXEvent(XEvent& ev);
    Window xid() const { return window_; }
    Neu_Theme& theme() { return theme_; }
    void setTheme(const Neu_Theme& t);
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
    Neu_Control* focusedControl_{nullptr};
    Neu_Control* hoveredControl_{nullptr};
    Neu_Control* captureControl_{nullptr};
    bool closing_{false};
    bool dirty_{false};
    bool painting_{false};
    bool redrawRequestedDuringPaint_{false};
    bool mapped_{false};
    bool multiStageDoubleBuffering_{true};
#ifdef _WIN32
    HDC memoryDc_{nullptr};
    HBITMAP backBitmap_{nullptr};
    HBITMAP oldBitmap_{nullptr};
#else
    Pixmap stageBackground_{0};
    Pixmap stageCompose_{0};
    Pixmap stageFinal_{0};
    XFontStruct* coreFont_{nullptr};
#endif
    int bufferWidth_{0};
    int bufferHeight_{0};
    void ensureBuffers();
    void releaseBuffers();
    void drawScene(Drawable target);
    Neu_Control* hitTest(int x, int y);
    void setFocusedControl(Neu_Control* control);
};

unsigned long Neu_Pixel(Display* d, const Neu_Color& color);
Neu_Color Neu_PixelToColor(Display* d, unsigned long pixel);
const char* Neu_SelectedBackendName();
bool Neu_IsWaylandBackendSelected();
void Neu_SetSmoothGraphicsOptions(const Neu_SmoothGraphicsOptions& options);
Neu_SmoothGraphicsOptions Neu_GetSmoothGraphicsOptions();
void Neu_EnableAntialiasing(bool enabled);
void Neu_UseVirtualMachineFriendlyDefaults(bool enabled);
void Neu_EnableMultiStageDoubleBuffering(bool enabled);
void Neu_SetCurrentDrawingTheme(const Neu_Theme& theme);
void Neu_ApplyThemeRenderingOptions(const Neu_Theme& theme);
Neu_Color Neu_LightenColor(const Neu_Color& color, int amount);
Neu_Color Neu_DarkenColor(const Neu_Color& color, int amount);
Neu_Color Neu_MixColor(const Neu_Color& a, const Neu_Color& b, double t);
void Neu_DrawRoundedRect(Display* d, Drawable drawable, GC gc, int x, int y, int w, int h, int radius, bool fill);
void Neu_DrawSmoothRoundedRect(Display* d, Drawable drawable, GC gc, const Neu_Color& color, const Neu_Color& background, int x, int y, int w, int h, int radius, bool fill, int supersample = 4);
void Neu_DrawSmoothDropShadow(Display* d, Drawable drawable, GC gc, const Neu_Color& shadow, const Neu_Color& background, int x, int y, int w, int h, int radius, int blur, int offsetX, int offsetY);

} // namespace neutrino
