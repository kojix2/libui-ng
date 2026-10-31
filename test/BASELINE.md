# Test baseline and implementation map

Recorded 2026-10-31. These are executable and cmocka case counts, not coverage
percentages. Source-only inventories and unexecuted configurations are marked
explicitly. The initial implementation covers baseline recording, fixture
extraction, CMake registration and GTK date/time and slider migration.
Later organization and its validation are recorded separately below.

## Snapshots and results

| Snapshot | Configuration | CTest | cmocka cases | Result |
| --- | --- | --- | --- | --- |
| `pre-build` `4cddb78b21c83942a7967c1a527bfa36a0883923` before organization | Debug shared | 10 | 255 | Passed |
| Same snapshot before organization | Debug static | 10 | 274 | Passed |
| Same snapshot before organization | Debug static, ASan + UBSan | 10 | 274 | Passed |
| Working tree organization based on that snapshot | Debug shared | 10 | 255 | Passed; case names/order unchanged |
| Same working tree | Debug static | 10 | 274 | Passed; case names/order unchanged |
| Same working tree | Debug static, ASan + UBSan | 10 | 274 | Passed |
| Same working tree, cmocka 1.1.8 | Debug static | 10 | 274 | Passed |
| Same working tree, local-source cmocka 1.1.8 fallback | Debug shared | 10 | 255 | Passed |
| `dev` `ad4fad8a` exported unchanged to a separate source directory | Debug shared | 16 | 336 | Passed |
| Same `dev` snapshot | Debug static | 16 | 357 | Passed |
| `dev` | Sanitizers / coverage | — | — | Not measured in this change |
| Both branches | Windows / macOS native execution | — | — | Unverified locally; CI configuration reviewed, remote run results not retrieved |

Environment: Ubuntu 26.04.1 LTS, Linux x86_64 (7.0.0-42), GCC/G++ 15.2.0,
CMake 4.2.3, Ninja, GTK 3.24.52, system cmocka 2.0.2. Test executions used Xvfb
and `GDK_BACKEND=x11`. The additional cmocka 1.1.8 builds used the local source
and an isolated install; no dependency download was needed. All builds used
C99/C++14 through existing internal options. Older compiler/CMake versions,
Release, other architectures and native Windows/macOS runs were not tested
locally in this change.

The first sandboxed runs could not connect to the Xvfb display: 6/10 executables
passed and the four GUI executables failed initialization. Those are recorded
as environment failures, not successful skips. Runs with access to the local
X server passed as shown above. ASan used `detect_leaks=0`; UBSan used
`halt_on_error=1`. This does not establish leak freedom.

## `pre-build` map

Paths below are relative to the repository. Case names are in each source's
registration array. All 30 defined component runners are registered in
`test/unit/main.c` and their files are listed by CMake; `control` is the one
static-only component. There are no unregistered component runners. The shared
`unit` executable initially ran 230 cases, and static 249 (now 232/251
after the two matrix contracts below). Except for the stated
internal references, components call the public API through linked libui.
Counts in this table are Linux counts; platform guards can change them.

