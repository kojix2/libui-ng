# Examples

Single-file examples are minimal, complete programs demonstrating one concept
each. Copy one as a starting point.

Subdirectories are larger demos that combine several concepts:
`controlgallery`, `histogram`, `drawtext`, `datetime`, and `cpp-multithread`.

## Single-file examples

| File | Concept |
| --- | --- |
| `hello-world.c` | Create a window with a label. |
| `window.c` | Configure a basic window and set its child control. |
| `button.c` | Handle a button click. |
| `label.c` | Read label text, free it with `uiFreeText()`, and update the label. |
| `checkbox.c` | Handle checkbox toggles and read checked state. |
| `entry.c` | Read entry text and free it with `uiFreeText()`. |
| `multiline-entry.c` | Set and append multi-line text. |
| `box-layout.c` | Arrange controls with vertical and horizontal boxes. |
| `form-layout.c` | Create labeled fields with `uiForm`. |
| `grid-layout.c` | Place controls in rows and columns with `uiGrid`. |
| `tabs.c` | Create a tab control with multiple pages. |
| `group.c` | Place controls inside a titled group. |
| `control-destroy.c` | Replace a window child and explicitly destroy the old child. |
| `combobox.c` | Select an item from a fixed list. |
| `editable-combobox.c` | Select or type text in an editable combobox. |
| `radio-buttons.c` | Select one item from radio buttons. |
| `spinbox.c` | Edit an integer with a spinbox. |
| `slider.c` | Edit an integer with a slider. |
| `progressbar.c` | Display determinate and indeterminate progress. |
| `separator.c` | Add horizontal and vertical separators. |
| `date-picker.c` | Select a date. |
| `time-picker.c` | Select a time. |
| `color-button.c` | Choose and read a color. |
| `font-button.c` | Choose and read a font descriptor. |
| `menu.c` | Create menus and menu items. |
| `menu-checkbox.c` | Create and read a checked menu item. |
| `open-file.c` | Open a file dialog and free the returned path. |
| `open-folder.c` | Open a folder dialog and free the returned path. |
| `save-file.c` | Open a save file dialog and free the returned path. |
| `message-box.c` | Show a message dialog. |
| `error-message-box.c` | Show an error message dialog. |
| `timer.c` | Run a repeating timer callback. |
| `queue-main.c` | Queue a callback onto the main UI loop. |
| `should-quit.c` | Handle application quit requests. |
