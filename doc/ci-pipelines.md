# Continuous Integration

Primary build and release automation is defined in `.github/workflows/cmake.yml`.

## References

- GitHub Actions matrix syntax: https://docs.github.com/en/actions/using-jobs/using-a-matrix-for-your-jobs
- GitHub-hosted runners: https://docs.github.com/en/actions/using-github-hosted-runners/about-github-hosted-runners
- CMake CI guidance: https://cmake.org/cmake/help/latest/guide/tutorial/Testing%20and%20CTest.html

## Triggers

The build workflow runs on:

- Pushes to `main`, `pre-build`, `dev`, and `cmake-migration`
- Tags matching `commit-*`
- Pull requests targeting `main`
- Manual `workflow_dispatch`

## Build Matrix

The workflow currently runs 28 release configurations before packaging, plus
minimum-CMake, Linux distribution, and Clang checks:

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
- Build types: `Release`, `Debug`
- Extra packages: `libgtk-3-dev`, `xvfb`
- Tests: `xvfb-run ctest --test-dir builddir --output-on-failure`

### Windows MSVC

- Runner: `windows-latest`
- Architectures: `x86`, `x64`
- Library types: `static`, `shared`
- Build types: `Release`, `Debug`
- Toolchain setup: `TheMrMilchmann/setup-msvc-dev`
- MSVC builds select the corresponding static CRT with `CMAKE_MSVC_RUNTIME_LIBRARY`
- Tests: `ctest --test-dir builddir --output-on-failure`

### Windows MinGW

- Runner: `windows-latest`
- MSYS2 environment: `MINGW64`
- Architecture: `x64`
- Library type: `static`
- Build types: `Release`, `Debug`
- Tests: `ctest --test-dir builddir --output-on-failure`

### Windows UCRT

- Runner: `windows-latest`
- MSYS2 environment: `UCRT64`
- Architecture: `x64`
- Library type: `static`
- Build types: `Release`, `Debug`
- Tests: `ctest --test-dir builddir --output-on-failure`

### macOS

- Runners: `macos-15-intel`, `macos-latest`
- Reported architectures: `x64`, `arm64`
- Library types: `static`, `shared`
- Build types: `Release`, `Debug`
- Tests: `ctest --test-dir builddir --output-on-failure`

## Release Packaging

Every matrix build runs CPack's ZIP generator against the project's CMake
install rules. Each resulting binary SDK is a relocatable install prefix with
headers in `include/`, native libraries in `lib/`, Windows runtime DLLs in
`bin/`, and exported CMake package files in `lib/cmake/libui-ng/`. Tests,
examples, build trees, and CI logs are not included in the SDK.

When the workflow runs for a tag, the `release` job waits for all 28 packages,
checks their names against `cmake/release-archives.txt`, validates their
contents, rejects legacy/build-tree paths, and
tests relocated CMake, direct C, Crystal, and Ruby Fiddle consumers before
publishing a GitHub Release. The same packaging and consumer validation runs on the
`cmake-migration` branch without publishing a release.

Tags whose names contain `experimental` are published as prereleases.
