# Stage2 MSVC compile fixes

This Stage2 refresh addresses Win32 backend compile errors reported from Visual Studio 2022 in `src/win32/Neu_Win32.cpp`.

Fixed items:

- Replaced mixed `std::max` calls involving Win32 `RECT`/`LONG` fields with explicit integer helper calls.
- Replaced the affected ListView/TreeView header truncation width expressions with explicit integer-safe calculations.
- Kept the public API unchanged.
- Kept all Visual Studio projects on C++17 and retained `_CRT_SECURE_NO_WARNINGS`.

The changes are intentionally narrow and are limited to Win32 compile-safety around integer clipping/truncation expressions.
