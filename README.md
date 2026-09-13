# libui-ng

Portable GUI library for C.

[![Build](https://github.com/kojix2/libui-ng/actions/workflows/cmake.yml/badge.svg)](https://github.com/kojix2/libui-ng/actions/workflows/cmake.yml)
[![Docs](https://github.com/kojix2/libui-ng/actions/workflows/doxygen.yml/badge.svg)](https://github.com/kojix2/libui-ng/actions/workflows/doxygen.yml)
[![Top Language](https://img.shields.io/endpoint?url=https%3A%2F%2Ftokei.kojix2.net%2Fbadge%2Fgithub%2Fkojix2%2Flibui-ng%2Flanguage)](https://tokei.kojix2.net/github/kojix2/libui-ng)
[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/kojix2/libui-ng)

This repository provides unofficial kojix2 builds of libui-ng.
It builds and distributes patched binaries while maintaining API compatibility
with upstream libui-ng.

Releases are available at https://github.com/kojix2/libui-ng/releases.

Upstream projects:

- libui-ng: https://github.com/libui-ng/libui-ng
- libui: https://github.com/andlabs/libui

## Branches

- `pre-build`: build branch for release binaries, with downstream fixes and
  compatible API additions (`uiVersion`, `uiControlOnDestroyed`, `uiTabSelected`,
  `uiTabSetSelected`, `uiTabOnSelected`, and `uiGridDelete`). `uiControlDestroy`
  is also safe to call from user callbacks. Tags use `commit-xxxxxxx`.
- `dev`: development branch based on `pre-build`, with additional fixes and
  experimental APIs (`uiToolbar`, `uiImageView`, `uiDrawImage`). Tags use
  `commit-xxxxxxx-experimental`.

Pre-build branches may be rebased or force-pushed. Use release tags when you
need a stable reference. Build details and included changes are tracked by the
tags attached to each release.

Primary build and release automation is defined in `.github/workflows/cmake.yml`.

## Runtime Requirements

- Windows: Windows 7 SP1 or newer
- Unix: GTK+ 3.18 or newer
- macOS: 10.12 or newer

## Build Requirements

- CMake 3.15 or newer; macOS requires 3.16 or newer, and native Apple Silicon
  builds require 3.19.2 or newer
- Ninja is recommended; Unix Makefiles, Visual Studio, and Xcode generators are also supported
- Windows: Microsoft Visual Studio 2017 or newer, or MinGW-w64
- Unix: GTK+ development packages
- macOS: tools required to build Cocoa programs

MinGW-w64 builds currently support static libraries only.

## Build

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

With a multi-configuration generator such as Visual Studio or Xcode, select the
configuration at build and test time instead:

```sh
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Common options:

- `-DLIBUI_BUILD_TESTS=ON|OFF`
- `-DLIBUI_BUILD_EXAMPLES=ON|OFF`
- `-DLIBUI_FETCH_TEST_DEPS=ON|OFF`
- `-DCMAKE_BUILD_TYPE=Debug|Release` with single-configuration generators
- `-DBUILD_SHARED_LIBS=ON|OFF`

Example:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON
cmake --build build
```

## Test

```sh
ctest --test-dir build --output-on-failure
```

Manual QA tests are under `test/qa`.
Linux GUI tests can be run headlessly with
`xvfb-run ctest --test-dir build --output-on-failure`.

When tests are enabled, CMake first looks for cmocka 1.1.8 or newer. A top-level
build fetches the pinned 1.1.8 source if needed; nested builds never access the
network unless `LIBUI_FETCH_TEST_DEPS=ON` is explicitly requested.

## Install

```sh
cmake --install build
```

Set the install prefix during setup:

```sh
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr/local
```

Installed CMake packages can be consumed with:

```cmake
find_package(libui-ng CONFIG REQUIRED)
target_link_libraries(myapp PRIVATE libui::ui)
```

The same `libui::ui` target is available through `add_subdirectory()` and
`FetchContent_MakeAvailable()`.

## Quick Start

Create an application target and link it to libui:

```cmake
cmake_minimum_required(VERSION 3.15)
project(myapp LANGUAGES C)

include(FetchContent)
FetchContent_Declare(libui_ng
    GIT_REPOSITORY https://github.com/kojix2/libui-ng.git
    GIT_TAG pre-build
)
FetchContent_MakeAvailable(libui_ng)

add_executable(myapp main.c)
target_link_libraries(myapp PRIVATE libui::ui)
libui_configure_application(myapp)
```

`libui_configure_application()` applies the platform-specific build and runtime
settings needed by an application target. Applications that manage these
details themselves can omit it.
For reproducible builds, replace `pre-build` with an immutable tag from the
[release page](https://github.com/kojix2/libui-ng/releases).

When libui is brought in through `FetchContent` or `add_subdirectory`, its tests
and examples default to off. A top-level libui build enables them by default.

A minimal `main.c` is:

```c
#include <stdio.h>
#include <ui.h>

static int onClosing(uiWindow *window, void *data)
{
    (void) window;
    (void) data;
    uiQuit();
    return 1;
}

int main(void)
{
    uiInitOptions options = {0};
    const char *error = uiInit(&options);
    uiWindow *window;

    if (error != NULL) {
        fprintf(stderr, "libui initialization failed: %s\n", error);
        uiFreeInitError(error);
        return 1;
    }

    window = uiNewWindow("Hello", 320, 200, 0);
    uiWindowOnClosing(window, onClosing, NULL);
    uiControlShow(uiControl(window));
    uiMain();
    uiUninit();
    return 0;
}
```

## Binary SDKs

Release ZIP files are relocatable CMake install trees. They contain public
headers under `include/`, native libraries under `lib/` (and DLLs under `bin/`
on Windows), license metadata, and the installed `libui-ng` CMake package.
Static and shared SDKs are published separately for each supported platform,
architecture, and Debug/Release configuration.
MSVC static SDKs are additionally published in `md` (`/MD`) and `mt` (`/MT`)
variants; MSVC shared SDKs use `md`. Select the variant that matches the CRT
linkage of the consuming application.
The complete set of release asset names is recorded in
[`cmake/release-archives.txt`](cmake/release-archives.txt).

After extracting an SDK, CMake consumers can set its root as a prefix:

```sh
cmake -S app -B app-build -DCMAKE_PREFIX_PATH=/path/to/libui-ng-sdk
```

Non-CMake consumers, including Crystal and Ruby extensions, can use
`include/`, `lib/`, and `bin/` directly. Library filenames remain native to the
toolchain: MSVC uses `.lib`, MinGW and Unix static builds use `.a`, Windows
shared builds provide a `.dll` and import library, and Unix/macOS shared builds
provide `.so`/`.dylib` files.

## Documentation and Examples

- API documentation: https://kojix2.github.io/libui-ng/
- API comments are in `ui.h`.
- Examples are under `examples`.
- Tests are under `test`.
- Old announcements are in [doc/old_news.md](doc/old_news.md).

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).
