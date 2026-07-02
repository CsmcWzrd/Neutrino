# Neutrino

Neutrino is a small C++17 GUI framework prototype for Linux/X11 and Windows/Win32 with smooth rounded controls, fixed-position layout metadata, scaling, max control sizing, themes, and C-style function pointer callbacks.



## Windows / Visual Studio 2022 build

The project now includes a native Windows backend implemented with basic Win32/GDI calls. It does not use X11 on Windows and does not require GTK, Qt, wxWidgets, Cairo, or signals/slots. The same public C++17 API uses the `Neu_` prefix, for example `Neu_Window`, `Neu_Button`, `Neu_Textbox`, `Neu_ListView`, and `Neu_Callbacks`.

Open this solution in Visual Studio 2022:

```text
msvc/Neutrino.sln
```

Projects included:

| Project | Type | Purpose |
|---|---|---|
| `Neutrino` | Static library | Builds the Win32/GDI implementation from `src/win32/Neu_Win32.cpp`. |
| `NeutrinoWin32Demo` | Windows app | Demonstrates `Neu_Button`, `Neu_Textbox`, `Neu_Passwordbox`, `Neu_Listbox`, `Neu_Label`, `Neu_ProgressSquare`, `Neu_ReadOnlyRichText`, BMP icons, function-pointer callbacks, and software double buffering on Windows. |

The Windows backend uses:

- `CreateWindowExW` / `WNDCLASSEX` / Win32 message loop
- GDI drawing with `RoundRect`, `TextOutW`, `StretchDIBits`, compatible memory DCs, and `BitBlt`
- `CLEARTYPE_QUALITY` fonts for smoother text
- BMP-only icon loading
- C-style callback function pointers with `void* userData`

CMake can also generate a Visual Studio build directory on Windows:

```bat
cmake -S . -B build-vs2022 -G "Visual Studio 17 2022" -A x64
cmake --build build-vs2022 --config Release
```

On Linux, the existing X11 build remains unchanged:

```sh
make
cmake -S . -B cmake-build
cmake --build cmake-build
./configure
make -f Makefile.autoconf
```

## Controls

- Neu_Textbox
- Neu_Passwordbox
- Neu_Multilinetextbox
- Neu_Listbox
- Neu_ComboBox
- Neu_ListView
- Neu_TreeView
- Neu_Button
- Neu_Window
- Neu_PopWindowMenu
- Neu_Placement
- Neu_MenuItem / Neu_FlatButton

Neu_Button, Neu_MenuItem, and Neu_FlatButton support BMP-only icon loading through `setIconBmp()`. The package includes a small BMP icon at `assets/icons/sample_icon.bmp` used by the test applications.

## Layout

Each control uses `Neu_Layout`:

```cpp
Neu_Layout{left, top, width, height, scale, maxWidth, maxHeight}
```

The top-left position is fixed. Width and height are multiplied by `scale`, then clamped to `maxWidth` and `maxHeight` when those values are non-zero.

## Event handling

No signals/slots are used. Events use plain C-style function pointers and `void* userData` similar to GTK-style callbacks.

```cpp
static void on_click(neutrino::Neu_Control* sender, void* user_data) {
    // handle event
}

neutrino::Neu_Callbacks cb;
cb.onClick = on_click;
cb.userData = my_pointer;
button->setCallbacks(cb);
```

Supported callback types include click, focus, blur, text changed, selection changed, key down, and window close.

## STL model binding

Neu_ListView and Neu_TreeView bind to:

```cpp
using Neu_StringTable = std::vector<std::vector<std::string>>;
```

This maps the requested `List<List<string>>` model to standard C++ STL.

## String-to-type interpretation

`Neu_TypeInterpreter::interpret()` recognizes examples such as:

- `42` -> Int64
- `98.5` -> Double
- `3.14f` -> Float
- `0x2A` or `hex:2A` -> Hex
- `1:true`, `true`, `checkbox:1` -> Boolean
- `0:false`, `false`, `checkbox:0` -> Boolean
- `bin8:10101010`, `bin16:...`, `bin32:...`, `bin64:...`, `bin128:...` -> binary values
- `image:path.bmp` -> BMP image reference
- `tri:maybe` -> tristate string
- `enum:Admin` -> enum string
- `utf:Text` -> UTF string marker


## Smooth graphics and antialiasing

Neutrino now has a Cairo-free smooth rendering layer for rounded controls. The default path enables antialiased rounded rectangles by supersampling into an `XImage` and pushing the result through Xlib. The framework also checks at runtime whether the XRender extension is available with `dlopen()`/`dlsym()`; this avoids a hard build dependency on XRender development headers while still allowing the framework to prefer an XRender-aware backend when the server supports it.

