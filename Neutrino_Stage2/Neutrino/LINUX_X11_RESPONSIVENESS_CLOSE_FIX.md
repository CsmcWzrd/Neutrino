# Linux/X11 Responsiveness and Close Fix

This update fixes two Linux/X11 runtime problems reported after the blank-screen and text-rendering fixes:

1. The GUI felt extremely unresponsive, as if the event loop or repaint logic was spinning.
2. The Linux window-manager close button did not reliably close the application.

## Root causes

### 1. Synchronous repaint from `requestRedraw()`

`Neu_Window::requestRedraw()` called `invalidate()`, and `invalidate()` immediately called `redraw()`. That meant any control event, hover state change, focus change, scroll update, or setter could repaint the entire window synchronously while the X11 event handler was still running.

This was especially expensive after the Stage2 rendering updates because one full repaint can include shadows, rounded rectangles, text fallback, list/table/tree rows, and rich-text content. A cluster of X11 motion/configure/expose events could therefore make the GUI appear unusable.

### 2. Redraw requests during paint were not coalesced

Some controls compute internal display state during paint. Even when individual setters avoid redundant redraws, this design needs protection against recursive redraw requests. The previous Linux backend had no paint guard/coalescing layer.

### 3. XRender library unloaded too early

`detectXRender()` used `dlopen()` to load `libXrender`, called `XRenderQueryExtension()`, then immediately called `dlclose()`.

`XRenderQueryExtension()` can register Xlib extension callbacks on the current `Display`. If `libXrender` is unloaded before `XCloseDisplay()`, Xlib may later call an extension close hook whose function pointer points into unloaded memory. That caused crashes or broken close behavior on Linux.

## Fixes applied

- Converted Linux `requestRedraw()` into a coalesced dirty-flag invalidation.
- Added `Neu_Window::hasPendingRedraw()` and `Neu_Window::flushPendingRedraw()`.
- Reworked `Neu_Application::run()` to:
  - drain all pending X11 events,
  - repaint each dirty window once,
  - block in `XNextEvent()` only when there are no pending events and no dirty windows.
- Added a paint recursion guard using `painting_` and `redrawRequestedDuringPaint_`.
- Made event dispatch safe when a window closes while an event is being handled.
- Added `MapNotify`, `UnmapNotify`, and `DestroyNotify` handling.
- Kept `libXrender` loaded until after `XCloseDisplay()`.
- Stored and freed the fallback core X11 font instead of leaking the `XFontStruct` returned by `XLoadQueryFont()`.
- Flushed the X11 connection after destroying a window.

## Verification performed

Built with:

```sh
make -j2
```

Runtime tested under Xvfb with `NEUTRINO_USE_X11=1` and `NEUTRINO_FAST_RENDER=1`:

- `neutrino_demo`
- `neutrino_test_03_lists_combo_autoscroll`
- `neutrino_test_12_scroll_windows_heavy_data`

Observed idle CPU ticks over 2 seconds:

```text
neutrino_demo idle_cpu_ticks_2s=0
neutrino_test_03_lists_combo_autoscroll idle_cpu_ticks_2s=0
neutrino_test_12_scroll_windows_heavy_data idle_cpu_ticks_2s=0
```

Window-manager close was tested by sending `WM_DELETE_WINDOW` to the X11 top-level window. All tested applications exited normally.
