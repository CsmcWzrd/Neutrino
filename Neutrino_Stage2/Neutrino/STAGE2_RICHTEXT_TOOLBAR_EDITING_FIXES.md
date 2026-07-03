# Stage2 Rich Text Toolbar, Selection, Undo/Redo, Drag Editing, and Wayland Build Preference Fixes

This Stage2 update makes the rich text/code toolbar functional and expands the editable text-control input model.

## Functional rich text toolbar

`Neu_RichTextCode` toolbar buttons now modify the selected text span, or the whole text if no selection is active. The toolbar uses Unicode icons/characters and supports:

- bold
- italic
- underline
- strikethrough
- double strikethrough
- Heading 1 and Heading 2 toolbar buttons, plus public `applyHeading(1..7)` API
- monospaced formatting
- font family cycling: Sans, Serif, SansSerif, Monospace
- font color selection
- background color selection
- sketch/highlight color selection
- left, center, and right alignment
- word-wrap toggle

Public helper methods include:

```cpp
rich->applyBold();
rich->applyItalic();
rich->applyUnderline();
rich->applyStrikethrough();
rich->applyDoubleStrikethrough();
rich->applyHeading(1);
rich->applyMonospace();
rich->cycleToolbarFont();
rich->applyFontColor({144, 202, 249, 255});
rich->applyBackgroundColor({36, 46, 62, 255});
rich->applyHighlightColor({255, 240, 120, 160});
```

## Text selection and editing keys

Editable text controls now support:

- Shift+Home
- Shift+End
- Shift+PageUp
- Shift+PageDown
- Home
- End
- PageUp
- PageDown
- Insert overwrite toggle
- mouse drag selection
- same-control selected-text drag/drop movement

Existing clipboard and undo/redo bindings remain active:

- Ctrl+A select all
- Ctrl+C copy
- Ctrl+X cut
- Ctrl+V paste
- Ctrl+Z undo
- Ctrl+Shift+Z redo
- Ctrl+Y redo
- Alt+Backspace redo

The support applies to `Neu_Textbox`, `Neu_Passwordbox`, `Neu_Multilinetextbox`, and `Neu_RichTextCode`.

## Wayland build preference

Linux Makefile, CMake, and autoconf paths prefer the Wayland-capable build path when `wayland-client` metadata is available and `NEUTRINO_USE_X11` is not set to `1`. Explicitly setting `NEUTRINO_USE_X11=0` keeps Wayland preference enabled:

```sh
NEUTRINO_USE_X11=0 make
NEUTRINO_USE_X11=0 cmake -S . -B build-wayland
NEUTRINO_USE_X11=0 ./configure
```

Set `NEUTRINO_USE_X11=1` to force X11-only build/runtime preference:

```sh
NEUTRINO_USE_X11=1 make
NEUTRINO_USE_X11=1 ./build/neutrino_demo
```

The current Stage2 renderer remains compatible with the existing X11/XWayland drawing path while reporting the Wayland session path through:

```cpp
neutrino::Neu_SelectedBackendName();
neutrino::Neu_IsWaylandBackendSelected();
```
