# SensorHub Firmware

Firmware for **SensorHub** devices used in a manikin system. This software runs on an **STM32F405RG** microcontroller and handles sensor sampling, USB communication, CAN bus integration, data storage, and debugging.

---

## Features

* Supports multiple board configurations:

  * ✅ **SensorHub1** (default)
  * ✅ **SensorHub Head**
* USB CDC (virtual COM port) support
* CAN bus communication using ISO-TP
* Flash memory storage using LittleFS
* Debugging with SEGGER RTT
* Modular hardware abstraction via Manikin Software Libraries

---

## Development Environment

This project uses a **Docker-based development container** (`.devcontainer`) to provide a consistent build environment with the STM32 GCC toolchain and dependencies preconfigured.

### Recommended: Use VS Code Remote Containers

1. Open the project folder in VS Code.
2. If prompted, **"Reopen in Container"** — or use the `Remote-Containers: Reopen in Container` command.
3. The toolchain, CMake, and dependencies will be ready to use inside the container.

---

## Board Configuration

You can switch the build target in vscode by adding this to the `settings.json` file:
```json
    "cmake.configureArgs": [
        "-DBUILD_SENSORHUB_HEAD=ON ",
        "-DBUILD_SENSORHUB_1=OFF"
    ]
```
or when using commandline you can manually pass in the options:

```bash
# Build for SensorHub1 (default)
cmake -DBUILD_SENSORHUB_1=ON  -DBUILD_SENSORHUB_HEAD=OFF -B build

# Or build for SensorHub Head
cmake -DBUILD_SENSORHUB_1=OFF -DBUILD_SENSORHUB_HEAD=ON -B build
```

---

## Flashing

Use your preferred STM32 flashing tool to flash the generated `.elf` file to the microcontroller. There are configurations for Segger Ozone included in this repo, see: `ozone_debug.jdebug`.

---

## License

This project is licensed under **GNU GPL v3**.
See [LICENSE](LICENSE) for details.
