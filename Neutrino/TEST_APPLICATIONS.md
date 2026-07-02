# Neutrino test applications

The package includes 24 cross-platform Linux/CMake/Makefile test and demo applications plus a native Win32 Visual Studio demo. The Visual Studio 2022 solution under `msvc/Neutrino.sln` contains projects for each listed example.

## Focused feature tests

- `neutrino_test_01_buttons_icons` - buttons, flat buttons, menu items, and BMP icons.
- `neutrino_test_02_text_inputs` - text box, password box, multiline text input, cursor/input routing.
- `neutrino_test_03_lists_combo_autoscroll` - list box, combo box, large datasets, and autoscroll.
- `neutrino_test_04_listview_typed_data` - ListView typed data interpretation and column truncation.
- `neutrino_test_05_treeview_collapse` - TreeView expand/collapse, selection, and hover highlighting.
- `neutrino_test_06_placement_layout_scaling` - placement container, fixed layout, scaling, and maximum sizes.
- `neutrino_test_07_popup_menu_categories` - pop-window menu categories and menu item views.
- `neutrino_test_08_richtext_code` - code-oriented rich text editing, toolbar, scrollbars, and word wrap.
- `neutrino_test_09_readonly_richtext_icons` - read-only rich text, label subcomponents, and icon selection by `#` count.
- `neutrino_test_10_images_progress_labels` - image view, progress square, labels, multiline labels, and icon support.
- `neutrino_test_11_rendering_buffering` - smooth rendering, hint bounds, shadowing, and multi-stage buffering.
- `neutrino_test_12_scroll_windows_heavy_data` - scroll window clipping and large-data scroll behavior.

## Regression tests added for this revision

- `test_progress_square` - progress starts at top-center, moves clockwise around all edges, and completes at top-center.
- `test_wordwrap_hints` - labels, multiline labels, hints, wrapping, and truncation stay inside bounds.
- `test_richtext_formatting` - rich text fragments: bold, italic, underline, strikethrough, double strikethrough, headings 1-7, monospaced text, font/color/background/highlight selection, and alignment.
- `test_readonly_richtext_spacing` - read-only rich text label spacing, line spacing, and `no_crlf()` same-line append behavior.
- `test_scroll_clip_select` - ListView/TreeView selection and hover, scrollbar selection/dragging, scroll-window clipping, and column truncation.

## Existing broad demos

- `demo`
- `test_all_controls`
- `test_heavy_data`
- `test_smooth_graphics`
- `test_treeview_collapse`
- `test_typed_views`
- `test_windows`

Run from the project root so `assets/icons/*.bmp` resolves correctly.
