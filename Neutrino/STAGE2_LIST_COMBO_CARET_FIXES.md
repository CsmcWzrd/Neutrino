# Stage2 List/Combo/Caret Fixes

This fix pass addresses remaining Windows and Linux GUI issues reported on July 3, 2026.

## Fixes

- Increased ListView/TableView and TreeView row height to prevent bottom clipping of text.
- Adjusted Win32 ListView/TableView cell text baseline so rendered text no longer clips at the bottom.
- Routed mouse move, click, release, and wheel events to an already-open ComboBox dropdown even when the pointer is outside the collapsed ComboBox rectangle.
- Kept ComboBox dropdown scrollbars active while the dropdown is open.
- Preserved active-theme shaped selection/highlight drawing for ComboBox, ListView/TableView, and TreeView rows.
- Normalized Win32 RichTextCode caret position after backspacing at line ends and after deleting the final character in a line.
- Fixed a Win32 Textbox UTF-8 insertion cursor advancement bug.
- Updated examples to use MaterialDark by default, except examples whose purpose is to switch themes interactively.

## Notes

The package remains proprietary and includes `PROPRIETARY_NOTICE.txt`.
