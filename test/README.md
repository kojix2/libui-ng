# Testing

The [baseline and test map](BASELINE.md) records the `pre-build` and `dev`
snapshots separately, including case counts and unverified configurations.

## Build and select tests

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=OFF \
  -DLIBUI_FETCH_TEST_DEPS=OFF
cmake --build build
ctest --test-dir build --show-only
ctest --test-dir build -L logic --output-on-failure
ctest --test-dir build -LE gui --output-on-failure
xvfb-run -a ctest --test-dir build -L gui --output-on-failure
xvfb-run -a ctest --test-dir build -R '^(datetimepicker|slider)-gtk$' --output-on-failure
```

Install cmocka >= 1.1.8 first, or explicitly enable `LIBUI_FETCH_TEST_DEPS`.
`LIBUI_BUILD_TESTS=OFF` disables the test dependency and support library.
Tests and dependency fetching remain off by default for nested consumers.

CTest labels describe independent attributes:

| Attribute | Labels | Meaning |
| --- | --- | --- |
| Subject | `logic`, `contract`, `backend`, `fault` | Shared algorithms, API contracts, native behavior, deliberate invalid allocation requests |
| Display | `gui` | Needs a usable native GUI session, even if no window is shown |
| Platform | `portable`, `gtk`, `windows`, `darwin` | Portable source or selected native implementation |
| libui linkage | `static`, `shared` | Tests linked to that libui variant; standalone source tests have neither label |

`alloc-unix` links GTK but does not need a display. `unit` combines API and
backend cases and therefore has both subject labels. Select individual tests
with `-R` when an executable has several subjects. CTest's `--show-only=json-v1`
also reports labels and timeouts. Platform conditions exclude unavailable
executables at configuration time; they are not runtime skips. Current
`pre-build` tests treat GUI initialization failures as failures, not skips.

The existing `unit` executable still runs every component by default. Its
optional arguments select whole components in their original registration
order; duplicates run once, and unknown names fail before running any tests:

```sh
build/artifacts/Debug/unit --list
xvfb-run -a build/artifacts/Debug/unit slider datetimepicker
```

Adjust the artifact path for the selected configuration (and use `unit.exe`
on Windows). A shared build omits the static-only `control` component from
`--list`. Component selection does not create extra CTest registrations.

## Add a test

Keep preparation and helpers before cases, followed by registration. Preserve
multi-operation regression scenarios. Prefer a new source boundary only when
its state, ownership and implementation dependency can be described separately.

`test/AddTest.cmake` supplies `libui_add_test(name sources...)`: executable,
internal compiler options, CTest registration and a 60-second timeout. Add
libraries, includes, native resources, labels, special timeouts and skip rules
with ordinary CMake commands at the call site in `unit/CMakeLists.txt`.
Standalone logic tests link cmocka and their implementation sources directly;
they do not link the GUI fixture.

`support/fixture.h` and `libui_test_support` provide the existing window fixture
without the `unit` runner. They are test-only and are not installed:

| Hook | Responsibility |
| --- | --- |
| Group `unitTestsSetup` / `unitTestsTeardown` | Allocate/free `struct state`; no libui initialization |
| Per-case `unitTestSetup` | Initialize libui, clear `c`, create a hidden window and install its close callback |
| Per-case `unitTestTeardown` | Attach `c`, show, enter stepping mode, run one blocking event step, destroy window/child, uninitialize; preserves the legacy order |
| Per-case `unitTestTeardownHidden` | Destroy window and already attached child, uninitialize; no show or event pumping |

Use `cmocka_unit_test_setup_teardown` for case cleanup after assertion failures.
With the hidden path, assign `state->c` and transfer ownership to `state->w`
with `uiWindowSetChild()` before assertions. The child must not be destroyed
separately. The legacy path retains ownership of `c` until teardown attaches
it. Keep feature-specific state local to the test; never retain a control
across cases. The GTK date/time and slider tests demonstrate the hidden path.
Native-widget discovery assertions belong in the case so teardown still runs
if discovery fails. Setup failure is not the same as a case assertion failure;
do not rely on per-case teardown after a failed setup.

The matrix component uses one runner-owned stack value, reset to identity
before each case through cmocka's initial-state support. It needs no GUI
initialization (`unit drawmatrix`). Matrix comparisons retain the relative
`1e-5` tolerance, report each differing component at double precision, and
preserve the assertion's caller location. Point/size cases verify coordinates,
the different treatment of translation, and that the input matrix is unchanged.

## Entry and Area backend boundaries

`unit/entry.c` and `unit/area.c` retain their public-API cases, fixture setup
and registration arrays. GTK-dependent regressions live in `entry_gtk.c`
(search-entry input and delayed notifications) and `area_gtk.c` (native scroll
adjustments and grab-broken events). CMake adds those sources to `unit` only
for non-Apple Unix builds. The private `entry_test.h`/`area_test.h` headers
provide state-access macros and declarations, without GTK headers in the
common files. Area's existing observation state is shared only with its native
cases, with the same reset points and storage lifetime; no production hook or
new shared state is introduced.

The CTest entry remains `unit`; `unit entry area` selects these components.
On GTK, Entry still has 20 cases and Area 7 (`dev`: Area 9). Registration names,
order and initialization are unchanged. The scroll-after-resize case now uses
the fixture window and its hidden teardown, so assertion failures also destroy
the child. It pumps nonblocking GTK events until the adjustment has the requested
range and visible rectangle, with a monotonic one-second limit per phase and
range/page/value diagnostics on timeout. This still requires a GUI
session for these fixtures; it does not make the API cases headless.

## Table test boundaries

`unit/table.c` retains the existing `table` component runner and the column,
selection and model notification scenarios. `table_value.c` contains value
round trips; `table_width.c` contains width and native width-regression cases.
`table_fixture.c` and the private `table_test.h` share the existing per-case
model/window/image fixture and event helpers. The executable, case names,
registration order, setup/teardown and bounded event stepping are unchanged.
Run `unit table` to select all of them. Value cases still use the original GUI
fixture; splitting a source file does not make that case headless.

## Sanitizers and coverage

```sh
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=OFF \
  '-DCMAKE_C_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer' \
  '-DCMAKE_CXX_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer'
