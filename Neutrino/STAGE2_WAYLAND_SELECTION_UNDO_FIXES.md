# Stage2 Wayland, Selection, Undo/Redo, and Unicode Toolbar Fixes

This Stage2 pass updates the Linux backend-selection path and text editing controls.

## Backend selection

Neutrino now defaults to the Wayland session path when a Wayland compositor is available. Set the environment variable below to force the X11 path:

```sh
NEUTRINO_USE_X11=1 ./build/neutrino_demo
```

The current Linux implementation uses the existing X11 renderer through the Wayland/XWayland bridge when running in a Wayland session. This keeps the existing drawing and event code stable while making Wayland the default session choice. Runtime helpers are available:

```cpp
const char* name = neutrino::Neu_SelectedBackendName();
bool wayland = neutrino::Neu_IsWaylandBackendSelected();
```

## Text selection highlight

Editable text controls now draw a visible highlight rectangle behind selected text. This applies to:

- `Neu_Textbox`
- `Neu_Passwordbox`
- `Neu_Multilinetextbox`
- `Neu_RichTextCode`

The highlight uses the active theme's `highlight` color.

## Undo and redo

Editable controls use a shared undo/redo snapshot stack. Supported shortcuts:

- `Ctrl+Z` undo
- `Ctrl+Shift+Z` redo
- `Ctrl+Y` redo
- `Alt+Backspace` redo

The custom RichTextCode newline/backspace path now records undo snapshots before text mutation, so line-join deletions are undoable.

## Unicode toolbar symbols

The RichTextCode toolbar now uses Unicode symbols instead of plain ASCII labels:

- bold: `𝐁`
- italic: `𝐼`
- underline: `U̲`
- strike: `S̶`
- heading: `H₁`, `H₂`
- monospaced: `⌨`
- alignment: `⇤`, `↔`, `⇥`
- wrap: `↩`

