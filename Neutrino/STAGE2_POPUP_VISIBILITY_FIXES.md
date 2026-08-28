# Stage2 Popup Window Menu Visibility Fixes

This update adds explicit popup visibility control to `Neu_PopWindowMenu` and the compatibility alias `Neu_PopupWindowMenu`.

## New API

```cpp
menu->show();
menu->showAt(x, y);
menu->hide();
menu->toggle();
bool open = menu->isVisible();
```

`Neu_PopWindowMenu` remains visible by default for backwards compatibility with existing examples. Applications that want normal popup behavior can call `hide()` immediately after construction and then open the menu with `show()`, `showAt(...)`, or `toggle()` from a button/menu callback.

## Behavior

- Hidden popup menus do not draw or receive events through normal parent routing.
- `showAt(x, y)` repositions the fixed-layout top-left location before showing the menu.
- Category selection in the left sidebar is handled by the control on Linux/X11 and Win32.
- `Neu_PopupWindowMenu` is provided as an alias for the original `Neu_PopWindowMenu` class name.

## Test coverage

`examples/neutrino_test_07_popup_menu_categories.cpp` now includes Show, Hide, and Toggle buttons that exercise the new API.
