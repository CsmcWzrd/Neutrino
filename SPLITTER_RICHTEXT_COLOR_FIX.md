# Splitter Minimum Pane / Clipping and Rich Text Color Fix

## Summary

This update fixes the horizontal and vertical `Neu_Splitter` behavior and improves the default text colors used by `Neu_RichTextCode` / rich text editor demos on dark themes.

## Splitter fixes

`Neu_Splitter` now supports configurable minimum pane size and sash size:

```cpp
splitter->setMinimumPaneSize(140);
splitter->setMinimumPaneWidth(140);   // alias for vertical splitters
splitter->setMinimumPaneHeight(42);   // alias for horizontal splitters
splitter->setSashSize(8);
```

The split position is clamped so that neither pane collapses below the configured minimum size. The clamp is applied while drawing and while dragging.

### X11 clipping

The X11 backend previously relied on the shared GC clip rectangle before calling each child control. Some child controls reset the GC clip internally while rendering text, scrollbars, or hint content. That could allow a control to overdraw past the splitter sash.

The X11 splitter now renders each child into a scratch pixmap, then copies back only the clipped pane rectangle. This keeps children clipped even when the splitter moves across the child control's start position or through the middle of the child.

### Win32 clipping

The Win32 splitter now uses the same pane assignment and minimum-size clamp logic as X11. It intersects the HDC clip region with the correct pane rectangle before drawing each child.

## Rich text color fixes

`Neu_RichTextCode` previously used a hard-coded dark default font color. On the default Material Dark theme this made rich text/code editor text difficult to see unless every demo manually overrode the font color.

The control now follows `theme.text` by default. Calling `setDefaultFontColor()` still works and marks the color as an explicit override. `clearDefaultFontColorOverride()` restores automatic theme text color.

Line numbers are now drawn with a dark gutter text color so they remain visible on the light gutter background.

The code editor demo uses the window theme's text and highlight colors instead of hard-coded low-contrast colors.

## Verification

Linux/X11 verified with:

```sh
make -j2
cmake -S . -B cmake-build
cmake --build cmake-build -j2
DISPLAY=:65 NEUTRINO_USE_X11=1 NEUTRINO_FAST_RENDER=1 ./build/neutrino_test_17_tabs_toolbar_splitter --fast
DISPLAY=:64 NEUTRINO_USE_X11=1 NEUTRINO_FAST_RENDER=1 ./build/neutrino_test_08_richtext_code --fast
```

Idle CPU checks:

```text
neutrino_test_17_tabs_toolbar_splitter idle_cpu_ticks_2s=0
neutrino_test_08_richtext_code idle_cpu_ticks_2s=0
```

The X11 screenshots confirmed:

- vertical splitter child clipping works when the sash crosses the right pane's start position;
- horizontal splitter child clipping works when the sash crosses the lower pane's start position;
- rich text/code editor text, gutter line numbers, and keyword-line highlighting are readable on the dark theme.

## Windows note

The Win32 source path and MSVC project files were updated in-place. This Linux container does not include MSVC, so the Windows path was not executed here. The Win32 splitter implementation was kept parallel to the X11 implementation and uses Win32 clipping (`SaveDC`, `IntersectClipRect`, `RestoreDC`) with the same minimum-pane calculation.
