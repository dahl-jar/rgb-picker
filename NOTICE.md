# Third-party code

## OpenRGB

`rgb-picker` includes OpenRGB device drivers and is licensed under GPL-2.0-only. [LICENSE](LICENSE)
contains OpenRGB's copy of the license and applies to the complete repository.

- Upstream: <https://codeberg.org/OpenRGB/OpenRGB>
- Commit: `394a5c8eb0d328c15e557b9639fc679fd388fd86` from 2026-07-26
- Copyright: Adam Honse and the OpenRGB contributors

The upstream snapshot is stored in `third_party/openrgb-src/`. Local driver fixes are applied to
that tree. The Windows build compiles these parts:

| Source | Use |
|---|---|
| `RGBController/`, `StringUtils.{h,cpp}` | OpenRGB device model and string helpers |
| `Controllers/LianLiController/` | Lian Li hubs, Strimer, coolers, and screens |
| `Controllers/GigabyteRGBFusion2USBController/` | Gigabyte RGB Fusion 2 boards |
| `Controllers/MSIGPUController/MSIGPUv2Controller/` | MSI GPU lighting |
| `dmiinfo/`, `wmi/` | Motherboard model detection |
| `i2c_smbus/`, `dependencies/NVFC/` | Nvidia I2C access |
| `dependencies/json/nlohmann/json.hpp` | JSON type used by OpenRGB interfaces |
| `dependencies/libusb-1.0.27/` | Raw USB transfers used by the first Uni Hub |

### Local modifications

#### Uni Hub SL color buffer length

`LianLiUniHubSLController::SendColor` passed `sizeof(buf)` to `hid_write`, where `buf` is a pointer.
The tested 64-bit build sent 8 bytes, enough for the report header and two LEDs. The local fix
passes the allocated buffer length:

```cpp
const size_t buf_len = 2 + num_colors;
hid_write(this->device, buf, buf_len);
```

The fix was checked on a Uni Hub SL with firmware v1.8.

#### Uni Hub SL mode 0 writes

`RGBController_LianLiUniHubSL::DeviceUpdateMode` returned before writing when `active_mode == 0`.
`Custom` occupies index 0 and uses the same static per-LED mode as `Static`. Removing the early
return allows color writes in `Custom` mode.

### Shims

Project-owned compatibility files live in `third_party/openrgb/shim/`:

- `DetectionManager.h` stores HID, I2C PCI, and I2C bus detector registrations.
- `LogManager.h` keeps driver log calls type-checked and discards their output.
- `SettingsManager.h` loads and saves driver settings through an injected store.
- `ResourceManager.h` exposes the settings manager to drivers.
- `comsupp_mingw.cpp` defines the COM string conversions missing from MinGW runtime libraries.

## nlohmann/json

- Source: `third_party/openrgb-src/dependencies/json/nlohmann/json.hpp`
- License: MIT
- Copyright: 2013-2022 Niels Lohmann

## libusb

- Source: `third_party/openrgb-src/dependencies/libusb-1.0.27/`
- Upstream: <https://github.com/libusb/libusb>
- Version: 1.0.27
- License: LGPL-2.1-or-later
- License text: `third_party/openrgb-src/dependencies/libusb-1.0.27/COPYING`

## hidapi

- Source: `third_party/hidapi/`
- Upstream: <https://github.com/libusb/hidapi>
- License choice: GPL-3.0, BSD-3-Clause, or the original HIDAPI license
- Selected license: BSD-3-Clause
- License text: `third_party/hidapi/LICENSE-bsd.txt`

## Dear ImGui

- Upstream: <https://github.com/ocornut/imgui>
- Version: 1.91.8 at commit `dbb5eeaadffb6a3ba6a60de1290312e5802dba5a`
- License: MIT
- Copyright: 2014-2025 Omar Cornut

CMake fetches Dear ImGui during Windows GUI configuration. Release packages include its license as `licenses/Dear-ImGui-MIT.txt`.

## Roboto

- Source: `assets/fonts/Roboto.ttf`
- Upstream: <https://github.com/googlefonts/roboto-classic>
- License: SIL Open Font License 1.1
- License text: `assets/fonts/OFL.txt`
- Copyright: 2011 The Roboto Project Authors

## MinGW runtime

Windows release packages include `libgcc_s_seh-1.dll`, `libstdc++-6.dll`, and `libwinpthread-1.dll` from the MSYS2 UCRT64 toolchain. GCC runtime terms and the GCC Runtime Library Exception are stored under `licenses/mingw-gcc/` in each package. Winpthreads terms are stored under `licenses/mingw-winpthreads/`.
