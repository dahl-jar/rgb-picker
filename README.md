# rgb-picker

`rgb-picker` is a Windows desktop app for controlling RGB hardware. `rgb-ctl` provides the same
controls from the command line. Both use an in-process backend built from selected drivers in the
OpenRGB fork under `third_party/openrgb-src`.

The desktop app supports saved profiles, per-device colors, brightness, hardware modes, zone
resizing, login startup, and session restoration. Settings and profiles are stored under
`%APPDATA%\rgb-picker`.

Windows builds include drivers for Lian Li controllers, Gigabyte RGB Fusion 2 boards, and MSI
GPUs. The simulator runs on Windows, macOS, and Linux.

Development notes are in [DEVELOPMENT.md](DEVELOPMENT.md). Third-party source and local fork
changes are recorded in [NOTICE.md](NOTICE.md). Release history is in
[CHANGELOG.md](CHANGELOG.md).

## Requirements

- CMake 3.16 or newer
- A compiler and standard library with C++23 support
- Windows hardware builds: MinGW-w64 and the Windows SDK libraries
- Windows GUI builds: network access during the first configure so CMake can fetch Dear ImGui

Hardware detection is built on Windows. macOS and Linux builds use the simulator.

## Build

```sh
# configure the CLI and tests
cmake -S . -B build -DRGBPICKER_BUILD_GUI=OFF -DBUILD_TESTING=ON

# compile and run the tests
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

`RGBPICKER_BUILD_GUI` defaults to `ON` on Windows. The Windows build compiles the selected
OpenRGB drivers into `openrgb_drivers` and links that library into the backend.

## CLI

```text
rgb-ctl [--simulate] <command>

  list                                 show devices, zones, and modes
  set <color> [device-filter]          set a device color
  zone <device> <zone> <color>         set a zone color
  resize <device> <zone> <led-count>   resize a controller zone
  mode <device> <mode-name>            activate a hardware mode
  rainbow [device-filter]              run a rainbow until interrupted
```

Colors may be names such as `red`, `purple`, or `off`, or hexadecimal values such as `#20a0f0`.

```sh
./build/rgb-ctl list
./build/rgb-ctl set purple
./build/rgb-ctl set "#00c8ff" "sl v2"
./build/rgb-ctl --simulate list
```

Each simulator process starts with the same in-memory device set.

## Zone sizes

An addressable channel may report a size of zero until its LED count is set. Check the supported
range before resizing it:

```sh
./build/rgb-ctl list
./build/rgb-ctl resize 1 0 64
```

SL V2 channels accept up to 96 LEDs. SL v1 channels accept up to 4 fans.

## L-Connect conflict

L-Connect 3 can overwrite lighting changes and block zone resizing. Stop its watcher and service
using the commands in [DEVELOPMENT.md](DEVELOPMENT.md#l-connect-services). The hubs retain the last
fan speed after these services stop.

`rgb-picker` is licensed under GPL-2.0-only. See [LICENSE](LICENSE) and [NOTICE.md](NOTICE.md).
