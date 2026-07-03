# Stage2 TreeView / RichTextCode Fixes

This fix pass addresses two remaining Stage2 issues reported on Windows and keeps the Linux/X11 code path aligned.

## TreeView hover and row hit-testing

The Win32 TreeView renderer was drawing with the newer Stage2 row metrics, but the mouse hit-test path still used older header and row heights. This caused the highlighted row to appear one or two rows below the pointer. The Win32 hit-test path now uses the same constants as the draw path:

- `kTreeHeaderHeightWin32`
- `kTreeRowHeightWin32`

The Linux/X11 TreeView hit-test math was also aligned to the TreeView viewport origin instead of the raw control top value.

## RichTextCode Backspace caret normalization

Backspacing across a line boundary now normalizes the caret after deleting `\n` or `\r\n` as one logical newline. The code also recomputes the current line prefix width and resets horizontal scrolling when the caret is on a line that fits in the viewport. This prevents the caret from carrying a stale horizontal scroll offset after joining lines.