Public configuration API:

```cpp
neutrino::Neu_SmoothGraphicsOptions options;
options.enabled = true;
options.backend = neutrino::Neu_GraphicsBackend::XRenderAntialias;
options.supersample = 4;
neutrino::Neu_SetSmoothGraphicsOptions(options);

neutrino::Neu_EnableAntialiasing(true);
```

Available backends:

- `Neu_GraphicsBackend::X11Basic` disables the antialiasing path and uses plain Xlib primitives.
- `Neu_GraphicsBackend::SoftwareAntialias` uses Cairo-free CPU supersampling and XImage upload.
- `Neu_GraphicsBackend::XRenderAntialias` prefers XRender availability detection and falls back to the Cairo-free software antialiasing path.

No Cairo dependency is used. Link requirements remain X11 plus `libdl` on Linux.

## Test applications

The package now includes multiple test applications that depict usage of all requested controls and windows:

| Binary | Source | Purpose |
|---|---|---|
| `neutrino_demo` | `examples/demo.cpp` | Compact starter demo using the framework basics. |
| `neutrino_test_all_controls` | `examples/test_all_controls.cpp` | Displays Neu_Textbox, Neu_Passwordbox, Neu_Multilinetextbox, Neu_Listbox, Neu_ComboBox, Neu_ListView, Neu_TreeView, Neu_Button, Neu_FlatButton, Neu_MenuItem, Neu_Placement, Neu_PopWindowMenu, BMP icons, themes, layout scaling metadata, and function pointer callbacks. |
| `neutrino_test_windows` | `examples/test_windows.cpp` | Demonstrates Neu_Window as a main window and dialog-style window, close callbacks, theme switching, and pop-window menu usage. |
| `neutrino_test_typed_views` | `examples/test_typed_views.cpp` | Demonstrates ListView/TreeView binding to `std::vector<std::vector<std::string>>` and string-to-datatype interpretation for numbers, binary values, hex, checkbox/booleans, tristate, enum, image, float/double, and UTF strings. |
| `neutrino_test_smooth_graphics` | `examples/test_smooth_graphics.cpp` | Demonstrates antialiased rounded controls, backend switching, and the Cairo-free smooth graphics API. |

These examples are graphical X11 applications. Run them from the project root so the sample BMP path resolves correctly:

```sh
./build/neutrino_test_all_controls
./build/neutrino_test_windows
./build/neutrino_test_typed_views
./build/neutrino_test_smooth_graphics
```

## Build with CMake

```sh
mkdir -p build
cd build
cmake ..
cmake --build .
./neutrino_demo
./neutrino_test_all_controls
./neutrino_test_windows
./neutrino_test_typed_views
./neutrino_test_smooth_graphics
```

## Build with basic Makefile

```sh
make
./build/neutrino_demo
./build/neutrino_test_all_controls
./build/neutrino_test_windows
./build/neutrino_test_typed_views
./build/neutrino_test_smooth_graphics
```

## Build with autoconf

```sh
./autogen.sh
./configure
make -f Makefile.autoconf
./build-autoconf/neutrino_demo
./build-autoconf/neutrino_test_all_controls
./build-autoconf/neutrino_test_windows
./build-autoconf/neutrino_test_typed_views
./build-autoconf/neutrino_test_smooth_graphics
```

## Notes

This is a practical foundation package, not a full production widget toolkit. It uses Xlib directly, has a Cairo-free antialiasing path for rounded controls, and keeps the rendering path intentionally lightweight. Future steps could add clipping refinement, richer text input, IME support, accessibility, and complete widget focus traversal. Multi-stage double buffering, scrollbars, and rich-text/code controls are already included in this revision.

## Latest source-organization and TreeView update

This package now keeps framework class implementations in separate C++ source files under `src/` instead of one monolithic implementation file. Examples include `Neu_Application.cpp`, `Neu_Control.cpp`, `Neu_Button.cpp`, `Neu_Textbox.cpp`, `Neu_ListView.cpp`, `Neu_TreeView.cpp`, and `Neu_Window.cpp`.

`Neu_TreeView` now supports collapsing and expanding hierarchical nodes. Rows are still bound through the requested STL-style model (`Neu_StringTable`, an alias for `std::vector<std::vector<std::string>>`). Each model row is interpreted as a tree path, for example `{"Root", "Controls", "Neu_Button"}`. Public APIs include `expandAll()`, `collapseAll()`, `toggleNodePath(path)`, and `isPathCollapsed(path)`. Clicking a visible tree row with children toggles its `+` / `-` state while still dispatching the existing function-pointer selection callback.

