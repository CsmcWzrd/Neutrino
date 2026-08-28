# Neu_Cards text boundary fix

This update fixes card text drawing so item text cannot overstep the bottom boundary of a card.

## Root cause

`Neu_Cards` originally passed the same vertical value to `drawText()` on every platform. On Linux/X11 that value is interpreted as a text baseline, but on Win32 the backend uses `TextOutW()`, where the value is the top of the text box. As a result, the Windows card text was drawn too low; the last item row could spill below the bottom edge of the card.

## Fix

- Card rows now use a `lineTop` coordinate.
- Win32 text uses a top-aligned `textY` calculated from `lineTop`.
- X11 text keeps using a baseline-style `textY` calculated from `lineTop`.
- Card content is clipped to the inside of each card before icons/text are drawn.
- The item line height was increased slightly from 20px to 22px to better match the active font metrics.
- The card height calculation now uses the same row height that drawing uses.

## Verification

Linux/X11 was rebuilt and run under Xvfb with `NEUTRINO_USE_X11=1` and `NEUTRINO_FAST_RENDER=1`. The card test displayed correctly, the text stayed inside card content bounds, and idle CPU remained at zero scheduler ticks over a two-second idle window.
