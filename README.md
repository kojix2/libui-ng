# libui-ng

Portable GUI library for C.

[![Build](https://github.com/kojix2/libui-ng/actions/workflows/build.yml/badge.svg)](https://github.com/kojix2/libui-ng/actions/workflows/build.yml)
[![Docs](https://github.com/kojix2/libui-ng/actions/workflows/doxygen.yml/badge.svg)](https://github.com/kojix2/libui-ng/actions/workflows/doxygen.yml)

This repository provides unofficial kojix2 builds of libui-ng.
It builds and distributes patched binaries while maintaining API compatibility
with upstream libui-ng.

Releases are available at https://github.com/kojix2/libui-ng/releases.

Upstream projects:

- libui-ng: https://github.com/libui-ng/libui-ng
- libui: https://github.com/andlabs/libui

## Branches

- `pre-build`: build branch for release binaries. Tags use `commit-xxxxxxx`.
- `dev`: development branch with additional fixes and experimental image APIs
  (`uiImageView`, `uiDrawImage`). Tags use `commit-xxxxxxx-experimental`.

Pre-build branches may be rebased or force-pushed. Use release tags when you
need a stable reference. Build details and included changes are tracked by the
tags attached to each release.

Build automation is defined in `.github/workflows/build.yml`.

## Runtime Requirements

- Windows: Windows 7 SP1 or newer
- Unix: GTK+ 3.18 or newer
- macOS: OS X 10.11 or newer

## Build Requirements

- Meson 0.58.0 or newer
- Ninja, or another Meson backend
- Windows: Microsoft Visual Studio 2013 or newer, or MinGW-w64
- Unix: GTK+ development packages
- macOS: tools required to build Cocoa programs

MinGW-w64 builds currently support static libraries only.

## Build

```sh
meson setup build
ninja -C build
```

Build output is written to `build/meson-out/`.

Common options:

- `-Dtests=true|false`
- `-Dexamples=true|false`
- `--buildtype=debug|release|debugoptimized`
- `--default-library=shared|static`
- `--wrap-mode=forcefallback|nofallback|nodownload`

Example:

```sh
meson setup build --buildtype=release --default-library=shared
ninja -C build
```

## Test

```sh
meson test -C build
```

Manual QA tests are under `test/qa`.

## Install

```sh
ninja -C build install
```

Set the install prefix during setup:

```sh
meson setup build --prefix=/usr/local
```

## Documentation and Examples

- API documentation: https://kojix2.github.io/libui-ng/
- API comments are in `ui.h`.
- Examples are under `examples`.
- Tests are under `test`.
- Old announcements are in [doc/old_news.md](doc/old_news.md).

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).
