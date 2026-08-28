# Stage2 Text Selection / Clipboard / Whitespace Fixes

This Stage2 fix pass updates editable text handling across Neutrino controls.

## Fixed whitespace preservation

The shared text wrapping logic no longer tokenizes through `std::istringstream`, which previously collapsed repeated spaces and indentation. Wrapping now preserves:

- ordinary spaces
- leading indentation
- repeated spaces inside rich text fragments
- spaces in multiline text boxes
- spaces in multiline labels
- spaces in read-only rich text labels
- spaces in RichTextCode/code-editor text

This affects both Linux/X11 and Win32/GDI backends.

## Added editable text selection API

`Neu_Textbox` and controls derived from it now expose:

```cpp
selectAll();
clearSelection();
setSelection(start, end);
hasSelection();
selectedText();
selectionStart();
selectionEnd();
```

These APIs are inherited by:

- `Neu_Textbox`
- `Neu_Passwordbox`
- `Neu_Multilinetextbox`
- `Neu_RichTextCode`
- alias controls based on these editable text classes

## Added keyboard selection and clipboard shortcuts

Editable text controls now support:

- `Ctrl+A` select all
- `Ctrl+C` copy selection
- `Ctrl+X` cut selection
- `Ctrl+V` paste clipboard text
- `Shift+Left` / `Shift+Right` selection extension
- `Shift+Home` / `Shift+End` selection extension
- selection-aware Backspace/Delete
- selection-aware typed character insertion
- selection-aware newline insertion in multiline controls

On Windows the shortcuts use the native Unicode clipboard. On Linux/X11 the framework currently provides an internal Neutrino clipboard for cross-control copy/cut/paste inside the running application.
