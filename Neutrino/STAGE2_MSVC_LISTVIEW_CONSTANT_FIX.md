# Stage2 MSVC ListView constant/scope fix

This fix moves Win32 ListView/TableView row and column constants to file scope and replaces the remaining mixed-type `std::max` calls with explicit integer helper calls.

Fixed symbols/errors reported by MSVC IntelliSense and the compiler:

- `kListMinColumnWidthWin32` undefined
- `kListDefaultColumnWidthWin32` undefined
- `kListRowHeightWin32` undefined
- `darkerWin32` undefined
- `std::max` overload failures in Win32 ListView/TableView code

The affected file is:

- `src/win32/Neu_Win32.cpp`
