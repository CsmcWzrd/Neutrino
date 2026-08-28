# Stage2 Rich Text Test and Alt Backspace Fixes

This update changes `Alt+Backspace` to perform Undo in editable text controls, matching the common desktop editing convention.

Undo/redo shortcuts now include:

- `Ctrl+Z`: Undo
- `Alt+Backspace`: Undo
- `Ctrl+Y`: Redo
- `Ctrl+Shift+Z`: Redo

Two full-window test applications were added:

- `neutrino_test_20_full_richtext_control`
- `neutrino_test_21_full_richtext_code_control`

Both are included in the Linux Makefile, CMake configuration, autoconf Makefile, and Visual Studio 2022 solution.
