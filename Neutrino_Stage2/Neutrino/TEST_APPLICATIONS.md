# Neutrino test applications

This revision adds a broad cross-platform graphical test suite for the Neutrino GUI framework. The same common C++17 test sources are wired into the Linux Makefile, Linux CMake build, autoconf Makefile, Windows CMake build, and the Visual Studio 2022 solution.

Run Linux examples from the project root so relative BMP paths such as `assets/icons/button_icon.bmp` resolve correctly. The Visual Studio projects set the debugger working directory to the project root for the same reason.

## Common Linux/Windows test projects

| Binary / VS2022 project | Source | Coverage |
|---|---|---|
| `neutrino_demo` / `NeutrinoDemo` | `examples/demo.cpp` | Compact starter demo with basic controls and callbacks. |
| `neutrino_test_all_controls` / `NeutrinoTestAllControls` | `examples/test_all_controls.cpp` | Existing all-controls overview. |
| `neutrino_test_windows` / `NeutrinoTestWindows` | `examples/test_windows.cpp` | Main window, dialog-style window, close callbacks, and themes. |
| `neutrino_test_typed_views` / `NeutrinoTestTypedViews` | `examples/test_typed_views.cpp` | ListView/TreeView typed string interpretation. |
| `neutrino_test_smooth_graphics` / `NeutrinoTestSmoothGraphics` | `examples/test_smooth_graphics.cpp` | Smooth graphics, antialiasing, and rendering options. |
| `neutrino_test_treeview_collapse` / `NeutrinoTestTreeviewCollapse` | `examples/test_treeview_collapse.cpp` | Existing TreeView collapse/expand demo. |
| `neutrino_test_heavy_data` / `NeutrinoTestHeavyData` | `examples/test_heavy_data.cpp` | Existing large-data auto-scroll demo. |
| `neutrino_test_01_buttons_icons` / `NeutrinoTest01ButtonsIcons` | `examples/neutrino_test_01_buttons_icons.cpp` | `Neu_Button`, `Neu_FlatButton`, `Neu_MenuItem`, BMP icons, nested placement, click/focus/blur callbacks. |
| `neutrino_test_02_text_inputs` / `NeutrinoTest02TextInputs` | `examples/neutrino_test_02_text_inputs.cpp` | `Neu_Textbox`, `Neu_Passwordbox`, `Neu_Multilinetextbox`, labels, text-change callbacks, editing. |
| `neutrino_test_03_lists_combo_autoscroll` / `NeutrinoTest03ListsComboAutoScroll` | `examples/neutrino_test_03_lists_combo_autoscroll.cpp` | `Neu_Listbox`, `Neu_ComboBox`, heavy item lists, auto-scroll and selection callbacks. |
| `neutrino_test_04_listview_typed_data` / `NeutrinoTest04ListViewTypedData` | `examples/neutrino_test_04_listview_typed_data.cpp` | `Neu_ListView`, `Neu_StringTable`, typed string interpretation for numbers, booleans, binary, hex, enums, UTF, and image paths. |
| `neutrino_test_05_treeview_collapse` / `NeutrinoTest05TreeViewCollapse` | `examples/neutrino_test_05_treeview_collapse.cpp` | `Neu_TreeView`, STL path model, collapse/expand buttons, path selection. |
| `neutrino_test_06_placement_layout_scaling` / `NeutrinoTest06PlacementLayoutScaling` | `examples/neutrino_test_06_placement_layout_scaling.cpp` | `Neu_Placement`, fixed left/top/width/height layout, scaling factor, max-size constraints, nested controls. |
| `neutrino_test_07_popup_menu_categories` / `NeutrinoTest07PopupMenuCategories` | `examples/neutrino_test_07_popup_menu_categories.cpp` | `Neu_PopWindowMenu`, left-side categories, right-side menu items, menu-item icon. |
| `neutrino_test_08_richtext_code` / `NeutrinoTest08RichTextCode` | `examples/neutrino_test_08_richtext_code.cpp` | `Neu_RichTextCode`, source-code text, language name, line-oriented rendering, scrolling. |
| `neutrino_test_09_readonly_richtext_icons` / `NeutrinoTest09ReadOnlyRichTextIcons` | `examples/neutrino_test_09_readonly_richtext_icons.cpp` | `Neu_ReadOnlyRichText`, label/multiline-label subcomponents, STL icon list, `#` count icon selection, escaped `\#`. |
| `neutrino_test_10_images_progress_labels` / `NeutrinoTest10ImagesProgressLabels` | `examples/neutrino_test_10_images_progress_labels.cpp` | `Neu_ImageView`, `Neu_ProgressSquare`, `Neu_Label`, `Neu_MultilineLabel`, BMP icons, progress updates. |
| `neutrino_test_11_rendering_buffering` / `NeutrinoTest11RenderingBuffering` | `examples/neutrino_test_11_rendering_buffering.cpp` | Shadows, hover hints, long hint popup behavior, smooth graphics options, multi-stage double buffering toggle. |
| `neutrino_test_12_scroll_windows_heavy_data` / `NeutrinoTest12ScrollWindowsHeavyData` | `examples/neutrino_test_12_scroll_windows_heavy_data.cpp` | `Neu_ScrollWindow`, standalone `Neu_ScrollBar`, heavy `Neu_ListView`, heavy `Neu_Listbox`, vertical/horizontal scroll metadata. |
| `neutrino_test_13_swing_swt_controls` / `NeutrinoTest13SwingSwtControls` | `examples/neutrino_test_13_swing_swt_controls.cpp` | Stage2 overview: Swing/SWT-inspired controls, theme selector, label offsets, tab/splitter/toolbar/group controls. |
| `neutrino_test_14_label_offsets_themes` / `NeutrinoTest14LabelOffsetsThemes` | `examples/neutrino_test_14_label_offsets_themes.cpp` | Stage2 label text offsets/insets, optional borders, single-line truncation, multiline wrapping, rich fragments, and 24+ built-in themes. |
| `neutrino_test_15_selection_controls` / `NeutrinoTest15SelectionControls` | `examples/neutrino_test_15_selection_controls.cpp` | `Neu_CheckBox`, `Neu_RadioButton`, `Neu_ToggleButton`, `Neu_GroupBox`, `Neu_Separator`, `Neu_LinkLabel`, and callback-driven radio behavior. |
| `neutrino_test_16_value_controls` / `NeutrinoTest16ValueControls` | `examples/neutrino_test_16_value_controls.cpp` | `Neu_ProgressBar`, `Neu_ProgressSquare`, horizontal/vertical `Neu_Slider`, `Neu_Spinner`, and value-control callbacks. |
| `neutrino_test_17_tabs_toolbar_splitter` / `NeutrinoTest17TabsToolbarSplitter` | `examples/neutrino_test_17_tabs_toolbar_splitter.cpp` | `Neu_ToolBar`, `Neu_TabView`, `Neu_Splitter`, fixed-position tab pages, icons, and nested controls. |
| `neutrino_test_18_stage2_scroll_controls` / `NeutrinoTest18Stage2ScrollControls` | `examples/neutrino_test_18_stage2_scroll_controls.cpp` | `Neu_ScrollWindow` containing Stage2 controls, clipping, scrolling and nested child drawing on Linux/Windows. |
| `neutrino_test_19_material_beauty` / `NeutrinoTest19MaterialBeauty` | `examples/neutrino_test_19_material_beauty.cpp` | Stage2 beauty rendering: MaterialDark default, gradient surfaces, highlight/focus colors, edge/rounded corner switching, and DAA/MSAA/SSAA theme antialiasing. |

