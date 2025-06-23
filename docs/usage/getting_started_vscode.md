# Developing using VSCode
This guide helps to setup the toolchain on Windows, Mac and Linux using vscode.

## Install VS Code
👉 [Download & install VS Code](https://code.visualstudio.com/Download) for Windows, macOS, or Linux.

## Install Docker
👉 [Download & install Docker](https://www.docker.com/get-started/) for Windows, macOS, or Linux.

## Necessary Extensions
In VS Code, open the Extensions panel (`Ctrl+Shift+X`) and install:

-  [**Dev Containers**](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers) — for the Docker-based build environment.
-  [**CMake Tools**](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools) — for configuring and building.
-  [**C/C++ Extension Pack**](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools-extension-pack) — for IntelliSense and debugging.

## Open in Container

* Clone this repository:

  ```bash
  git clone https://github.com/RobotPatient-Research/SensorHub.git
  ```

* Open the project folder in VS Code.
* When prompted, click **“Reopen in Container”**, or run `Dev Containers: Reopen in Container` from the Command Palette (`Ctrl+Shift+P`).

## Configure CMake Tools
![alt text](artifacts/guide_vscode_build_bar.png)
* Make sure you are in the devcontainer environment and click on the **build cogwheel** in the bottom bar.
* The first time you build, CMake Tools will prompt you to select a **kit**:

  * First, click **“Scan for Kits”** to detect available compilers. The menu will disappear...
  * Hit build button again, then, select **`arm-none-eabi-gcc`** from the list.

## Build

* Use the **CMake Tools** status bar at the bottom to:

  * **Configure**
  * **Build**

## How to select the other SensorHub build target?
CMake needs to be instructed to build with the other `board_conf.h` file. By adding this to your `.vscode/settings.json` (if it doesn't exist, create it (don't forget to put snippet in brackets {...})).
```json
    "cmake.configureArgs": [
        "-DBUILD_SENSORHUB_HEAD=ON ",
        "-DBUILD_SENSORHUB_1=OFF"
    ]
```