cmake --build build-asan
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
  xvfb-run -a ctest --test-dir build-asan --output-on-failure
```

The `cmake-linux-sanitizers` job in `.github/workflows/cmake.yml` runs this
configuration for static and shared Debug builds on Linux. It instruments the
library, all tests and example sources with both sanitizers, then runs the full
CTest suite under Xvfb. Example execution and visual checks remain separate.
UBSan uses `halt_on_error=1:print_stacktrace=1` so a report fails the job with a
stack trace. Build/test logs and CMake configuration are retained as
`linux-sanitizers-<static|shared>-logs` artifacts when available. The release job
also depends on these checks; the sanitizer builds do not produce SDK archives.

`detect_leaks=0` is not a leak check. Record leak analysis separately, with
reasons for any external-library suppressions. Existing OS, architecture,
Debug/Release and static/shared CI jobs retain their configurations.

For GCC coverage, configure a separate static Debug build with
`-DCMAKE_C_FLAGS='--coverage -O0'` and matching C++ flags, build the CTest
targets, clear that build's `.gcda` files, then run CTest. Use matching `gcov`
on **all** `.gcno` files, including standalone tests and direct includes; missing
`.gcda` records remain in the denominator as unexecuted. From a clean report
directory, for example:

```sh
find ../build-coverage -name '*.gcno' -exec gcov --json-format --branch-counts \
  --branch-probabilities --preserve-paths {} + > gcov.log 2>&1
python3 ../test/coverage.py . --backend unix --output summary.json
python3 ../test/test_coverage.py
```

Adjust paths to the actual build/source tree. The optional Python script uses
only the standard library and is not a build or CTest dependency. Its default
source root is the repository containing the script. Process reports from one
source snapshot, compiler version and configuration; do not mix stale reports.
Only `common/` and the selected backend under that root count, including their
internal headers. Tests, examples, dependencies, separate generated sources
(including `version.c`) and inactive backends are excluded.

Line hits merge by source path and line. Functions merge by name and full
source span; macro-generated functions on the same line remain distinct.
Branches merge by that function identity, source line and edge position. GCC
block IDs can shift between library and fault-test copies, so the script checks
that overlapping edge lists have matching lengths/fallthrough/throw flags and
rejects incompatible lists instead of inventing a total. Current direct includes
rename allocator/protocol callees, not production function definitions. If a
future test renames definitions, normalize those aliases explicitly before
aggregation; do not infer aliases from coordinates shared by macro functions.

A key is hit if any copy executes it; execution counts are not added across
copies. The JSON records merged totals, per-source counts and copy counts.
Keep original gcov reports/logs and record the tested revision separately.

## OpenType and attribute contracts

`opentype-unit` runs eight portable cases without libui initialization, a display
or backend linkage. It compiles `common/opentype.c` and `common/attribute.c`
with a local allocation counter; teardown frees case-owned features, clones and
attributes after assertions and requires no live allocations. Native allocator
behavior and family-name comparison remain covered by their existing tests.
The test never substitutes for platform font shaping or text-layout checks.

The cases cover empty/missing tags (including unchanged output), duplicate
replacement, zero/UINT32_MAX values, growth/removal, clone independence,
unordered enumeration and early stop. Attribute cases cover copied family
names, fractional size, weight boundaries and an unnamed valid weight, all
italic/stretch/underline modes, RGBA values and owned feature copies. Background
colors have no documented public getter; `unit attrstr` checks their equality
through run merging, along with feature-copy/merge/delete ownership and empty
runs. These two integration cases retain the existing native initialization;
only the portable executable is headless.

```sh
ctest --test-dir build -R '^opentype-unit$' --output-on-failure
xvfb-run -a build/artifacts/Debug/unit attrstr
```

## Consumer and manual checks

`cmake/consumer/` exercises `add_subdirectory`, FetchContent, installed-package
and platform-extension consumers. These checks and direct C linkage are run by
`.github/workflows/cmake.yml` outside the unit CTest list. Existing CI retains
its OS, architecture, Debug/Release and static/shared matrix.

`tester/` contains the interactive `tester` program: its page sources, drawing
scenarios, embedded image data and source images, plus Windows resources and
manifests. Build with `cmake --build build --target tester`; the executable
remains `build/artifacts/<configuration>/tester` (`tester.exe` on Windows).

`tester`, `qa`, and examples are interactive programs, not CTest cases. Run
them on the target OS and record the control, operation and expected display
or behavior. A forced termination only establishes that a program started;
it does not establish correct shutdown or visual behavior.
