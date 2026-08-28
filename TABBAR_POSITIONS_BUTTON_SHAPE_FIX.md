# Tabbar Button Shape and Position Fix

This update extends `Neu_TabView` so the tab strip behaves like a proper tabbar while preserving Neutrino's fixed-position layout model.

## API additions

- `Neu_TabPosition::Top`
- `Neu_TabPosition::Bottom`
- `Neu_TabPosition::Left`
- `Neu_TabPosition::Right`
- `Neu_TabView::setTabPosition(Neu_TabPosition)`
- `Neu_TabView::tabPosition()`
- `Neu_TabView::setTabBarThickness(int)`
- `Neu_TabView::tabBarThickness()`
- `Neu_TabView::setMinimumTabButtonSize(int)`
- `Neu_TabView::minimumTabButtonSize()`

## Rendering changes

- Tab buttons now use the same rounded shape as normal Neutrino buttons.
- The selected tab uses the theme highlight/focus visual treatment.
- Non-selected tab buttons use a lighter or darker variant of `theme.highlight` so they remain visually related to normal highlighted buttons without looking focused.
- Tab content is clipped to the page body and cannot draw over the tab strip.
- Side tabbars use horizontal text in vertically stacked rounded buttons.

## Verification

Updated `neutrino_test_17_tabs_toolbar_splitter` to display all four tab positions: top, bottom, left, and right. The same test still contains the splitter minimum-size and clipping regression checks.

Linux/X11 verification command:

```sh
DISPLAY=:79 NEUTRINO_USE_X11=1 NEUTRINO_FAST_RENDER=1 ./build/neutrino_test_17_tabs_toolbar_splitter --fast
```

Runtime idle CPU check reported:

```text
idle_cpu_ticks_2s=0
```
