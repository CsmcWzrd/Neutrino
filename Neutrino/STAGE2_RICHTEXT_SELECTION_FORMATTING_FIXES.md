# Stage2 Rich Text Selection and Formatting Fixes

This fix pass addresses two remaining rich text editing issues reported in the Windows build and applies the same behavior to the Linux/X11 code path.

## Fixes

- Rich text selection highlight rectangles now draw on the same visual line as the selected text.
- Win32 rich text/code selection rectangles were using top-oriented text coordinates as if they were baseline coordinates; the selection fill is now drawn from the current row top to the current row bottom.
- Toolbar formatting no longer applies to the whole control when there is no active selection.
- Toolbar formatting target behavior is now:
  1. apply to the selected text range;
  2. if no text is selected, apply to the current word under/adjacent to the caret;
  3. if the caret is not on a word, do nothing.
- This behavior applies to bold, italic, underline, strikethrough, double strikethrough, headings, monospace, font selection, font color, background color, and sketch/highlight color.

## Affected files

- `src/Neu_RichTextCode.cpp`
- `src/win32/Neu_Win32.cpp`

## Notes

The selection and formatting behavior remains compatible with the existing function-pointer callback model and the existing fixed-position layout system.