A dedicated test app was added:

```sh
./build/neutrino_test_treeview_collapse
```

It shows a collapsible tree with expand-all and collapse-all buttons. Event handling remains plain callback function pointers with `void* userData`; no signals/slots approach is used.

## Formatting

The C++ files were reorganized in an Artistic Style / Allman-compatible layout. The package includes `tools/format_astyle.sh` for systems with `astyle` installed:

```sh
./tools/format_astyle.sh
```

The script uses `astyle --style=allman --indent=spaces=4 --pad-oper --pad-header --unpad-paren --align-pointer=name --suffix=none`.

## Font quality, shadows, hover highlighting, hints, and icons

The default theme now requests `DejaVu Sans-10` and the build system automatically enables Xft/Fontconfig antialiased UTF-8 text when `pkg-config xft` is available on the Linux system. This gives the framework the highest-quality common Linux/X11 font path without using Cairo. When Xft development files are not present, the framework still builds and falls back to plain Xlib text drawing.

Controls now draw soft Cairo-free shadows, change highlight color when the mouse enters/leaves the control, and trigger the existing `onFocus`/`onBlur` function-pointer callbacks from pointer hover state. The public hint API is:

```cpp
control->setHintText("Helpful hint text shown while hovering over the control.");
control->setHintExpanded(true);
```

Hint popups are drawn as rounded shadowed popups near the hovered control. They are limited to 400 pixels wide, wrap text, show a drop-down marker for longer text, and draw a vertical scrollbar when the natural content height exceeds 500 pixels.

Additional BMP icons were added under `assets/icons/`:

- `button_icon.bmp`
- `menu_icon.bmp`
- `save_icon.bmp`
- `sample_icon.bmp`

The demo and all-controls test app now load these BMP icons into `Neu_Button`, `Neu_FlatButton`, `Neu_MenuItem`, and nested placement controls.

## 2026-06-30 heavy-data, scroll, font, and rich text update

This package revision adds a larger data demo and additional controls requested for richer applications.

### New controls

- `Neu_ScrollBar` - standalone vertical or horizontal scrollbar control.
- `Neu_ScrollWindow` - scrollable container/window-style placement area.
- `Neu_Label` - single-line label with optional BMP icon support.
- `Neu_MultilineLabel` - multiline label with optional BMP icon support and auto-scroll support.
- `Neu_RichTextCode` - simple coding-oriented rich text control with line numbers, keyword-line highlighting, horizontal/vertical scrolling, and optional read-only mode.
- `Neu_ProgressSquare` - square progress control; the edges illuminate as progress approaches completion.
- `Neu_ImageView` - BMP image display control.
- `Neu_ReadOnlyRichText` - read-only rich text container that accepts `Neu_Label` and `Neu_MultilineLabel` subcomponents. It can select icons from an `std::vector<std::string>` of BMP paths based on the number of unescaped `#` characters in the label text. Use `\#` to display a literal `#`.

### Auto-scroll support

Scrollable data-heavy controls now expose:

```cpp
control->setAutoScroll(true);
control->setVirtualSize(width, height);
control->setScrollOffset(x, y);
```

Mouse wheel scrolling is handled directly by the control event path. This remains callback/function-pointer based; no signals/slots system has been added.

### New large demo

Build and run:

```sh
make
./build/neutrino_test_heavy_data
```

The demo fills controls with hundreds of rows and many code lines, so `Neu_Listbox`, `Neu_ListView`, `Neu_MultilineLabel`, `Neu_RichTextCode`, and `Neu_ReadOnlyRichText` show auto-scroll behavior.

### Better default font quality

The default theme font name now requests Xft/Fontconfig antialiasing, full hinting, RGB subpixel rendering, and LCD filtering:

```text
DejaVu Sans:size=10:antialias=true:hinting=true:hintstyle=hintfull:rgba=rgb:lcdfilter=lcddefault
```

For best text quality on Ubuntu, install the Xft, Fontconfig, FreeType, and XRender development packages before building:

```sh
sudo apt update
sudo apt install build-essential libx11-dev libxft-dev libfontconfig1-dev libfreetype6-dev libxrender-dev pkg-config
```

`libxrender-dev` is the Ubuntu development package for the X Rendering Extension client library. Runtime systems typically also have `libxrender1`; development builds need `libxrender-dev` so headers and linker metadata are present.

