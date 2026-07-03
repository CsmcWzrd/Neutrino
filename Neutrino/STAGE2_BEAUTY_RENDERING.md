# Neutrino Stage2 Beauty Rendering Update

This Stage2 update changes the default visual language to `MaterialDark` and adds theme-driven rendering knobs for both Linux/X11 and the native Win32 backend.

## Default theme

New `Neu_Window` instances default to:

```cpp
Neu_Theme::MaterialDark()
```

The common test helper also applies `MaterialDark` unless a test explicitly changes the theme.

## Gradient surfaces

Controls now draw a themed surface gradient by default. The main fields are:

```cpp
theme.gradientControls = true;
theme.controlGradientTop = {58, 62, 70, 255};
theme.controlGradientBottom = {22, 24, 30, 255};
theme.highlight = {64, 76, 92, 255};
theme.focus = {42, 112, 178, 255};
```

`highlight` is used for mouse hover/highlight states. `focus` is the focus border/insert indicator color and is normally a darker companion of the accent color. Individual themes may override it.

## Corner styles

The default corner style is no longer fully rounded. By default:

```cpp
theme.setCornerStyles(Neu_CornerStyle::EdgeCorner,
                      Neu_CornerStyle::RoundedCorner,
                      Neu_CornerStyle::RoundedCorner,
                      Neu_CornerStyle::EdgeCorner);
```

This gives an edge on the top-left and bottom-right corners while retaining rounded corners elsewhere.

Supported programmatic settings:

```cpp
theme.setRoundedCorners();
theme.setDefaultEdgeCorners();
theme.setAllCorners(Neu_CornerStyle::EdgeCorner);
theme.setCornerStyles(topLeft, topRight, bottomLeft, bottomRight);
theme.edgeSize = 8;
```

Theme asset keywords used in `assets/themes/material_dark.theme` include:

```text
rounded-corner
corner-top-left=top-left-edge-corner
corner-top-right=top-right-rounded-corner
corner-bottom-left=bottom-left-rounded-corner
corner-bottom-right=bottom-right-edge-corner
```

## Antialiasing modes

Themes can specify antialiasing intent:

```cpp
theme.antiAliasMode = Neu_AntiAliasMode::DAA;
theme.antiAliasMode = Neu_AntiAliasMode::MSAA;
theme.antiAliasMode = Neu_AntiAliasMode::SSAA;
theme.antiAliasSamples = 4;
```

`DAA` means default device/font/XRender antialiasing. `MSAA` uses multi-sample shape antialiasing. `SSAA` uses super-sampled shape antialiasing for the smoothest surface/corner edges.

## Test application

A new cross-platform test was added:

```text
neutrino_test_19_material_beauty
```

It demonstrates the MaterialDark default, gradient buttons, highlight/focus colors, edge and rounded corner switching, and DAA/MSAA/SSAA theme settings without adding a new layouting scheme.
