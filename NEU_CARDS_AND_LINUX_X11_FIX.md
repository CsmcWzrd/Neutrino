# Neu_Cards and Linux/X11 blank-screen verification

This update adds a list-style card control and keeps the Linux/X11 rendering fixes from the previous package.

## New API

### `Neu_CardItem`

`Neu_CardItem` stores one visual item inside a card.

- `std::wstring text` is the visible text.
- `std::string iconPath` is optional.
- `Neu_CardIconPosition iconPosition` can be `Beginning` or `End`.
- `simpleRichText` is enabled by default for the primary item.

Simple rich text markers supported by the primary item:

- `**bold**`
- `//italic//`
- `__underline__`
- `~~strike~~`
- `` `monospace` ``

Use backslash to escape marker characters, for example `\*`.

### `Neu_Card`

`Neu_Card` contains:

- `std::vector<Neu_CardItem> items`
- `int levelIndex`

The first item is the primary item. It supports icon placement and simple rich text. Subsequent items are displayed as text-only detail rows.

### `Neu_Cards`

`Neu_Cards` is a scrollable list-like control.

Important setters:

```cpp
cards->setCards(vectorOfCards);
cards->addCard(card);
cards->setCardLevelOffset(34);
cards->setCardMinHeight(58);
cards->setCardPadding(12, 8, 12, 8);
cards->setCardSpacing(9);
cards->setItemSpacing(2);
cards->setIconSize(22);
cards->setSelectable(true);
cards->setSelectedIndex(0);
```

The rendered x position is shifted by:

```text
levelIndex * cardLevelOffset
```

## New test application

Added:

```text
examples/neutrino_test_22_cards.cpp
```

Built executable:

```text
build/neutrino_test_22_cards
```

It verifies:

- level indentation
- primary item icon at beginning
- primary item icon at end
- additional text-only item rows
- scrolling
- selection callbacks
- simple rich text markers
- Linux/X11 repaint safety

## Linux/X11 blank-screen verification

The latest source was built and run under Xvfb with the plain X11 runtime path:

```sh
make -j2
DISPLAY=:46 NEUTRINO_USE_X11=1 NEUTRINO_FAST_RENDER=1 ./build/neutrino_test_22_cards --fast
```

The test displayed controls and text correctly. Idle CPU was checked using `/proc/<pid>/stat` and reported:

```text
idle_cpu_ticks_2s=0
```

The previous X11 fixes remain in place:

- redraws are coalesced through a dirty flag instead of synchronous recursive repainting
- redraw requests raised during paint do not start another immediate frame
- the X11 text backend draws once by default
- the XRender library remains loaded until after `XCloseDisplay()`
- `MapNotify`, `Expose`, `ConfigureNotify`, `DestroyNotify`, and `WM_DELETE_WINDOW` are handled
