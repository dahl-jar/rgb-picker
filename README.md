<p align="center">
  <img src="assets/app.ico" width="112" alt="RGB Picker">
</p>

<h1 align="center">RGB Picker</h1>

<p align="center">
  RGB Picker is a Windows app for controlling supported RGB hardware through selected OpenRGB drivers linked directly into the application.
</p>

## Stack

C++23 · Dear ImGui · OpenRGB · Win32 · DirectX 11 · CMake

## Features

- Saved lighting profiles and per-device colors
- Brightness, hardware modes, zone colors, and addressable-zone sizing
- Login startup and lighting restoration when hardware appears
- Lian Li controllers, Gigabyte RGB Fusion 2 boards, and MSI GPUs
- `rgb-ctl` for the same controls from a terminal

Settings and profiles are stored under `%APPDATA%\rgb-picker`.

## Download

Download the installer or portable ZIP from the [latest GitHub Release](https://github.com/dahl-jar/rgb-picker/releases/latest). Each release also includes SHA-256 checksums.

Release builds are unsigned, so Windows may show a SmartScreen warning when opening the installer.

## Requirements

- Windows 11 x64
- Supported RGB hardware
- Administrator access for hardware or service operations that require it

L-Connect 3 can overwrite lighting changes and block zone resizing. Stop its watcher and service with the commands in [DEVELOPMENT.md](DEVELOPMENT.md#l-connect-services).

## Build

Install CMake, Ninja, and a C++23-capable MinGW-w64 UCRT64 toolchain, then run:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The first GUI configuration fetches the pinned Dear ImGui revision. The selected OpenRGB drivers compile into `openrgb_drivers` and link into the backend.

## CLI

```text
rgb-ctl <command>

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
```

An addressable channel may report a size of zero until its LED count is set. Run `list` to check its supported range before using `resize`. SL V2 channels accept up to 96 LEDs; SL v1 channels accept up to 4 fans.

## Development

Architecture, linting, packaging, and hardware troubleshooting are documented in [DEVELOPMENT.md](DEVELOPMENT.md). Third-party source and local fork changes are recorded in [NOTICE.md](NOTICE.md).

## License

RGB Picker is licensed under [GPL-2.0-only](LICENSE). Third-party terms are listed in [NOTICE.md](NOTICE.md).
