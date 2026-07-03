# Stage2 MSVC Text Width Helper Fix

This update fixes a Windows/MSVC compile error in `src/win32/Neu_Win32.cpp` where the Win32 rich-text caret normalization path called `textWidthWin32(...)` before that helper existed in the translation unit.

## Fix

- Added a file-scope `textWidthWin32(HDC, const std::string&, bool, bool, bool, int)` helper.
- The helper uses the same Win32 font creation path as the editable controls and calls `GetTextExtentPoint32W` for accurate caret and scroll-width calculation.
- The helper includes a safe fallback width when no `HDC` is available.

This directly addresses the reported error:

```text
C3861: 'textWidthWin32': identifier not found
E0020: identifier "textWidthWin32" is undefined
```
