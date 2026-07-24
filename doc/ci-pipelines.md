# Continuous Integration

Build automation is defined in `.github/workflows/build.yml`.

## References

- GitHub Actions matrix syntax: https://docs.github.com/en/actions/using-jobs/using-a-matrix-for-your-jobs
- GitHub-hosted runners: https://docs.github.com/en/actions/using-github-hosted-runners/about-github-hosted-runners
- Meson CI notes: https://mesonbuild.com/Continuous-Integration.html

## Triggers

The build workflow runs on:

- Pushes to `main`, `pre-build`, and `dev`
- Tags matching `commit-*`
- Pull requests targeting `main`
- Manual `workflow_dispatch`

## Build Matrix

The workflow currently runs 28 build configurations before any tag-only release
packaging:

- Ubuntu: 8 builds
- Windows MSVC: 8 builds
- Windows MinGW: 2 builds
- Windows UCRT: 2 builds
- macOS: 8 builds

### Ubuntu

- Runners: `ubuntu-latest`, `ubuntu-24.04-arm`
- Reported architectures: `x64`, `arm64`
- Library types: `static`, `shared`
- Build types: `release`, `debug`
- Extra packages: `libgtk-3-dev`, `xvfb`
- Tests: `xvfb-run meson test -C builddir --verbose`

### Windows MSVC

- Runner: `windows-latest`
- Architectures: `x86`, `x64`
- Library types: `static`, `shared`
- Build types: `release`, `debug`
- Toolchain setup: `TheMrMilchmann/setup-msvc-dev`
- Meson setup also passes `-Db_vscrt=mt`
- Tests: `meson test -C builddir --verbose`

### Windows MinGW

- Runner: `windows-latest`
- MSYS2 environment: `MINGW64`
- Architecture: `x64`
- Library type: `static`
- Build types: `release`, `debug`
- Tests: `meson test -C builddir --verbose`

### Windows UCRT

- Runner: `windows-latest`
- MSYS2 environment: `UCRT64`
- Architecture: `x64`
- Library type: `static`
- Build types: `release`, `debug`
- Tests: `meson test -C builddir --verbose`

### macOS

- Runners: `macos-15-intel`, `macos-latest`
- Reported architectures: `x64`, `arm64`
- Library types: `static`, `shared`
- Build types: `release`, `debug`
- Tests: `meson test -C builddir --verbose`

## Release Packaging

When the workflow runs for a tag, the `release` job waits for all build jobs,
downloads their artifacts, zips each platform artifact directory, and publishes
a GitHub Release with `softprops/action-gh-release`.

Tags whose names contain `experimental` are published as prereleases.
