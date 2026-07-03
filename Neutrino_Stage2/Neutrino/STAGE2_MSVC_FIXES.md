# Stage2 MSVC Fixes

This Stage2 refresh fixes the Win32 backend compilation errors reported from Visual Studio 2022 around `src/win32/Neu_Win32.cpp`.

## Fixed issues

- Replaced mixed `std::max` calls using Win32 `RECT`/`LONG` fields with explicit integer helper calls.
- Fixed the supersampled checkbox/radio helper paths that computed width and height from `RECT`.
- Fixed ComboBox/ListView/TreeView text clipping width expressions that mixed `LONG` and `int` values.
- Removed the failing `std::max(...)` expressions from `truncateTextToWidth(...)` call sites in the Win32 ListView and TreeView drawing paths.

## Notes

The fix keeps the public API unchanged. It only adjusts the Win32 drawing backend implementation so MSVC can resolve the integer overloads cleanly under C++17.
