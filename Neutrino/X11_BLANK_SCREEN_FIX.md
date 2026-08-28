# X11 Blank Screen Fix

This update fixes the Linux/X11 test applications opening as a blank dark window.

## Root cause

Two problems combined to create the blank-screen symptom:

1. Several controls call `setAutoScroll(true)` from inside their `draw()` methods. The old setter always called `requestRedraw()`, even when the value was already true. Since `requestRedraw()` repaints immediately, painting a scroll-capable control recursively started another paint before the current paint reached the final `XCopyArea()`/`XFlush()` presentation step.
2. The software anti-aliasing and shadow paths called `XAllocColor()` for every generated pixel through `Neu_Pixel()`. On TrueColor X11 visuals this is unnecessary and extremely slow, especially on Xvfb, VMs, XWayland, and remote X11 sessions.

The result was a mapped window whose X11 background color was visible, while the control frame was never presented in time.

## Changes

- `Neu_Control::setAutoScroll()` now invalidates only when the value changes.
- `Neu_Control::setScrollOffset()` now invalidates and sends `onScroll` only when the actual clamped scroll position changes.
- `Neu_Pixel()` now packs RGB values directly for TrueColor/DirectColor visuals using the X11 visual masks.
- `Neu_Pixel()` keeps a small fallback cache for indexed/colormap visuals so repeated colors do not allocate repeatedly.
- `Neu_PixelToColor()` now decodes TrueColor/DirectColor visuals directly.

## Verification performed

The project was built on Linux with the supplied Makefile. Representative applications were run under Xvfb and sampled from the X11 window pixels after the first paint:

- `build/neutrino_demo`: 152 unique colors
- `build/neutrino_test_01_buttons_icons`: 85 unique colors
- `build/neutrino_test_11_rendering_buffering`: 88 unique colors
- `build/neutrino_test_19_material_beauty`: 121 unique colors
- `build/neutrino_test_21_full_richtext_code_control`: 74 unique colors

Before the fix, `build/neutrino_demo` sampled as one color only, the dark window background.

## Build

```sh
make clean
make -j$(nproc)
NEUTRINO_USE_X11=1 ./build/neutrino_demo
```

