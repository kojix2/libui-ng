# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Changed

- CMake is now the primary build system and provides the `libui::ui` target for source and installed-package consumers.
- The minimum supported Microsoft toolchain is now Visual Studio 2017; Visual Studio 2013 support has ended.
- uiControlDestroy() now safely defers destruction requested from user callbacks.
- uiFreeFontDescriptor() is now the common release function for all font descriptors filled by libui; uiFreeFontButtonFont() remains a compatibility alias.

### Added

- uiVersion() API
- uiWindowPosition() API
- uiWindowSetPosition() API
- uiWindowOnPositionChanged() API
- Manual testing suite for quality assurance
- uiTableSelection API
- uiTableOnRowClicked() API
- uiTableOnRowDoubleClicked() API
- Unit testing framework with cmocka
- Doxygen documentation for most uiControls
- uiWindowOnFocusChanged() API
- uiWindowFocused() API
- Build for ARM and x86 on macOS by default
- Arch Linux AUR package for ARM and x86 ([libui-ng-git](https://aur.archlinux.org/packages/libui-ng-git))
- uiSliderOnReleased() API
- GitLab repository [mirror](https://gitlab.com/libui-ng/libui-ng)
- uiTableColumnWidth() API
- uiTableColumnSetWidth() API
- uiComboboxNumItems() API
- uiOpenFolder() API
- uiSliderHasToolTip() API
- uiSliderSetHasToolTip() API
- uiSliderSetRange() API
- uiTableHeaderSortIndicator() API
- uiTableHeaderSetSortIndicator() API
- uiTableHeaderOnClicked() API
- uiTableHeaderVisible() API
- uiTableHeaderSetVisible() API
- uiBoxNumChildren() API
- uiFormNumChildren() API
- uiComboboxInsertAt() API
- uiComboboxDelete() API
- uiComboboxClear() API
- uiWindowResizeable() API
- uiWindowSetResizeable() API
- uiDrawPathEnded() API
- Automatic dark mode detection on macOS
- uiFreeFontDescriptor() API
- uiLoadControlFont() API
- Doxygen documentation generator
- GitHub Actions CI

### Removed
- AppVeyor and Azure Pipelines CI
