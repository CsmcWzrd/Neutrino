# Neutrino verification notes

Revision verified for the 2026-07-02 window-close, CPU-use, Windows event-routing, and Linux regression update.

## Linux/X11 verification performed in this environment

The following commands completed successfully from the project root:

```sh
make -j2
cmake -S . -B cmake-build
cmake --build cmake-build -j2
./autogen.sh
./configure
make -f Makefile.autoconf -j2
```

The Linux Makefile, CMake, and autoconf build paths produced the Neutrino static library and the graphical test/demo binaries.

## Windows/Win32 verification scope

The Windows source path and Visual Studio 2022 project files were updated and statically checked in this Linux container. MSVC and the Windows SDK are not available in this environment, so the Visual Studio solution was not compiled here.

Updated Windows behavior includes:

- `WM_CLOSE` invokes the optional close callback, unregisters the window, destroys the native `HWND`, and posts quit after the last window closes.
- The Win32 application loop uses blocking `GetMessageW()` dispatch instead of a polling loop.
- Control redraws are coalesced through `InvalidateRect()` and rendered through `WM_PAINT`.
- `WM_ERASEBKGND` is suppressed to reduce flicker.
- Mouse events are routed to hovered/captured controls; keyboard input is routed to the focused control.
- Textbox/password/multiline editing supports focus, caret drawing, printable input, Backspace, Delete, Home, End, Left, and Right.
- The requested `Neu_Window(width, height, title)` size is converted to a client-area window size with `AdjustWindowRectEx()`.
- Nested `Neu_Placement` controls propagate their parent window to child controls on both Linux and Windows.
- Mouse wheel scrolling targets the hovered/focused scrollable control; Shift+wheel scrolls horizontally on Windows.
