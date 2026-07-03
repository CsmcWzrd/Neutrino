# Neutrino Stage2 Notes

Stage2 is built on top of the Stage1 checkpoint and keeps the original fixed-position layout model: top-left coordinate, width, height, scale, and maximum size metadata. No new layouting scheme was introduced.

## Label text offsets

Labels and multiline labels support explicit text offsets/insets to keep text inside the control boundary on Windows and Linux:

```cpp
label->setTextOffset(top, right, bottom, left);
label->setTextInsets(left, top, right, bottom);
```

The Windows label drawing path now clips to the inset rectangle so text cannot protrude outside the left edge.

## Additional built-in themes

Stage2 adds 24+ built-in themes and matching theme asset files, including Win95, WinXP, Win10, Win11, ClassicMotif, Solarized Light/Dark, Nord, Dracula, Gruvbox Light/Dark, high-contrast themes, Ubuntu Aubergine, KDE Breeze, Mac Aqua, Material Light/Dark, Ocean, Forest, Rose, Amber, Slate, Candy, Terminal Green, and Corporate Blue.

Use:

```cpp
window.setTheme(Neu_Theme::BuiltInThemeByName("Win11"));
```

or list them:

```cpp
auto names = Neu_Theme::BuiltInThemeNames();
```

## Swing/SWT-inspired Stage2 controls

Stage2 adds or aliases common desktop controls/features while retaining the existing event callback model and fixed layout:

- `Neu_CheckBox`
- `Neu_RadioButton`
- `Neu_ToggleButton`
- `Neu_ProgressBar`
- `Neu_Slider`
- `Neu_Spinner`
- `Neu_GroupBox`
- `Neu_Separator`
- `Neu_LinkLabel`
- `Neu_ToolBar`
- `Neu_TabView`
- `Neu_Splitter`
- `Neu_TableView` alias for `Neu_ListView`
- `Neu_TextArea` alias for `Neu_Multilinetextbox`
- `Neu_Panel` / `Neu_Composite` aliases for `Neu_Placement`

Focused Stage2 demos are:

- `neutrino_test_13_swing_swt_controls` - overview of all Stage2 controls/features.
- `neutrino_test_14_label_offsets_themes` - label offsets/insets and 24+ themes.
- `neutrino_test_15_selection_controls` - checkbox, radio, toggle, group, separator and link controls.
- `neutrino_test_16_value_controls` - progress bar, progress square, sliders and spinner.
- `neutrino_test_17_tabs_toolbar_splitter` - toolbar, tabs and splitter/sash controls.
- `neutrino_test_18_stage2_scroll_controls` - scroll window clipping with nested Stage2 controls.

Each of these is wired into Linux Make/CMake/autoconf and the Visual Studio 2022 solution.
