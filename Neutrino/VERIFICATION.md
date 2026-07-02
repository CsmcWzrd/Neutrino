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

