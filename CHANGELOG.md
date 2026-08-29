# Changelog

## 2026-08-29

### Added

- Added CI jobs for Clang-Tidy, warnings-as-errors builds, Linux and macOS tests, memory-safety checks, and the full Windows target.
- Added tagged GitHub Release publishing with a Windows installer, portable ZIP, and SHA-256 checksums.
- Added CPack install rules for executables, runtime DLLs, fonts, notices, and third-party license texts.

### Changed

- Moved feature tests beside their production code. Shared test infrastructure remains under `tests/`.
- Split the GUI into app, devices, profiles, settings, and UI feature folders.
- Pinned Dear ImGui 1.91.8 to its commit and completed binary-distribution notices.

### Removed

- Removed the simulator backend, simulator-only tests, and the `rgb-ctl --simulate` option.

### Verified

- 67 tests pass in normal and AddressSanitizer/UndefinedBehaviorSanitizer builds.
- Strict compiler warnings, GitHub workflow linting, Windows resource generation, and CPack metadata checks pass.

## 2026-07-27

### Added

- Compiled selected OpenRGB drivers into the Windows backend. The build includes Lian Li
  controllers, Gigabyte RGB Fusion 2 boards, and MSI GPUs.
- `DriverBackend` adapts one OpenRGB controller. `MergedBackend` assigns device IDs and routes
  writes.
- Zone sizes persist in `layout.ini`, and driver settings persist in `driver-settings.json`.
- Login startup retries profile restoration while hardware appears.

### Changed

- Replaced the OpenRGB TCP SDK runtime and handwritten hardware backends with linked drivers from
  `third_party/openrgb-src/`.
- Consolidated OpenRGB source under `third_party/openrgb-src/`. `third_party/openrgb/` contains
  project-owned shims.
- Replaced `ConnectionSupervisor` with `BackendSession` for backend creation, validation, and
  retries.
- Split backend code into device, factory, hardware, session, and simulation feature folders.
  Tests mirror the same layout.
- Moved device models, modes, and zone names to OpenRGB registrations. Profiles using old device
  or zone names must be saved again.
- Login startup restores the selected profile. The last applied look is used when no profile is
  selected.

### Fixed

- Corrected `LianLiUniHubSLController::SendColor` to pass the allocated buffer length to
  `hid_write`. The old pointer-size write sent only two LEDs on a 64-bit build.
- Allowed `RGBController_LianLiUniHubSL::DeviceUpdateMode` to write `Custom` mode at index 0.
- Linked `openrgb_drivers` as a whole archive so static detector registrations reach the binary.
- Built the vendored libusb import library from its definition file. Windows executables load the
  bundled `libusb-1.0.dll`.
- Login restoration refreshes discovery for late hardware and continues after a device write
  fails.
- The Windows Run entry is rewritten when its command or `--startminimized` flag changes.
- I2C bus detection runs once per process, preventing duplicate bus registrations after reconnects.

### Verified

- 78 contract tests pass in normal and sanitizer builds.
- The MinGW `openrgb_drivers` target builds from the fork and project shims.
- Lian Li hubs, the Gigabyte board, and the MSI GPU were detected and controlled through the
  linked driver backend.

## 2026-07-26

### Added

- Stored the complete OpenRGB source snapshot under `third_party/openrgb-src/`.
- Integrated in-process drivers for the Uni Hub families, Gigabyte RGB Fusion 2, and MSI GPU
  lighting.
- Integrated Nvidia I2C access through OpenRGB's SMBus and NvAPI code.
- Closing the window hides the app, and `--startminimized` starts it in the
  notification area.
- Uni Hub SL V2 channels support per-LED control for up to 96 LEDs.
- Exposed the motherboard's onboard zones and ARGB headers.
- Active profiles, applied colors, and zone layouts persist under `%APPDATA%\rgb-picker`.

### Changed

- Profile saving updates the active profile when the name field is empty. A typed name creates a
  new profile.
- The selected profile and applied colors are written as changes settle, avoiding a file write for
  every picker step.
- The profile rail shows drift between the saved profile and the colors on the hardware.
- The app restores per-device colors during login startup and reconnects.

### Fixed

- MSI GPU static color writes select programming mode, write RGB, set brightness, and select
  static mode in the required order.
- Gigabyte motherboard ARGB headers use their hardware indexes and disable built-in effects
  before direct color writes.
- Uni Hub SL color writes reach the driver mode update path.
- Applied-color restoration runs once per connection attempt and does not repaint over later
  external changes.

## 2026-07-25

### Added

- Implemented OpenRGB SDK connection startup, reconnects, device enumeration, and capped retry
  delays.
- Implemented Windows OpenRGB process startup and `RGBPICKER_OPENRGB_PATH` lookup.
- Added `rgb-ctl --no-auto-start` for connection failures without process startup.
- Added a settings dialog with Windows login startup and color restoration.
- Added profile persistence and shared key-value configuration parsing.
- Bundled Roboto and copied it beside the application during the build.

### Changed

- Renamed the project to `rgb-picker`, with the `rgbpicker` namespace and `RGBPICKER_*` CMake
  options.
- Reworked the window into a profile rail, color workspace, properties panel, toolbar, and status
  bar.
- Split GUI code into theme, widgets, worker, panel, and Win32 host files.
- Moved shared look sorting and configuration paths out of the GUI.

### Verified

- Windows login startup writes and removes the Run entry.
- Settings round-trip through `%APPDATA%\rgb-picker\settings.ini`.
- 63 contract tests passed under MinGW GCC 14.2.

## 2026-07-20

### Added

- Created the Dear ImGui Windows desktop app with Win32 and DirectX 11 rendering.
- Added device and zone selection, RGB and HSV controls, hex input, presets, brightness, hardware
  modes, zone resizing, and rainbow animation.
- Moved backend calls to a worker thread so rendering stays responsive during device traffic.
- Added DPI-aware fonts, a Windows theme-aware title bar, an app icon, version information, and a
  Start Menu shortcut.
- Added deterministic Uni Hub simulator fixtures and offline simulator tests.

### Fixed

- Replaced `std::find_if` over `orgb::PointerIterator` with a range loop for GCC 14 compatibility.

### Verified

- The GUI controlled both Uni Hubs, the RTX 5080, and the B850 Aorus through a live OpenRGB server.
- A two-minute rainbow run held process memory and handle counts steady.

## 2026-07-17

### Added

- Created the C++23 `rgb-ctl` command-line tool using the OpenRGB SDK server.
- Added `list`, `set`, `zone`, `resize`, `mode`, and `rainbow` commands.
- Added named and hexadecimal color parsing, HSV rainbow animation, and request error messages.
- Added remote host and port options, MinGW and macOS build instructions, zone sizing notes, and
  L-Connect service instructions.

### Verified

- Built with MinGW GCC 14.2 and CMake 4.4 on Windows 11.
- Controlled Uni Hub SL V2, Uni Hub SL v1, MSI RTX 5080 Gaming Trio OC, and Gigabyte B850 Aorus
  Elite WiFi7 ICE hardware.
