# Neutrino verification notes

Revision verified for the expanded Linux/Windows graphical test-suite update.

## Source/package scope

This revision adds a focused common test suite for the Neutrino GUI framework. The package now contains:

- 19 common Linux/Windows demo/test applications wired through Makefile, CMake, autoconf Makefile, Windows CMake, and Visual Studio 2022 project files.
- 1 additional Windows-only native Win32 starter demo: `NeutrinoWin32Demo`.
- `TEST_APPLICATIONS.md`, which maps every binary and VS2022 project to its source file and feature coverage.

The 12 new focused tests are:

1. `neutrino_test_01_buttons_icons`
2. `neutrino_test_02_text_inputs`
3. `neutrino_test_03_lists_combo_autoscroll`
4. `neutrino_test_04_listview_typed_data`
5. `neutrino_test_05_treeview_collapse`
6. `neutrino_test_06_placement_layout_scaling`
7. `neutrino_test_07_popup_menu_categories`
8. `neutrino_test_08_richtext_code`
9. `neutrino_test_09_readonly_richtext_icons`
10. `neutrino_test_10_images_progress_labels`
11. `neutrino_test_11_rendering_buffering`
12. `neutrino_test_12_scroll_windows_heavy_data`

Together they cover buttons/menu items/flat buttons, BMP icons, text input, password input, multiline input, listbox, combobox, ListView, TreeView collapse, fixed/scaled layout, placement containers, popup menu categories, labels, multiline labels, image control, progress square, scrollbars, scroll windows, rich code text, read-only rich text with `#` icon selection, typed string interpretation, hints, shadows, hover highlighting, VM-friendly mode, and multi-stage double buffering.

## Linux/X11 verification performed in this environment

The Linux CMake build completed successfully:

```sh
cmake -S . -B cmake-build
cmake --build cmake-build -j1
```

The basic Makefile build completed and produced all expected 19 Linux demo/test executables under `build/`:

```sh
make clean
make -j1
find build -maxdepth 1 -type f -perm -111 | wc -l
# 19
```

The autoconf Makefile path configured successfully and produced all expected 19 Linux demo/test executables under `build-autoconf/`:

```sh
./configure
make -f Makefile.autoconf -j1
find build-autoconf -maxdepth 1 -type f -perm -111 | wc -l
# 19
```

Some Linux build commands were continued incrementally because the execution tool terminated long compile/link runs before all targets were finished in a single call. The final verified state contains the expected library and all expected test/demo binaries in both `build/` and `build-autoconf/`.

## Visual Studio 2022 verification scope

The Visual Studio 2022 project files were statically checked in this Linux container:

- `msvc/Neutrino.sln` references all existing `.vcxproj` files.
- All `.vcxproj` files are well-formed XML.
- The solution includes `Neutrino`, `NeutrinoWin32Demo`, the existing demos, and the 12 new focused `NeutrinoTest*` projects.
- All test projects reference the `Neutrino` static library project.
- Every `.vcxproj` Debug and Release `ClCompile` block sets `<LanguageStandard>stdcpp17</LanguageStandard>`.
- Every `.vcxproj` Debug and Release `PreprocessorDefinitions` block includes `_CRT_SECURE_NO_WARNINGS`.
- The debugger working directory is set to the repository root with `$(SolutionDir)..`, so `assets/icons/*.bmp` resolves when a test is launched from Visual Studio.

MSVC and the Windows SDK are not available in this Linux container, so the VS2022 solution was not compiled here.

## Windows/Win32 behavior retained from the previous update

The Windows backend remains based on native Win32/GDI and uses:

- `CreateWindowExW`, `WNDCLASSEX`, and a blocking Win32 message loop.
- GDI drawing through rounded rectangles, text output, compatible memory DCs, bitmap blits, and BMP icon rendering.
- `CLEARTYPE_QUALITY` font rendering.
- `WM_CLOSE` handling that invokes the optional close callback, unregisters the window, destroys the native `HWND`, and quits after the last window closes.
- `WM_PAINT`-based drawing with `InvalidateRect()` redraw coalescing and `WM_ERASEBKGND` suppression.
- Mouse routing to hovered/captured controls and keyboard routing to the focused control.
- Textbox/password/multiline editing for printable input, Backspace, Delete, Home, End, Left, and Right.
- Client-area sizing through `AdjustWindowRectEx()`.
- Parent propagation for nested `Neu_Placement` controls.
- Mouse wheel targeting for hovered/focused scrollable controls, with Shift+wheel horizontal scrolling on Windows.


## 2026-07-02 verification after text/rendering update

Verified on Linux in this packaging environment:

```sh
make -j1
cmake -S . -B cmake-build
cmake --build cmake-build -j1
make -f Makefile.autoconf -j1
```

The Visual Studio 2022 solution and Win32 backend were updated in source form, but MSVC/Windows SDK are not available in this Linux packaging container, so the VS2022 build itself was not executed here.

## 2026-07-02 clipping/input update

Applied and verified the following fixes:

