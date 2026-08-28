# Linux/X11 Text Rendering Fix

## Problem

On some Linux/X11 systems the Stage2 test applications opened and drew controls/widgets, but all labels, button captions, text boxes, list rows, tree rows, table cells and rich-text content were invisible.

## Root cause

The Linux renderer could enter the Xft text path when `pkg-config xft` was available. On some X11/XWayland/VM servers, `XftFontOpenName()` succeeds and the framework assumes the text was drawn, but the Xft draw does not visibly land on the redirected/multi-stage backing pixmap. The framework then returned early and skipped the plain Xlib text fallback, leaving all controls visible but text blank.

There was also no explicit core X11 font selected for the shared GC, so the few remaining low-level `XDrawString()` helper paths were not fully portable across X servers.

## Fix

- `Neu_Control::drawTextColored()` now uses a safer text pipeline.
- Xft is still attempted when available and the RENDER extension is present.
- A core X11/Xutf8 fallback pass is enabled by default so text is visible on servers where Xft silently fails on a pixmap/backbuffer.
- `Neu_Control::measureTextWidth()` now falls back to real core-font measurement instead of only using an approximate width.
- `Neu_Application::open()` initializes the process locale and X locale modifiers for Xutf8 text drawing.
- `Neu_Window::create()` installs the built-in `fixed` font into the GC so any direct `XDrawString()` helper still works.

## Runtime controls

Default behavior favors visible text everywhere.

- Force core X11 text only:

  ```sh
  NEUTRINO_X11_TEXT_BACKEND=core ./build/neutrino_demo
  ```

- Keep Xft only and disable the safety fallback:

  ```sh
  NEUTRINO_X11_TEXT_CORE_FALLBACK=0 ./build/neutrino_demo
  ```

- Disable Xft without changing build options:

  ```sh
  NEUTRINO_DISABLE_XFT=1 ./build/neutrino_demo
  ```

## Verification performed

Built successfully on Linux/X11 with:

```sh
make -j2
```

Representative apps were run under Xvfb. The demo now shows both widgets and text, including text boxes, password box, multi-line text, button captions, list items, tree/list headers, table rows and popup-menu text.
