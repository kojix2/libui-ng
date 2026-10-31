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

## Sanitizers and coverage

```sh
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=OFF \
  '-DCMAKE_C_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer' \
  '-DCMAKE_CXX_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer'
cmake --build build-asan
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 \
  xvfb-run -a ctest --test-dir build-asan --output-on-failure
```

This checks address/undefined behavior errors, including instrumented test and
example sources. `detect_leaks=0` is not a leak check. Record leak analysis
separately, with reasons for any external-library suppressions.

For GCC coverage, configure a separate static Debug build with
`-DCMAKE_C_FLAGS='--coverage -O0'`, build, and run CTest. Clear that build's
`.gcda` files before an independent measurement. Use matching `gcov` with
`--json-format --branch-counts --branch-probabilities --preserve-paths` on all
`.gcno` files. Count production `common/` and the active backend only; exclude
tests, examples, dependencies and generated `version.c`. Merge repeated source
records by source path and line/function/branch identity before counting hits;
do not count the library and standalone source copies twice. Keep detailed JSON
and logs in the build directory and record comparable totals in the baseline.

## Consumer and manual checks

`cmake/consumer/` exercises `add_subdirectory`, FetchContent, installed-package
and platform-extension consumers. These checks and direct C linkage are run by
`.github/workflows/cmake.yml` outside the unit CTest list. Existing CI retains
its OS, architecture, Debug/Release and static/shared matrix.

`tester`, `qa`, and examples are interactive programs, not CTest cases. Run
them on the target OS and record the control, operation and expected display
or behavior. A forced termination only establishes that a program started;
it does not establish correct shutdown or visual behavior.