| Component / `test/unit/` file | Contract or feature | Main implementation | Cases shared / static |
| --- | --- | --- | --- |
| `init.c` | Version, initialization, queue, timers, deferred destruction | `unix/main.c`, `common/control.c`, generated version | 11 / 11 |
| `control.c` | Callback reentry and deferred destruction; internal references | `common/control.c`, `unix/control.c` | excluded / 19 |
| `window.c` | Defaults, geometry, notifications | `unix/window.c` | 13 / 13 |
| `box.c` | Child lifetime and layout | `unix/box.c` | 8 / 8 |
| `group.c` | Label, margins, child | `unix/group.c` | 4 / 4 |
| `menu.c` | Items and enabled/checked state | `unix/menu.c` | 17 / 17 |
| `slider.c` | Range, value, tooltip flag, callback suppression | `unix/slider.c` | 14 / 14 |
| `spinbox.c` | Range and values | `unix/spinbox.c` | 8 / 8 |
| `label.c` | Text | `unix/label.c` | 6 / 6 |
| `button.c` | Text | `unix/button.c` | 3 / 3 |
| `combobox.c` | Items and selection | `unix/combobox.c` | 18 / 18 |
| `editablecombobox.c` | Editable text | `unix/editablecombobox.c` | 4 / 4 |
| `checkbox.c` | Text and checked state | `unix/checkbox.c` | 6 / 6 |
| `radiobuttons.c` | Items and selection | `unix/radiobuttons.c` | 7 / 7 |
| `tab.c` | Pages, margins and ownership | `unix/tab.c` | 8 / 8 |
| `entry.c`, `entry_gtk.c` | Normal/password/search text, native search events | `unix/entry.c` | 20 / 20 |
| `multilineentry.c` | Text, readonly and callbacks | `unix/multilineentry.c` | 10 / 10 |
| `progressbar.c` | Default and changed values | `unix/progressbar.c` | 4 / 4 |
| `datetimepicker.c` | Picker construction variants | `unix/datetimepicker.c` | 3 / 3 |
| `separator.c` | Horizontal/vertical construction | `unix/separator.c` | 2 / 2 |
| `fontdescriptor.c` | Load/free control font | `unix/drawtext.c` | 1 / 1 |
| `fontbutton.c` | Font and notification | `unix/fontbutton.c` | 4 / 4 |
| `colorbutton.c` | Color and notification | `unix/colorbutton.c` | 3 / 3 |
| `form.c` | Rows, padding and lifetime | `unix/form.c` | 5 / 5 |
| `area.c`, `area_gtk.c` | Events and ownership, including GTK observations | `common/areaevents.c`, `unix/area.c` | 7 / 7 |
| `image.c` | Representations and lifetime | `unix/image.c`, `common/imagerep.c` | 4 / 4 |
| `grid.c` | Insertion, padding, sizing and lifetime | `unix/grid.c`, `common/gridutil.c` | 8 / 8 |
| `drawmatrix.c` | Matrix operations, point/size conversion and inversion | `common/matrix.c`, `unix/drawmatrix.c` | 10 / 10 initially; 12 / 12 now |
| `attrstr.c` | Attribute overlap, self-editing, graphemes | `common/attrstr.c`, `common/attrlist.c`, `common/attribute.c`, `unix/attrstr.c` | 11 / 11 |
| `table.c`, `table_value.c`, `table_width.c` | Values, columns, selection, model notifications, widths | `common/table*.c`, `unix/table*.c` | 11 / 11 |

`unit` requires a GUI session as an aggregate executable. The isolated
`drawmatrix` component does not initialize libui. Other platforms use the
corresponding Windows/AppKit implementations, with additional guarded cases;
Linux case totals must not be used as their totals.

| CTest entry | `test/unit/` source / feature | Connection to implementation | Cases | GUI / platform / linkage |
| --- | --- | --- | --- | --- |
| `utf-unit` | `utf.c`: encoding and iteration | Compile `common/utf.c` directly | 6 | no / portable / independent of libui linkage |
| `allocsize-unit` | `allocsize.c`: overflow boundaries | Include `common/allocsize.h` | 1 | no / portable / independent |
| `shouldquit-unit` | `shouldquit.c`: quit handler | Compile `common/shouldquit.c` directly | 1 | no / portable / independent |
| `imagerep-unit` | `imagerep.c`: representation choice | Compile `common/imagerep.c` directly | 8 | no / portable / independent |
| `gridutil-unit` | `gridutil.c`: dimension calculation | Compile `common/gridutil.c` directly | 3 | no / portable / independent |
| `alloc-unix` | `alloc_backend.c`: data, realloc, overflow aborts | Compile `unix/alloc.c`; subprocess invalid-size requests | 2 | no / GTK / independent |
| `minimumsize-gtk` | `minimumsize_gtk.c`: Box, Form, Grid alignment, focus, external control | Public libui + GTK handles | 5 sequential scenarios, not cmocka | yes / GTK / static + shared |
| `datetimepicker-gtk` | `datetimepicker_gtk.c`: valid and overflowing hour input | Public libui + native spin-button input signal | 2 | yes, hidden / GTK / static + shared |
| `slider-gtk` | `slider_gtk.c`: range changes update/disable value tooltip | Public libui + native tooltip observation | 2 | yes, hidden / GTK / static + shared |
| `windows-regression-unit` | `regression_windows.c`, `area_state_windows.cpp`: lifetime, tables, input, Area/drawing | Linked libui + Win32 messages; static-only internal hooks | 20 shared / 24 static, source inventory only | yes / Windows / static + shared; unverified |
| `layout-darwin` | `layout_darwin.m`: sizing, constraints, visibility | Linked libui + AppKit observations | 8, source inventory only | yes / macOS / static + shared; unverified |
| `alloc-darwin` | `alloc_backend.c`, `alloc_darwin.m`: allocation and overflow | Compile `darwin/alloc.m`; AppKit autorelease pool | 2, source inventory only | no display initialization / macOS / independent; unverified |

