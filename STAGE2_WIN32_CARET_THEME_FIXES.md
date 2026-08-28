# Stage2 Win32 caret/theme fix pass

This pass addresses remaining Windows-side visual/input issues reported after the Stage2 beauty rendering update.

## Fixed/updated

- `Neu_RichTextCode` now handles Backspace through an override path and normalizes the caret after newline deletion so the insert cursor does not drift right after deleting a line.
- Large/container controls no longer use hover highlight fills:
  - `Neu_RichTextCode`
  - `Neu_ReadOnlyRichText`
  - `Neu_Placement`
  - `Neu_ScrollWindow`
  - `Neu_ListView`
  - `Neu_TreeView`
  - `Neu_Multilinetextbox`
- ListView/TableView and TreeView row hover/selection highlights now use the active theme corner geometry instead of rectangular fills.
- Win32 drop shadows now use the theme-shaped path, including top-left-edge and bottom-right-edge corner styles.
- Tab selection buttons now use themed corner geometry.
- ProgressBar filled/highlight segment now uses themed corner geometry.

The fixed layout model is unchanged and the package remains proprietary.
