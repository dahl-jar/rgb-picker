# Development

## Architecture

The backend contract lives in `include/rgbpicker/backend.h`. GUI, CLI, session lifecycle, and tests depend on this interface.

```text
third_party/openrgb-src
          |
          v
   openrgb_drivers
          |
          v
    DriverBackend
          |
          v
    MergedBackend
          |
          v
    BackendSession
       /       \
      v         v
  rgb-picker  rgb-ctl
```

`DriverBackend` adapts one OpenRGB `RGBController` to the application backend. `MergedBackend` combines detected controllers, assigns application device IDs, and routes writes to their owning controller. `BackendSession` creates and validates the hardware backend, then manages retries.

Fork-specific includes stay in `src/backend/hardware/driver_backend.cpp` and the Windows CMake configuration. The rest of the application uses the backend contract.

## Source layout

- `include/rgbpicker/`: public interfaces and data types
- `src/backend/device/`: device rules and mode selection
- `src/backend/factory/`: runtime backend construction
- `src/backend/hardware/`: OpenRGB adapters, discovery, and backend composition
- `src/backend/session/`: connection lifecycle and retry policy
- `src/cli/`: command parsing, execution, and co-located contract tests
- `src/core/`: shared color behavior and its tests
- `src/storage/`: settings, profiles, applied colors, zone layouts, and tests
- `src/gui/app/`: UI state, frame composition, picker session, and worker lifecycle
- `src/gui/devices/`: device properties and color workspace
- `src/gui/profiles/`: profile rail
- `src/gui/settings/`: settings dialog
- `src/gui/ui/`: theme, fonts, and shared ImGui primitives
- `tests/`: test runner and shared recording fixtures
- `third_party/openrgb-src/`: OpenRGB source fork
- `third_party/openrgb/shim/`: interfaces needed by the compiled driver set

Feature tests live beside the production code they verify. Only the runner and shared fixtures stay under `tests/`.

The main CMake targets follow this dependency order:

```text
rgbpicker_core
    ^
rgbpicker_storage
    ^
rgbpicker_backend
    ^
rgbpicker_cli
```

The GUI links `rgbpicker_backend` and `rgbpicker_storage`. Tests link the same target graph through `rgbpicker_cli`.

## Backend lifecycle

`BackendSession` accepts a hardware backend only after discovery returns at least one device. An unavailable backend is recreated with capped exponential backoff. The clock and sleeper are injected so tests can advance time directly.

The GUI worker owns calls to the live backend. It drains queued operations, refreshes discovery, restores zone sizes before colors, and publishes device snapshots to the render thread.

## Build and test

A portable build compiles the CLI, first-party backend contracts, storage, and tests without the Windows GUI:

```sh
cmake -S . -B build \
  -DRGBPICKER_BUILD_GUI=OFF \
  -DBUILD_TESTING=ON \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Tests use recording backends and do not require physical devices. Hardware checks run on Windows with the controllers, firmware, and display driver present.

## Lint

Install Clang-Tidy and configure the strict build:

```sh
cmake -S . -B build-lint \
  -DRGBPICKER_BUILD_GUI=OFF \
  -DRGBPICKER_ENABLE_CLANG_TIDY=ON \
  -DRGBPICKER_WARNINGS_AS_ERRORS=ON \
  -DBUILD_TESTING=ON
cmake --build build-lint --parallel
```

The lint configuration covers first-party C++ with Clang analyzer, bug-prone, performance, and portability checks. Vendored OpenRGB and hidapi sources are excluded.

## Package

Windows release builds use MSYS2 UCRT64 with GCC, CMake, Ninja, and NSIS:

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DRGBPICKER_REQUIRE_MINGW_RUNTIME_DLLS=ON \
  -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
cpack --config build/CPackConfig.cmake -G ZIP -B artifacts
cpack --config build/CPackConfig.cmake -G NSIS -B artifacts
```

Both packages contain the GUI, CLI, libusb, MinGW runtime DLLs, fonts, notices, and license texts. The NSIS package adds an RGB Picker Start Menu entry.

Every green `main` build updates the rolling `latest` GitHub Release with both packages and their SHA-256 checksums. The `latest` tag moves to the successful commit rather than creating a release for every push.

Tags matching `vMAJOR.MINOR.PATCH` remain available for versioned releases. They rebuild and test the Windows target, then attach the packages and checksums to the matching GitHub Release.

## Adding a driver

Keep upstream source changes under `third_party/openrgb-src`. Add the driver's translation units, include paths, and system libraries to `openrgb_drivers` in `CMakeLists.txt`.

The detector must register an `RGBController`. The whole-archive link keeps static detector registrations in the final binary. `DriverBackend` supplies the application adapter and `MergedBackend` supplies device IDs and routing.

Record local fork changes and upstream commit details in `NOTICE.md`.

## L-Connect services

L-Connect 3 can overwrite effects and block resizing. Stop its watcher before its service from an administrator PowerShell:

```powershell
Stop-Service LConnectServiceWatcher
Stop-Service LConnectService
Stop-Process -Name "L-Connect 3" -Force -ErrorAction SilentlyContinue
```

The hubs retain the last fan speed after these services stop.

## Troubleshooting

- No devices appear: check Windows device detection and the driver's `openrgb_drivers` entries.
- A color command leaves a channel dark: inspect the zone and set its size.
- Colors change again after a few seconds: stop the L-Connect watcher and service.