All existing CTest names, registration order, timeouts (unit 180s, Windows
regressions 120s, others 60s), platform conditions and resource sources remain
unchanged. Missing OS executables and the shared `control` component are
configuration exclusions, not unregistered or skipped tests.

## `dev` differences (unchanged source snapshot)

All 35 component runners are registered and included in CMake, with the same
static-only `control` exclusion. Relative to the Linux map above, `unit` changes
label to 10 cases, multilineentry to 18 and Area to 9. It adds `drawimage.c`
(2), `imageview.c` (4), `toolbar.c` (5 shared / 7 static), `tooltip.c` (5) and
`focus.c` (9). Its initial `unit` total is 269 shared / 290 static (now 271/292 after
the two matrix contracts below). Toolbar has additional
Windows-only cases not represented by these Linux totals.

| CTest entry / file | Feature / implementation connection | Linux cases | GUI / condition |
| --- | --- | --- | --- |
| `textarray-unit` / `textarray.c` | Direct `common/textarray.c` | 3 | no / portable |
| `imagerep-unit` / `imagerep.c` | Direct `common/imagerep.c`; expanded dimension/fit checks | 12 | no / portable |
| `imagebuffer-unit` / `imagebuffer.c` | Mock backend and included `common/imagebuffer.c` for failure injection | 8 | no / portable, `_UI_STATIC` independent of libui variant |
| `imagebuffer-cairo` / `imagebuffer.c` | `IMAGEBUFFER_CAIRO_TEST` selects real linked Cairo backend | 5 | no / GTK, static + shared |
| `alloc-unix` / `alloc_backend.c` | Also compiles `unix/text.c`, `common/textarray.c` | 3 | no / GTK |
| `imageviewrect-unit` / `imageviewrect.c` | Direct `common/imageview.c`, `common/imagerep.c` | 9 | no / portable |
| `drop-unit` / `drop.c` | Direct common DND/control/UTF/textarray sources with mocks | 6 | no / portable |
| `drop-gtk` / `drop_gtk.c` | Included `unix/drop.c` for transfer/lifetime injection; direct common sources | 6 | yes / GTK; return 77 skips if GTK display unavailable |

Unchanged entries retain the counts above. Additional native-only entries are
`tooltip-windows-unit` (`tooltip_windows.c`), `imagebuffer-windows`
(`imagebuffer_windows.cpp`), `drop-windows` (`drop_windows.cpp`),
`toolbar-darwin` (`toolbar_darwin.m`), and `drop-darwin` (`drop_darwin.m`).
These were inventoried from CMake but not built/run locally. `test/benchmark/`
contains `imagebuffer-bench`; its performance results are not correctness
CTest results and were not measured. No `dev` files were changed here.

## Fixture and integration checks

A standalone temporary probe linked only the new test support, libui and
cmocka, without the `unit` runner. For each teardown mode it deliberately
failed the first case after creating a child, then checked that destruction
was observed and the second case could initialize a new libui lifetime. Both
children were destroyed in each run; ASan/UBSan reported no error. The two
intentional assertion failures were expected by the probe and were not counted
as successful regression cases. GUI initialization failure remains outside
that cleanup guarantee.

Component CLI checks covered listing in both linkage modes, omission of static
`control` in shared, rejection of an unknown name before execution, and a
reversed/duplicate request (`slider init slider`) executing init then slider
once each. Tests-off builds and default-off `add_subdirectory`/FetchContent
consumers built and ran without cmocka. An install contained no support or
cmocka artifacts. Missing cmocka with fetching disabled produced the existing
configuration error; the enabled fallback was checked using local cmocka
1.1.8 sources, without accessing the network.

## Second organization: Table boundaries and existing CI

