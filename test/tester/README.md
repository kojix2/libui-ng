# Interactive tester

Build with `cmake --build build --target tester` and run
`build/artifacts/<configuration>/tester` (`tester.exe` on Windows).
This is a manual GUI exerciser; it is not registered with CTest.

The first tab row selects a subject. The second selects a scenario. Source
names follow those scenarios rather than the historical page numbers.

| Subject | Scenario | Source | Previous location |
| --- | --- | --- | --- |
| Controls | Entries: editable/read-only echo | `entries.c` | Page 2 |
| Controls | Multiline: wrapping, set and append | `multiline.c` | Page 12 |
| Controls | Numbers: spinbox, slider, range, tooltip and progress | `numbers.c` | Pages 4, 13 |
| Controls | Selection: combo boxes and independent radio groups | `selection.c` | Pages 4, 13 |
| Controls | Date/Time: combined, date and time pickers | `datetime.c` | Page 4 |
| Controls | State: nested enable/disable, disabled tabs, initially hidden child | `control_state.c` | Page 2 |
| Layout | Properties: text/title round trips, spacing, child and box visibility | `properties.c` | Page 1 |
| Layout | Parenting: move a label between boxes, move Properties out of and back into its tab | `layout_parenting.c` | Page 2 |
| Layout | Visibility: compare hidden and omitted children, with and without stretching | `layout_visibility.c` | Page 3 |
| Layout | Sizes: open horizontal/vertical minimum-size control collections | `layout_sizes.c` | Page 13 |
| Layout | Form: password/search/multiline entries, hide a child, delete a row | `layout_form.c` | Page 13 |
| Layout | Grid: insertion, spanning, alignment, hidden and empty cells | `layout_grid.c` | Page 14 |
| Layout | Empty: deliberately blank tab | `main.c` | Empty Page |
| Drawing | Area: drawing gallery, mouse/key logging, enable/disable | `area.c`, `drawtests.c` | Page 6 |
| Drawing | Arc: angle, sweep, direction and radians | `area_arc.c` | Page 7a |
| Drawing | Mouse: scrolling coordinates and enter/leave state | `area_mouse.c` | Page 7b |
| Drawing | Scroll: clipping and a gradient, with switchable large content | `area_scroll.c` | Page 7c |
| Drawing | Text: fixed attributed/Unicode samples, font/color buttons and extents | `text.c` | Page 10 |
| Windows | Geometry & Focus: size, position, fullscreen, borders, auxiliary windows and focus | `windows.c` | Pages 2, 5, 15 |
| Windows | Dialogs: open file/folder, save file, message and error dialogs | `dialogs.c` | Page 5 |
| Tables | Columns & Selection: cell types, headers, widths, selection and event reports | `table.c` | Page 16 |
| Tables | Model Updates: insert/delete rows with two views of one model | `table_model.c` | Page 17 |

`main.c` assembles the navigation and owns application startup/shutdown.
`menus.c` exercises menu items. `spaced.c` tracks the containers affected by
the global spacing switch and supplies the box constructors used by `swaphv`.
`images.c` embeds the image data; `images/` keeps the source images and generator.
Windows resources and manifests remain beside the executable's sources.

The Properties/Parenting pair intentionally shares controls: Properties edits
the title of Parenting's group, and Parenting reparents the complete Properties
panel. `main.c` passes those dependencies explicitly. Properties must be the
first tab in Layout, with Parenting immediately after it. The navigation button
also works while Properties is outside the tab. Scenario state stays in its own
source; the two table modules release their models/images after the windows and
controls have been destroyed and before `uiUninit()`.
Form removal keeps detached children alive for the Show/Hide scenario;
`freeLayoutForm()` destroys those children at shutdown. Repeated deletion stops
when the form is empty.

Keep callbacks, state and control construction for an operation together.
For a new scenario, choose the subject by the behavior being inspected: a form
with entries belongs under Layout when its purpose is row deletion or child
visibility. Retain coupled operations such as synchronized numeric controls and
the two table views in the same scenario.

## Options

Existing options can be combined:

```sh
build/artifacts/Debug/tester nomenus
build/artifacts/Debug/tester startspaced
build/artifacts/Debug/tester swaphv
build/artifacts/Debug/tester steps
```

`nomenus` omits the menu definitions; `startspaced` enables margins/padding at
startup; `swaphv` swaps the tracked horizontal/vertical box constructors;
`steps` uses the stepping event loop. They apply across all subjects.

## Manual checks after reorganizing scenarios

- Visit every subject/scenario, including Grid's six inner tabs and Empty.
  For a visual review, capture each tab and inspect labels, clipping and the
  intended comparison. Empty and Empty Grid are deliberately blank; State
  deliberately contains disabled controls. Keep Visibility's unpadded reference
  boxes and the Grid comparisons unchanged.
- In Entries, type into the editable entry and check its read-only echo.
  In Multiline, exercise Set/Append for both wrapping modes.
- In Numbers, exercise slider/spinbox updates, release reporting, range limits,
  tooltip toggles and indeterminate progress. In Selection, append/insert/delete/
  clear items and confirm Second/None affect the interactive radio group while
  the other two groups remain independent.
- In Properties, get/set Parenting's group title. Follow Go to Parenting, move
  the label both ways, then move Properties out and back. Repeat the navigation
  while Properties is outside the tab; check spacing and visibility operations.
- In Form and Grid, hide/show children and exercise deletion/insertion. Compare
  Visibility's hidden/omitted children and Sizes' auxiliary windows.
- In Drawing, select gallery examples and check redraws, arc parameters, scroll
  clipping, coordinates, key handling, fonts and text colors.
  Arc starts with a visible 270-degree sweep. Scroll's Large content switch
  exposes scrolling even when the window is larger than the 250-pixel drawing;
  inspect both axes and clipping at the viewport edge. Text draws fixed samples;
  Redraw applies the width, font and color settings to those samples.
- Exercise window geometry/focus and the auxiliary windows. Open/cancel each
  dialog and check its result. Check the menu toggles in a run with menus.
- In Tables, change headers/widths/selection and interact with cell types. In
  Model Updates, insert/delete a valid row and verify both views update.
- Close the main window normally; check that execution reaches `after uiUninit()`.
  Repeat with the options above on the target OS.

Startup or automated tab traversal alone does not verify rendering, native
window-manager behavior or every interactive operation. Record the OS, linkage,
options and operations actually checked; Windows/macOS need their own checks.
