# MSVC Neu_Cards Compile Fix

This update fixes the Windows/Visual Studio build errors introduced by the initial `Neu_Cards` implementation.

## Fixed issues

1. Removed deprecated C++17 `<codecvt>` / `std::wstring_convert` usage from `src/Neu_Cards.cpp`.
   - Visual Studio treats these deprecation diagnostics as C4996 errors in this project configuration.
   - Replaced them with a small internal UTF-8 <-> `std::wstring` conversion helper.
   - Handles Windows UTF-16 `wchar_t` surrogate pairs and Linux/macOS UTF-32 `wchar_t`.

2. Fixed platform-specific event handling in `Neu_Cards::handleXEvent()`.
   - Windows now uses the project `XEvent` shim fields: `message`, `x`, `y`.
   - Linux/X11 continues to use real X11 fields: `type`, `xmotion`, `xbutton`.
   - This removes MSVC errors for missing `XEvent::type`, `XEvent::xmotion`, `MotionNotify`, `LeaveNotify`, and `ButtonRelease`.

3. The `Neutrino.lib` linker errors are resolved once the Neutrino static library compiles successfully.
   - Those were cascading errors caused by the failed `Neutrino` project build.

## Verification performed here

- Linux make build completed.
- Linux CMake build completed.
- Windows branch of `src/Neu_Cards.cpp` was syntax-checked with a local Win32 API shim to catch `_WIN32` event-path compile errors in this Linux container.

## Files changed

- `src/Neu_Cards.cpp`
- `MSVC_NEU_CARDS_COMPILE_FIX.md`
