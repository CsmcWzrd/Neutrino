# Neutrino Stage2 Fix Pack

This Stage2 fix pack addresses the control behavior issues reported during Windows and Linux testing.

## Fixes in this package

- Centered ComboBox and Spinner up/down arrows.
- Reworked ScrollWindow child coordinate handling so child controls are content-relative after `add()`.
- Reworked ScrollWindow drawing and hit testing so moving the scroll window keeps the contents in sync.
- Added stricter clipping for ScrollWindow children on Windows and Linux.
- Added clipped Splitter pane drawing so controls on either side of a moved sash cannot paint outside their pane.
- Added darker ListView/TableView and TreeView header bars.
- Added TreeView header resizing on Windows and retained ListView header resizing behavior.
- Improved text click-to-caret mapping in multiline text and RichTextCode on Windows.
- Hardened `neutrino_test_14_label_offsets_themes` callback payload ownership to avoid a crash and redraws the full window on theme changes.
- Added supersampled Win32 drawing for CheckBox and RadioButton glyphs.
- Improved ListView/TableView virtual width calculation so wide columns expose horizontal scrolling.
- Preserved proprietary packaging; no MIT/open-source license file is included.

## Reverified build paths

```sh
make -j2
cmake -S . -B cmake-build-stage2-fix
cmake --build cmake-build-stage2-fix -j2 --target Neutrino neutrino_test_14_label_offsets_themes neutrino_test_17_tabs_toolbar_splitter neutrino_test_18_stage2_scroll_controls
./configure
make -f Makefile.autoconf -j1 build-autoconf/libNeutrino.a build-autoconf/neutrino_test_14_label_offsets_themes build-autoconf/neutrino_test_17_tabs_toolbar_splitter build-autoconf/neutrino_test_18_stage2_scroll_controls
```

The Visual Studio 2022 solution is included under `msvc/`. The Linux packaging environment cannot execute MSVC, but the Win32 backend source has been updated for the Windows-specific fixes above.
