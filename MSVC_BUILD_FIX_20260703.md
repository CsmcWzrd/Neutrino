# MSVC Build Fix - 2026-07-03

This package fixes Win32 backend compilation errors reported from Visual Studio 2022 in `src/win32/Neu_Win32.cpp`.

Fixed areas:

- Replaced problematic `std::max(1, RECT_LONG_EXPRESSION)` calls with explicit `neuMaxIntWin32(...)` calls.
- Added explicit integer casts for Win32 `RECT` width/height calculations.
- Fixed ListView/TableView and TreeView header text truncation calls that combined `truncateTextToWidth(...)` with `std::max(...)` over Win32 `LONG` values.
- Kept `_CRT_SECURE_NO_WARNINGS` and standard C++17 project settings in the VS2022 project files.

The affected errors were caused by MSVC refusing to instantiate `std::max/std::min` when one argument was `int` and the other was Win32 `LONG`.
