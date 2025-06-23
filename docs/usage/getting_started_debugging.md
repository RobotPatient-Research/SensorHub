# Debugging with SEGGER Ozone

This guide explains how to debug your firmware using **SEGGER Ozone** with the provided `.jdebug` configurations for different workflows.

---

## 📁 Provided `.jdebug` files

| File                    | Intended for                                               | Notes                                                                                                                      |
| ----------------------- | ---------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------- |
| **`ozone_debug_vscode.jdebug`**     | Use with **VS Code** builds (any build target)             | Works with the build output from VS Code tasks                                                                             |
| **`ozone_debug_clion_sensorhub_head.jdebug`**      | Use with **CLion**, **build target: `sensorhub-head`**     | Make sure your **CMake profile** is named `Debug-SensorHub-Head` — the build folder must match the `.jdebug` configuration |
| **`ozone_debug_clion.jdebug`** | Use with **CLion**, **build target: default Docker build** | Works with the CMake profile named `Debug-Docker`                                                                          |

---

## How to use

1️⃣ **Install SEGGER Ozone**

* Download and install from [SEGGER’s official website](https://www.segger.com/downloads/jlink/#Ozone).

2️⃣ **Build your firmware**

* Use **VS Code** or **CLion** to build your project with the correct target/profile.

  * **VS Code:** Any build target
  * **CLion:** Ensure the **CMake profile name** matches the `.jdebug` expectation exactly.

3️⃣ **Open the `.jdebug` file in Ozone**

* Start Ozone.
* Open the appropriate `.jdebug` via **File → Open Project**.
* Verify that Ozone loads the correct binary and target.

4️⃣ **Start your debug session**

* Click **Start Debug Session** (`F5`).
* Set breakpoints, step through code, and inspect variables as needed.

---

## ⚠️ FAQ & Common Pitfalls

### CLion: Why does my debug session fail?

When using CLion, two things must match your `.jdebug` file:

**1 CMake profile name must match exactly**

* For `ozone_debug_clion.jdebug`:

  * **Profile name:** `Debug-SensorHub-Head`
  * **Build target:** `sensorhub-head`
* For `ozone_debug_clion_sensorhub_head.jdebug`:

  * **Profile name:** `Debug-Docker`

If your CMake profile name differs, CLion will put build output in a different folder — breaking the path in `.jdebug`.

**2 Binary path must match**

Check that the `File.Open ("$(ProjectDir)/cmake-build-debug-docker/SensorHub_FW.elf");` line path in the `.jdebug` points to the actual location of your compiled `.elf` file in the build folder.

If you rename the profile or build folder, update the `.jdebug` accordingly.

---

## Good to know

* **VS Code users:** The `ozone_debug_vscode.jdebug` works with any build target. Just build in VS Code and run the file in Ozone (as long as the build folder is in firmware/ folder and called build (default config)).
* **CLion users:** Be consistent with your CMake profiles and targets to avoid path mismatches.
* Keep your `.jdebug` files version-controlled for easy sharing across your team.
* **Notice:** Both CLion and Vscode handle the mount between docker and your workspace differently. Hence why it is needed to create different `.jdebug` files. The absolute mount path for vscode is hardcoded to: `/workspaces/SensorHub/firmware` and in CLion it is the default path at: `/tmp/firmware`. These paths were set manually by using notepad++ or vscode and opening the .jdebug files as plain text and adding this line to the OnProjectLoad section: `Project.AddPathSubstitute ("/tmp/firmware", "$(ProjectDir)");`. If you ever encounter any issues were Ozone would complain it can't find the source-files. Look at this line inside the script :).
