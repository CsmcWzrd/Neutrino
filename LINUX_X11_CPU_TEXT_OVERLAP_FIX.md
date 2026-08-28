# Linux/X11 CPU and Duplicate Text Fix

This update fixes two Linux/X11 regressions reported after the earlier text-rendering and responsiveness fixes:

1. Very high CPU usage while the GUI is apparently idle.
2. Text appearing overlapped or drawn multiple times with a tiny offset.

## Root causes

### Duplicate text

The earlier X11 text visibility fallback drew Xft text first and then also drew the core X11/Xutf8 fallback text by default. On systems where Xft is actually visible, every label/button/editor string was therefore rendered twice.

The Linux renderer now uses exactly one text backend by default:

- Default: `core` / Xutf8 core text, safe and low overhead.
- Optional: `NEUTRINO_X11_TEXT_BACKEND=xft` for Xft-only antialiased text.
- Optional: `NEUTRINO_X11_TEXT_BACKEND=auto` to try Xft and fall back only when Xft cannot draw.
- Optional diagnostic mode: `NEUTRINO_X11_TEXT_BACKEND=dual` or `fallback` to intentionally draw both paths.

The old always-on fallback behavior has been removed.

### High CPU redraw loop

Some controls can call `requestRedraw()` while the window is already painting. Keeping those requests as a new dirty frame can make the event loop repaint continuously even when there is no user input. `Neu_Window::redraw()` now treats paint-time redraw requests as invalid renderer side effects and does not re-dirty the window after the frame completes.

User-driven redraws still work normally from events, expose, resize, focus changes, scrolling, and explicit application state changes.

## Verification

Built with:

```sh
make -j2
```

Idle CPU checks under Xvfb with `NEUTRINO_USE_X11=1 NEUTRINO_FAST_RENDER=1`:

```text
neutrino_demo idle_cpu_ticks_2s=0
neutrino_test_03_lists_combo_autoscroll idle_cpu_ticks_2s=0
neutrino_test_12_scroll_windows_heavy_data idle_cpu_ticks_2s=0
```