Starting from `pre-build` `4ef32df0`, value round trips move to `table_value.c`,
width scenarios to `table_width.c`, and the existing Table-specific fixture to
`table_fixture.c`/`table_test.h`. `table.c` retains column/selection/model
scenarios and the same registration array. Every function body, fixture field,
initialization/destruction sequence and event step is preserved, including
native conditional cases. No CTest executable or public symbol is added.
Shared/static/ASan+UBSan static all passed 10/10, with unchanged case
names/order (255 shared, 274 static) and CTest properties.

Before this second organization, native Windows/macOS CI results were checked
for both branch tips: [pre-build `4ef32df0`](https://github.com/kojix2/libui-ng/actions/runs/41188661053)
and [dev `128881ad`](https://github.com/kojix2/libui-ng/actions/runs/41188661580).
Both runs completed successfully with Windows MSVC/MSYS2 and macOS x64/arm64
static/shared Debug/Release jobs, along with the minimum-CMake job. This
supersedes the earlier source-only CI inventory for those commits. Execution of
the new source/registration changes on Windows/macOS remains pending.

## CI confirmation after the second organization

The exact organized tips completed their existing CI successfully:
[pre-build `48fae4c8`](https://github.com/kojix2/libui-ng/actions/runs/41189747256)
finished all 40 jobs successfully. [dev `c7f193ba`](https://github.com/kojix2/libui-ng/actions/runs/41189768602)
finished successfully with 39 successful jobs and the conditional release job
skipped. This confirms the Table/registration changes on the native Windows
and macOS matrices, along with the other existing configurations; it supersedes
the earlier pending-CI statements for these tips. Manual visual behavior and
leak auditing are not established by these runs.

The subsequent `cmake-linux-sanitizers` job adds static/shared Debug ASan+UBSan
checks on Linux without changing existing jobs or matrices. C/C++ flags cover
libui, tests and examples; the full CTest suite runs under Xvfb with leak
detection disabled and UBSan configured to halt with a stack trace. Diagnostic
artifacts contain logs/configuration, not SDK packages. Release depends on the
new sanitizer job as well as its previous prerequisites.

The workflow passes actionlint 1.7.12. Its parsed configuration preserves every
existing job and matrix, except for the explicit sanitizer prerequisite added
to release. Locally, the new commands passed both static and shared pre-build
CTest suites (10/10; 274/255 cases) under ASan/UBSan, with original case names
and order. The same static/shared sanitizer commands passed dev 16/16 with
357/336 cases, also preserving case names/order and CTest properties. Compile
commands in both branches confirm instrumentation in common/GTK production
sources, tests, and all examples, including the C++ example. These are local
checks of the new job; its hosted execution is still pending.

## Entry and Area source separation

Starting from `pre-build` `3dec36c8` and `dev` `84645869`, the existing native
search-entry scenarios move to `entry_gtk.c`, and GTK Area scenarios/helpers
move to `area_gtk.c`. Common files retain setup, API cases and their unchanged
registration arrays. `entry_test.h`/`area_test.h` are private to this component;
Area's existing observation state keeps its original reset points and lifetime.
Only the GTK build adds the two sources, and no CTest entry or production API
is added. Existing wait loops and widget destruction are deliberately preserved.
The initial pre-build move preserves all 15 Entry and 14 Area function bodies,
including registration; common sources contain no GTK headers/types/operations.
Local pre-build shared/static and ASan+UBSan static/shared each passed 10/10
(255/274/274/255 cases), preserving case names/order and CTest properties.
The combined dev move preserves 15 Entry and 18 Area function bodies,
including ScrollEvent and UserData assertions. Its normal shared/static and
ASan+UBSan static/shared suites each passed 16/16 (336/357/357/336 cases), with
original names/order and CTest properties. The native source additions are
the only CMake change; all production/API sources are unchanged.
The Entry/Area separation subsequently passed its full hosted matrices:
[pre-build `9ab9dc86`](https://github.com/kojix2/libui-ng/actions/runs/41191966650)
and [dev `7ba345a9`](https://github.com/kojix2/libui-ng/actions/runs/41191975927)
both completed successfully. This does not establish hosted execution of the
later matrix cases below.

The preceding sanitizer-CI change has now also passed hosted validation:
[pre-build `3dec36c8`](https://github.com/kojix2/libui-ng/actions/runs/41190806796)
completed all 42 jobs successfully, and
[dev `84645869`](https://github.com/kojix2/libui-ng/actions/runs/41190818235)
completed with 41 successful jobs and the conditional release job skipped.
Both static/shared ASan+UBSan jobs succeeded in each run. This supersedes the
sanitizer job's earlier pending-hosted-check note, and does not establish native
execution of the subsequent Entry/Area move.

## Matrix fixtures, diagnostics and missing contracts

Starting from `pre-build` `9ab9dc86` / `dev` `7ba345a9`, matrix-only heap
fixtures and empty teardown hooks are replaced by a runner-owned value with
per-case identity initialization. One comparison helper retains scaled `1e-5`
tolerance and caller locations, while printing component names and full-precision
expected/actual values. All original ten cases and their relative order remain.
Two separate cases verify point/size coordinates and unchanged matrix input;
the integral non-diagonal matrix includes translation, which only points apply.
No file, target, dependency or production code is added. The file is 272 lines,
compared with 277 before cleanup and the two added contracts.

A temporary no-op replacement for both transforms makes exactly the two new
cases fail with input/expected/actual coordinates; the original ten still pass.
The complete matrix component also passes with minimum cmocka 1.1.8 without
GUI initialization. These checks are separate from the ordinary test results.

Fresh static Debug GCC 15/gcov 15 runs with `--coverage -O0` used the same full
CTest suite and aggregation rules, clearing execution data before each run:

| Production measurement | Before | After |
| --- | --- | --- |
| `uiDrawMatrixTransformPoint` calls | 0 | 1 |
| `uiDrawMatrixTransformSize` calls | 0 | 1 |
| Combined lines hit / instrumented | 3872 / 5556 | 3880 / 5556 |
| Combined functions hit / instrumented | 663 / 978 | 665 / 978 |
| Combined branch edges hit / instrumented | 1002 / 1762 | 1002 / 1762 |

Common-source counts are unchanged; the added hits are in the GTK matrix
backend. Windows/macOS execution and coverage of these new cases remain
unconfirmed. Linux fallback-size coverage is not implied by GTK's native path.
The two cases increase pre-build cmocka totals to 257 shared / 276 static,
and dev totals to 338 shared / 359 static. CTest executable counts are unchanged.
Pre-build normal shared/static and ASan+UBSan static/shared each passed 10/10;
all earlier cases retain their relative order and only the two intended cases
are added. CTest names, labels, timeouts and skip conditions are unchanged.
Dev normal shared/static and ASan+UBSan static/shared likewise passed 16/16
(338/359/359/338 cases), with only these two case additions and all original
cases in their prior order. The hosted run for this matrix change is pending.

## Allocator death-test runtime

Linux overflow-test children now disable dumping with `PR_SET_DUMPABLE` before
triggering the expected abort. `RLIMIT_CORE=0` alone does not prevent piped
core collectors; this host uses Apport. The four invalid allocation requests,
`SIGABRT` checks and diagnostic checks are unchanged. Other platforms retain
their existing behavior.

On this host, three standalone runs fell from 4.364–4.372 seconds to
0.0043–0.0050 seconds. The same pre-build static Debug, serial Xvfb CTest run
fell from 7.21 to 2.85 seconds, with all 10 executables passing. The allocator
tests also passed with ASan + UBSan. These timings depend on the host's core
collector configuration; GUI waits and test counts are unchanged.

## Initial coverage baseline

GCC 15/gcov 15, static Debug, `--coverage -O0`, full Linux CTest suite. The same
configuration and aggregation were used before and after; all measured hits
and totals are unchanged:

| Production scope | Lines hit / instrumented | Functions hit / instrumented | Branch edges hit / instrumented |
| --- | --- | --- | --- |
| `common/` | 1053 / 1448 | 127 / 172 | 412 / 615 |
| `unix/` | 2819 / 4108 | 536 / 806 | 590 / 1147 |
| Combined | 3872 / 5556 | 663 / 978 | 1002 / 1762 |

The denominator includes instrumented production C sources and internal
headers in these two directories, including zero-hit records. It excludes
tests, examples, dependencies, generated version source, and all inactive OS
sources. gcov JSON records from repeated compilation of one source were merged
by canonical source path: line number, function start/name, and function identity plus line/branch
index; a key is hit if any copy executed it. There are no direct production
`.c` includes in this `pre-build` test snapshot. `dev` does use direct includes;
its aggregation has not been validated and is not represented by these totals.

In this initial measurement, gaps included all functions in `common/opentype.c` and
`unix/opentype.c`, matrix point/size transforms, and size/weight/italic/stretch/
background/underline/features attribute construction and access. Attribute
family/color operations do execute. These are candidates for separate contract
tests after organization; no production behavior or regression cases were
added here. Windows/macOS and `dev` coverage are unmeasured, not zero coverage.

## Allocator CI confirmation and Area waits

The allocator-change tips passed their full Build + Release workflows:
[pre-build `15c0104b`](https://github.com/kojix2/libui-ng/actions/runs/41195491068)
completed all 42 jobs successfully;
[dev `6e507d0d`](https://github.com/kojix2/libui-ng/actions/runs/41195505263)
completed with 41 successful jobs and the conditional release job skipped.
Both static/shared sanitizer jobs and native Windows/macOS jobs passed.

The GTK scroll-after-resize case now waits for a nonzero viewport, the updated
adjustment upper bound and visibility of the last requested rectangle. Each
phase processes nonblocking events with a monotonic one-second deadline; an
empty event queue is not a completion condition. The two ScrollTo requests and
all original assertions remain. The existing fixture window owns the Area
before any assertion, and hidden teardown handles success and failure without
a separate native window.

On the same Linux static Debug build, five separate Xvfb runs of `unit area`
before the change took 0.754–0.762 seconds (median 0.758); thirty after took
0.153–0.159 seconds (median 0.155), all seven cases passing each time. These
measurements include Xvfb startup and process shutdown. The seven cases also
passed with ASan + UBSan (`detect_leaks=0`, UBSan halts on errors). Entry's
notification observation periods are unchanged. Native Windows/macOS Area
cases are unchanged; this GTK change does not establish new results there.

## OpenType and attribute contracts

Based on `pre-build` `15c0104b` and `dev` `6e507d0d`, eight new portable
`opentype-unit` cases exercise the public feature-set and attribute value APIs
through compiled common implementation sources. The test-local allocator counts
live objects through teardown and uses ordinary zeroed allocations/reallocation;
it does not test or replace the native allocator. GUI initialization is absent.
Two new `unit attrstr` cases test feature-copy/merge/delete ownership and
background run equality, including an empty run whose attribute must be freed.
All eleven original attrstr cases retain their order. No production code,
public API, ABI, existing CTest properties or backend conditions change.

Linux Debug validation (GCC 15, GTK 3.24.52, system cmocka 2.0.2):

| Branch | Configuration | CTest | cmocka cases | Result |
| --- | --- | --- | --- | --- |
| pre-build | static | 11 | 286 | Passed |
| pre-build | shared | 11 | 267 | Passed |
| pre-build | static, ASan + UBSan | 11 | 286 | Passed; leak detection disabled |
| dev | static | 17 | 369 | Passed |
| dev | shared | 17 | 348 | Passed |
| dev | static, ASan + UBSan | 17 | 369 | Passed; leak detection disabled |

The rebased dev tree preserves its original production sources and public
headers byte for byte. `git range-diff` shows only the changed Area teardown /
helper context and appended baseline context for its fourteen development
commits. No development API change is dropped. Current local commits have not
yet run hosted CI; the successful runs above apply to the named earlier tips.

Minimum cmocka 1.1.8 also passes the eight headless cases and the seven Area /
thirteen attrstr cases. The headless executable independently passes ASan +
UBSan with LeakSanitizer enabled (`detect_leaks=1`), with no suppression. This
is a leak check of these common-source cases, not the GTK library or whole suite.
Tests-off and nested add_subdirectory consumer configurations omit the new target.

Temporary builds that overwrite a missing tag's output, or ignore enumeration
stop, fail four cases and one case respectively. Both complete all eight cases
without a teardown allocation failure. A separate shared-library interposition
that suppresses Area scrolling triggers its timeout in 1.14 seconds including
Xvfb, then successfully tears down and runs the following grab-broken case.
No mutation or interposition is part of the tracked source.

A fresh full static Debug coverage run (GCC/gcov 15, `--coverage -O0`, execution
data cleared first) passes 11/11. Merge duplicate production records by source
and function identity, counting a function as hit if any copy runs; do not add
common library and standalone copies twice. All functions in `common/opentype.c`
and `common/attribute.c` now execute, including the previously unexecuted public
feature-set operations and size/weight/italic/stretch/background/underline/
features construction/access. Background has no documented public getter;
its distinct/equal values are checked through attributed-string run boundaries.
Not every equality branch is covered. `unix/opentype.c`'s native CSS conversion
still does not execute in these tests; native font shaping and Windows/macOS
execution of the new cases remain unverified.

## GTK event cleanup on assertion failure

The grab-broken helper frees its temporary GdkEvent before asserting the signal's
handled result. A failed result assertion can no longer bypass event release.
The seven pre-build Area cases pass in Linux static Debug and ASan + UBSan
(`detect_leaks=0`); production sources, registration and case counts are unchanged.

## Coverage aggregation correction

`test/coverage.py` adds reproducible aggregation of GCC gcov JSON without a new
build dependency. Its three standalone checks cover duplicate direct-included
copies, unexecuted denominators, distinct macro-generated functions sharing all
source coordinates, inactive/test/generated source exclusion, and incompatible
branch lists. Replaying every report in the measured dev dataset twice leaves
all hit/total results unchanged while doubling source-copy counts.

The previous temporary summarizer keyed branches only by source/line/edge index.
Five GTK type-definition macros each generate distinct branch-bearing functions
on the same source line, causing ten edges (five hit) to be merged incorrectly.
Reprocessing the archived pre-build initial reports preserves the line/function
totals and corrects GTK branches to 590/1147 and combined branches to 1002/1762.
The initial and matrix tables above now use these corrected branch totals; this
is a reporting correction, not a new test run or production change.

Function identity now includes name and source span, preserving macro-generated
functions with identical coordinates. Branch identity also includes that function
and the line/edge index. Block IDs themselves differ in native versus fault-test
copies and are not comparable. Overlapping edge lists agree in this dataset;
the script rejects incompatible edge-list shapes. Current direct includes change
`calloc`/`free` and `gtk_drag_finish` callees, with no renamed production function
definitions. Future definition aliases require explicit normalization.

## Hosted confirmation of OpenType contracts and Area waits

The exact preceding tips completed Build + Release successfully:
[pre-build `1ce35c58`](https://github.com/kojix2/libui-ng/actions/runs/41200057416)
passed all 42 jobs, and
[dev `36083ec2`](https://github.com/kojix2/libui-ng/actions/runs/41200089048)
passed 41 jobs with the conditional release job skipped. These runs include
the native Windows/macOS configurations and Linux static/shared sanitizer jobs.
They supersede the pending hosted checks for the eight OpenType/attribute value
cases, two attrstr integration cases and conditional GTK Area wait change.
They do not cover the subsequent event/handler/Focus cleanup changes or the
optional coverage-report script added in this follow-up.

## Interactive tester directory

The tester's 23 C sources, private header, Windows resource/manifests and image
assets/generator move together from `test/` into `test/tester/` (32 existing
files). Its original target definition moves into that directory's CMake file;
the target remains `tester`, with the same artifact path, linkage and options.
Only the private header's public-header include and one diagnostic source path
change in the moved files. Image data, manifests, page logic and drawing cases
are preserved. `qa`, unit tests and measurement tools retain their locations.

On pre-build, Linux static/shared Debug builds of tester and all eleven CTest
targets pass. CTest names, commands and properties match the before-move lists.
In both builds, tester starts from `/tmp`, displays its main window, receives a
normal WM_DELETE_WINDOW event, reaches uiUninit and exits zero without a native
allocator leak diagnostic. These are startup/shutdown checks, not visual QA.
Tests-off and nested-consumer configurations still omit tester.

MinGW windres compiles both static/shared manifest variants before and after
the move to byte-identical resources. Native Windows/macOS builds and execution
of this directory change remain unverified locally; their CI configuration is
unchanged. Build, resource, file-comparison and smoke logs are in the local
`/tmp/libui-tester-move/` measurement directory.
