# Stage2 Text Navigation and Selection Fixes

This fix pass addresses multiline and rich-text editing behavior in the Neutrino Stage2 codebase.

## Fixed

- `Neu_Multilinetextbox` now handles line-aware cursor movement:
  - Up arrow
  - Down arrow
  - Home / End within the current logical line
  - Shift + Up / Down / Home / End selection extension
  - Shift + PageUp / PageDown selection extension
- `Neu_RichTextCode` now benefits from the same line-aware movement path.
- Selection highlight rectangles are now painted for wrapped and unwrapped multiline text paths.
- `Neu_RichTextCode` now paints selection highlights in both plain-code and rich-fragment modes.
- `Neu_RichTextCode` now keeps caret painting active even after toolbar formatting creates rich-text fragments.
- Win32 rich-code mouse drag selection now uses the rich-code content origin instead of falling through to multiline-textbox coordinates.
- Win32 multiline text controls now support Up/Down cursor movement with Shift-selection.

## Notes

- The selection model remains byte-offset based internally, as in earlier Stage2 builds.
- Linux/X11 and Win32 paths were both patched.
- No new layout scheme was introduced.
