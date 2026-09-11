# libui-ng

Portable GUI library for C.

[![Build](https://github.com/kojix2/libui-ng/actions/workflows/build.yml/badge.svg)](https://github.com/kojix2/libui-ng/actions/workflows/build.yml)
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

Build automation is defined in `.github/workflows/build.yml`.

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

## Documentation and Examples

- API documentation: https://kojix2.github.io/libui-ng/
- API comments are in `ui.h`.
- Examples are under `examples`.
- Tests are under `test`.
- Old announcements are in [doc/old_news.md](doc/old_news.md).

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).
