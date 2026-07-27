# Development

## Architecture

The backend contract lives in `include/rgbpicker/backend.h`. The GUI, CLI, session lifecycle, and
tests depend on this interface.

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

`DriverBackend` adapts one OpenRGB `RGBController` to the application backend. `MergedBackend`
combines detected controllers, assigns application device IDs, and routes writes to their owning
controller. `BackendSession` creates the selected backend and manages retries.

Fork-specific includes stay in `src/backend/hardware/driver_backend.cpp` and the Windows CMake
configuration. The rest of the application uses the backend contract.

## Source layout

- `include/rgbpicker/`: public interfaces and data types
- `src/backend/`: device rules, runtime factory, hardware adapters, session, and simulator
- `src/cli/`: command parsing and execution
- `src/storage/`: settings, profiles, applied colors, and zone layouts
- `src/gui/`: Win32 and Dear ImGui code
- `tests/`: feature folders matching the production layout
- `third_party/openrgb-src/`: OpenRGB source fork
- `third_party/openrgb/shim/`: interfaces needed by the compiled driver set

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

The GUI links `rgbpicker_backend` and `rgbpicker_storage`. Tests link `rgbpicker_cli`,
`rgbpicker_backend`, `rgbpicker_storage`, and `rgbpicker_core` through target dependencies.

## Backend lifecycle

`BackendSession` creates the hardware backend and accepts it after discovery returns a device. An
unavailable backend is recreated with capped exponential backoff. The clock and sleeper are
injected so tests can advance time directly.

The GUI worker owns calls to the live backend. It drains queued operations, refreshes discovery,
restores zone sizes before colors, and publishes device snapshots to the render thread.

## Build and test

```sh
# configure a simulator build
cmake -S . -B build-sim \
  -DRGBPICKER_BUILD_GUI=OFF \
  -DBUILD_TESTING=ON \
  -DCMAKE_BUILD_TYPE=Debug

# compile, test, and list simulated devices
cmake --build build-sim --parallel
ctest --test-dir build-sim --output-on-failure
./build-sim/rgb-ctl --simulate list
```

The automated suite uses simulated hardware. Physical-device checks run on Windows with the USB
controllers, firmware, and display driver present.

## Adding a driver

Keep upstream source changes under `third_party/openrgb-src`. Add the driver's translation units,
include paths, and system libraries to `openrgb_drivers` in `CMakeLists.txt`.

The detector must register an `RGBController`. The whole-archive link keeps static detector
registrations in the final binary. `DriverBackend` supplies the application adapter and
`MergedBackend` supplies device IDs and routing.

Record local fork changes and upstream commit details in `NOTICE.md`.

## L-Connect services

L-Connect 3 can overwrite effects and block resizing. Stop its watcher before its service from an
administrator PowerShell:

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