- Windows scroll-window drawing now reapplies a strict viewport clipping region around each child draw.
- Linux scroll-window drawing now composites children into a viewport-sized offscreen pixmap before copying back to the real window, preventing child controls from overriding the parent viewport clip.
- Hint popups are armed on stable hover and require a 5-second dwell before painting; the hover anchor resets when the pointer moves more than 4 px.
- Windows hint text uses an inset text origin and width-bound truncation to avoid missing/overflowing edge text.
- Single-line textbox horizontal scrolling keeps the caret visible as new text is typed.
- ListView and TreeView cells/rows are clipped and truncated to the visible cell width after horizontal scrolling.
- ListBox, ComboBox list behavior, ListView, and TreeView default to multi-select and support Ctrl-click and Shift-click selection paths.
- RichTextCode and multiline text paths preserve CRLF/LF line breaks and draw an insert caret while focused.
- Labels remain borderless by default, with border display controlled through setBorderVisible(true).

Verification performed on Linux:

```sh
make -j2
cmake -S . -B cmake-build
cmake --build cmake-build -j2
./configure
make -f Makefile.autoconf -j2
```

The Win32 backend and VS2022 project files were updated, but MSVC/Windows SDK compilation was not available inside this Linux container.

## 2026-07-02 Win32 MSVC compile fix

Patched `src/win32/Neu_Win32.cpp` around the ListView and TreeView clipping paths to avoid MSVC template deduction failures with `std::min` / `std::max` when mixing Win32 `RECT` members (`LONG`) with `int` viewport values. The affected values are now cast explicitly to `int` before calling `std::min` / `std::max` and before calculating draw widths.

This directly addresses the VS2022 errors reported around lines 1523-1526, 1536-1537, and 1768-1770.

## 2026-07-02 Win32 MSVC clipping compile fix

Patched `src/win32/Neu_Win32.cpp` to remove the MSVC C2672/C2660 errors reported around ListView/TreeView clipping. The fix avoids mixed `LONG`/`int` `std::min` and `std::max` deduction by converting Win32 `RECT` coordinates to `int` before clipping/truncation math.

Affected areas:

- `Neu_ListView::draw(...)` visible-cell clipping and `IntersectClipRect(...)` calls.
- `Neu_TreeView::draw(...)` text clipping and `IntersectClipRect(...)` calls.

Linux build paths rechecked after the patch:

```sh
make -j2
cmake -S . -B cmake-build
cmake --build cmake-build -j2
./configure
make -f Makefile.autoconf -j2
```

MSVC cannot be executed in this Linux container, but the reported Win32 compile errors were directly patched in the reported file and line area.

## Stage2 verification

Stage2 should be verified with the new focused tests:

```sh
make clean
make -j2 build/libNeutrino.a     build/neutrino_test_13_swing_swt_controls     build/neutrino_test_14_label_offsets_themes     build/neutrino_test_15_selection_controls     build/neutrino_test_16_value_controls     build/neutrino_test_17_tabs_toolbar_splitter     build/neutrino_test_18_stage2_scroll_controls
cmake -S . -B cmake-build-stage2
cmake --build cmake-build-stage2 --target Neutrino     neutrino_test_13_swing_swt_controls     neutrino_test_14_label_offsets_themes     neutrino_test_15_selection_controls     neutrino_test_16_value_controls     neutrino_test_17_tabs_toolbar_splitter     neutrino_test_18_stage2_scroll_controls -j2
make -f Makefile.autoconf clean
make -f Makefile.autoconf -j2 build-autoconf/libNeutrino.a     build-autoconf/neutrino_test_13_swing_swt_controls     build-autoconf/neutrino_test_14_label_offsets_themes     build-autoconf/neutrino_test_15_selection_controls     build-autoconf/neutrino_test_16_value_controls     build-autoconf/neutrino_test_17_tabs_toolbar_splitter     build-autoconf/neutrino_test_18_stage2_scroll_controls
```

The Visual Studio 2022 solution contains matching Stage2 projects and remains configured for standard C++17 and `_CRT_SECURE_NO_WARNINGS`.
## Stage2 text selection and clipboard fix

Editable text controls now preserve repeated spaces/indentation and support `Ctrl+A`, `Ctrl+C`, `Ctrl+X`, `Ctrl+V`, Shift+arrow selection, and selection-aware Backspace/Delete. See `STAGE2_TEXT_SELECTION_CLIPBOARD_FIXES.md`.


## Stage2 popup visibility verification

Verified after adding `Neu_PopWindowMenu::show()`, `showAt()`, `hide()`, `toggle()`, and `isVisible()`:

```sh
make -j2 build/libNeutrino.a build/neutrino_test_07_popup_menu_categories
cmake -S . -B /mnt/data/cmake-neutrino-popup
cmake --build /mnt/data/cmake-neutrino-popup --target Neutrino neutrino_test_07_popup_menu_categories -j2
make -f Makefile.autoconf -j2 build-autoconf/libNeutrino.a build-autoconf/neutrino_test_07_popup_menu_categories
```
