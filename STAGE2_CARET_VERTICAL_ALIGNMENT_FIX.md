# Stage2 caret vertical alignment fix

This update adjusts the Win32 editable-control caret placement so the insert cursor is vertically aligned with the actual rendered font metrics instead of fixed hard-coded offsets.

Affected controls:

- `Neu_Textbox`
- `Neu_Passwordbox` through the shared textbox drawing path
- `Neu_Multilinetextbox`
- `Neu_RichTextCode`

Changes:

- Added Win32 font metric helper `uiFontPixelHeightWin32(...)`.
- Added centered single-line text helper `centeredTextTopWin32(...)`.
- Single-line textboxes now center text using the active font height.
- Caret top/bottom now use the same text top and font height used for drawing.
- Multiline and rich-text carets no longer start above the text row.

The fix is limited to caret/text vertical alignment and does not change layout sizing or event routing.
