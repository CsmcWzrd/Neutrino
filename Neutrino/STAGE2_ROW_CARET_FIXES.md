# Stage2 Row and RichTextCode Caret Fixes

This fix pass addresses two reported remaining rendering/input issues.

## ListView / TableView / TreeView row text clipping

- Increased the row heights used by the list/table/tree body rows.
- Replaced baseline-style row drawing with top-aligned text placement computed from the actual visible cell rectangle.
- On Win32, text now uses `uiFontPixelHeightWin32()` to vertically center inside each clipped cell.
- On Linux/X11, the body rows now use a taller row height and a safer text Y offset so text is not clipped at the bottom.
- Row selection and hover rectangles continue to use the active theme geometry.

## RichTextCode Backspace caret normalization

- RichTextCode now handles Backspace directly instead of relying on the single-character Textbox deletion path.
- CRLF (`\r\n`) is deleted as one logical newline when the caret is immediately after the newline pair.
- The caret is clamped back to the actual post-delete byte offset.
- Horizontal scrolling is reset when the caret lands at the start of the previous line.
- The same newline-aware behavior was added to the Linux/X11 and Win32 paths.