## VirtualBox / low-power rendering mode

If the GUI feels slow inside VirtualBox, VMware, remote X11, KDE compositing, or a software-rendered desktop session, use the VM-friendly rendering mode. This disables the expensive software supersampling path, disables soft shadows, lowers repaint frequency during mouse movement, and uses plain Xlib rounded primitives for responsiveness.

Run any demo with the environment variable:

```sh
NEUTRINO_VM_MODE=1 ./build/neutrino_demo
NEUTRINO_VM_MODE=1 ./build/neutrino_test_all_controls
NEUTRINO_VM_MODE=1 ./build/neutrino_test_heavy_data
```

The shorter alias is also supported:

```sh
NEUTRINO_FAST_RENDER=1 ./build/neutrino_test_heavy_data
```

Several demo applications also accept a command-line flag:

```sh
./build/neutrino_demo --fast
./build/neutrino_test_all_controls --fast
./build/neutrino_test_heavy_data --fast
```

Application code can enable the same profile directly:

```cpp
neutrino::Neu_UseVirtualMachineFriendlyDefaults(true);
```

For custom tuning instead of the preset:

```cpp
neutrino::Neu_SmoothGraphicsOptions options;
options.enabled = false;
options.backend = neutrino::Neu_GraphicsBackend::X11Basic;
options.supersample = 1;
options.drawShadows = false;
options.drawHints = true;
options.repaintOnMouseMove = false;
neutrino::Neu_SetSmoothGraphicsOptions(options);
```

The default visual mode remains available for native Linux desktops, while the VM-friendly mode is intended for slower virtual GPUs and software compositing.

## Software multi-stage double buffering

Neutrino now defaults to software-managed multi-stage double buffering to reduce flicker and reduce visible redraw tearing on slower X11 environments, including VirtualBox/KDE.

The window renderer uses these stages:

1. **Background stage**: clears and prepares the theme background in an offscreen buffer.
2. **Composition stage**: copies the background stage and draws shadows and controls offscreen.
3. **Final stage**: draws transient overlays such as hint popups, then performs a single blit to the X11 window.

This keeps partially drawn controls from appearing onscreen during repaint. The mode is enabled by default, including VM-friendly mode.

Public API:

```cpp
neutrino::Neu_SmoothGraphicsOptions options = neutrino::Neu_GetSmoothGraphicsOptions();
options.multiStageDoubleBuffering = true;
options.bufferStages = 3; // 2 or 3 are useful; 3 enables a final overlay stage
neutrino::Neu_SetSmoothGraphicsOptions(options);

// Global convenience toggle.
neutrino::Neu_EnableMultiStageDoubleBuffering(true);

// Per-window toggle.
window.setMultiStageDoubleBuffering(true);
```

For the fastest VirtualBox path, use:

```sh
NEUTRINO_VM_MODE=1 ./build/neutrino_test_heavy_data
```

VM mode keeps multi-stage buffering enabled but disables expensive supersampled drawing and soft shadows.

## Neutrino Stage2 Additions

Stage2 checkpoints the previous package as Stage1 and adds label text offset/inset support, 24+ additional built-in themes, and Swing/SWT-inspired controls while preserving the existing fixed-position layout system.

Label text can be inset from each side:

```cpp
label->setTextOffset(top, right, bottom, left);
label->setTextInsets(left, top, right, bottom);
```

Built-in theme names are available through `Neu_Theme::BuiltInThemeNames()`, and themes can be selected by name through `Neu_Theme::BuiltInThemeByName("Win11")`. Stage2 includes Windows-like Win95, WinXP, Win10, and Win11 color schemes plus many additional Linux/desktop palettes.


The Stage2 focused Linux/Windows test applications are:

```sh
./build/neutrino_test_13_swing_swt_controls
./build/neutrino_test_14_label_offsets_themes
./build/neutrino_test_15_selection_controls
./build/neutrino_test_16_value_controls
./build/neutrino_test_17_tabs_toolbar_splitter
./build/neutrino_test_18_stage2_scroll_controls
```

The Visual Studio 2022 solution includes matching projects:

- `NeutrinoTest13SwingSwtControls.vcxproj`
- `NeutrinoTest14LabelOffsetsThemes.vcxproj`
- `NeutrinoTest15SelectionControls.vcxproj`
- `NeutrinoTest16ValueControls.vcxproj`
- `NeutrinoTest17TabsToolbarSplitter.vcxproj`
- `NeutrinoTest18Stage2ScrollControls.vcxproj`