## Windows-only demo

| VS2022 project | Source | Coverage |
|---|---|---|
| `NeutrinoWin32Demo` | `examples/windows/neutrino_win32_demo.cpp` | Native Win32/GDI backend smoke demo using Windows subsystem entry points. |

## Linux build commands

```sh
make
cmake -S . -B cmake-build
cmake --build cmake-build
./autogen.sh
./configure
make -f Makefile.autoconf
```

## Windows build commands

Open `msvc/Neutrino.sln` in Visual Studio 2022 and build the solution. All VS2022 projects explicitly use the MSVC C++17 setting `stdcpp17` and define `_CRT_SECURE_NO_WARNINGS` for both Debug and Release x64 builds. You can also generate a VS2022 CMake build:

```bat
cmake -S . -B build-vs2022 -G "Visual Studio 17 2022" -A x64
cmake --build build-vs2022 --config Release
```

## Visual Studio project settings

Every `msvc/*.vcxproj` file uses Visual Studio 2022 toolset `v143`, C++17 mode via `<LanguageStandard>stdcpp17</LanguageStandard>`, Unicode character set, `/utf-8`, and `_CRT_SECURE_NO_WARNINGS` in both Debug and Release configurations.
## Stage2 text selection and clipboard fix

Editable text controls now preserve repeated spaces/indentation and support `Ctrl+A`, `Ctrl+C`, `Ctrl+X`, `Ctrl+V`, Shift+arrow selection, and selection-aware Backspace/Delete. See `STAGE2_TEXT_SELECTION_CLIPBOARD_FIXES.md`.



## Stage2 rich text focused tests

- `neutrino_test_20_full_richtext_control`: full-window rich-text editing, toolbar formatting, selection, clipboard, drag selection, and undo/redo.
- `neutrino_test_21_full_richtext_code_control`: full-window code-oriented rich text editor with toolbar formatting, code text, selection, clipboard, and undo/redo.


## Stage2 text navigation and selection update

See `STAGE2_TEXT_NAV_SELECTION_FIXES.md` for the multiline/rich-text cursor movement and selection highlight fix pass.
