# Continuous Integration

Build automation is defined in `.github/workflows/build.yml`.

## References

- GitHub Actions matrix syntax: https://docs.github.com/en/actions/using-jobs/using-a-matrix-for-your-jobs
- GitHub-hosted runners: https://docs.github.com/en/actions/using-github-hosted-runners/about-github-hosted-runners
- CMake CI guidance: https://cmake.org/cmake/help/latest/guide/tutorial/Testing%20and%20CTest.html

## Triggers

The build workflow runs on:

- Pushes to `main`, `pre-build`, and `dev`
- Tags matching `commit-*`
- Pull requests targeting `main`
- Manual `workflow_dispatch`

## Build Matrix

The workflow currently runs 28 release configurations before any tag-only
packaging, plus minimum-CMake, Linux distribution, Clang, and Meson/CMake parity
checks:

- Ubuntu: 8 builds
- Windows MSVC: 8 builds
- Windows MinGW: 2 builds
- Windows UCRT: 2 builds
- macOS: 8 builds

Unix Makefiles are exercised by the minimum-CMake job. One MSVC configuration
also performs a Visual Studio generator smoke test, and one Apple Silicon
configuration performs an Xcode generator smoke test. Release Linux consumers
cover both static and shared relocated installs; the static configuration also
covers `FetchContent`.

### Ubuntu

- Runners: `ubuntu-latest`, `ubuntu-24.04-arm`
- Reported architectures: `x64`, `arm64`
- Library types: `static`, `shared`
- Build types: `release`, `debug`
- Extra packages: `libgtk-3-dev`, `xvfb`
- Tests: `xvfb-run ctest --test-dir builddir --output-on-failure`

### Windows MSVC

- Runner: `windows-latest`
- Architectures: `x86`, `x64`
- Library types: `static`, `shared`
- Build types: `release`, `debug`
- Toolchain setup: `TheMrMilchmann/setup-msvc-dev`
- MSVC builds select the corresponding static CRT with `CMAKE_MSVC_RUNTIME_LIBRARY`
- Tests: `ctest --test-dir builddir --output-on-failure`

### Windows MinGW

- Runner: `windows-latest`
- MSYS2 environment: `MINGW64`
- Architecture: `x64`
- Library type: `static`
- Build types: `release`, `debug`
- Tests: `ctest --test-dir builddir --output-on-failure`

### Windows UCRT

- Runner: `windows-latest`
- MSYS2 environment: `UCRT64`
- Architecture: `x64`
- Library type: `static`
- Build types: `release`, `debug`
- Tests: `ctest --test-dir builddir --output-on-failure`

### macOS

- Runners: `macos-15-intel`, `macos-latest`
- Reported architectures: `x64`, `arm64`
- Library types: `static`, `shared`
- Build types: `release`, `debug`
- Tests: `ctest --test-dir builddir --output-on-failure`

## Release Packaging

Every build invokes the `stage-legacy` target to produce the release-compatible
`builddir/meson-out` archive layout without changing CMake's native library
layout. When the workflow runs for a tag, the `release` job waits for all build
jobs, zips each staged artifact directory, and publishes a GitHub Release.

Tags whose names contain `experimental` are published as prereleases.
