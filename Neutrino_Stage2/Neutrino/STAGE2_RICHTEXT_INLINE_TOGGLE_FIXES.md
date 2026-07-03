# Stage2 Rich Text Inline Selection and Toggle Formatting Fixes

This fix pass addresses the remaining rich text and rich text code editing issues reported on Windows and Linux.

## Fixes

- Rich text styled fragments are now drawn inline on the same logical line unless the underlying text actually contains a newline.
- Mouse hit-testing for rich text and rich text code now walks the same styled line model used by the renderer, including per-line style height, so selection no longer drifts after the first few lines.
- Selection highlight rectangles are drawn against the actual styled-line top/bottom rather than an older fixed-line coordinate.
- Toolbar formatting is now a true toggle:
  - clicking Bold twice removes bold from the target range;
  - clicking Italic twice removes italic;
  - underline, strike, double-strike, heading, monospace, font family, color/background/highlight styles also toggle when the same style is already present.
- Formatting applies to the selected text if a selection exists, otherwise to the current word under or immediately before the caret.
- Formatting no longer makes the entire control bold/italic/etc. when no text is selected.
- Nested range behavior is supported through fragment splitting. For example, if a small bold word is inside a larger selection and Bold is applied to the larger selection, the inner word toggles back to normal while the outer range becomes bold.
- Applying formatting no longer creates a visual line break simply because a styled fragment boundary was created.
- MaterialDark toolbar icons are drawn with black text on the light toolbar strip for better visibility.

## Files touched

- `src/Neu_RichTextCode.cpp`
- `src/win32/Neu_Win32.cpp`
