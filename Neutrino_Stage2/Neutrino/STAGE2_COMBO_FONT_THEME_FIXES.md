# Stage2 Combo, Font, and Theme Geometry Fixes

This fix pass addresses remaining Windows-first issues reported on 2026-07-03:

- `Neu_ComboBox` now opens a real drop-down list instead of only cycling through values.
- ComboBox drop-down lists include a scrollbar when the item count exceeds the visible row count.
- ComboBox drop-down rows use the active theme's corner geometry for selected and hovered rows.
- Added logical font-family API support:
  - `Neu_FontFamily::Sans`
  - `Neu_FontFamily::Serif`
  - `Neu_FontFamily::SansSerif`
  - `Neu_FontFamily::Monospace`
  - `Neu_FontFamilyName(...)`
  - `Neu_Theme::setFontFamily(...)`
- MaterialDark default text color is now an off-white value `RGB(232,234,237)`.
- Win32 `Neu_RichTextCode` caret computation now treats CRLF and LF as logical line breaks when calculating the caret after end-of-line deletion.
- ListView/TableView and TreeView header bars now use the active theme-shaped rectangle path instead of a plain rectangular fill.
- ListView/TableView, TreeView, and ComboBox selection/hover highlights use active theme corner geometry.

The package remains proprietary and includes `PROPRIETARY_NOTICE.txt`.
