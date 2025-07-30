好的，这是您提供的文本的 Markdown 格式版本。

# ZWO\_SkyXPlugins

## ZWO device plugin suite for The SkyX astronomy software, supporting Windows, Linux and macOS systems

-----

### Quick Start (End Users)

Use precompiled versions without compiling\!

1.  Go to the **[Release Download Page](https://www.google.com/search?q=https://www.zwoastro.com/software-drivers)**
2.  Select the installer for your system:
      * **Windows**: `***setup.exe` installer
      * **Linux/macOS**: `install.bin`
3.  Run the installer to complete the setup

-----

### Build from Source (Developers)

  * **All Platforms**: Git (`git --version`)
  * **Windows**: Visual Studio 2008 (with C++ tools)
  * **Linux/macOS**: GCC/G++ (`g++ --version`)

-----

### Build Steps

1.  **Get Source Code**

    ```bash
    git clone https://github.com/ZWOAstro/ZWO_SkyXPlugins.git
    cd ZWO_SkyXPlugins
    ```

2.  **Linux/macOS Build**

      * **Camera Plugin**
        ```bash
        cd x2cameraplugins/x2cameraplugins
        ./buildall.sh   # Select OS -> 1 (Build) -> y (Confirm)
        ```
      * **Focuser Plugin**
        ```bash
        cd ../../x2focuserplugins/x2focuserplugins
        ./buildall.sh
        ```
      * **Filter Wheel Plugin**
        ```bash
        cd ../../x2filterwheelplugins/x2filterwheelplugins
        ./buildall.sh
        ```

3.  **Windows Build**

      * Run Visual Studio 2008 as **Administrator**
      * Open solution files:
          * Camera: `x2cameraplugins/x2cameraplugins.sln`
          * Focuser: `x2focuserplugins/x2focuserplugins.sln`
          * Filter Wheel: `x2filterwheelplugins/x2filterwheelplugins.sln`
      * Select target platform (x86 or x64)
      * Build → Build Solution

-----

### Frequently Asked Questions

**Q: Permission denied when building on macOS/Linux**

```bash
chmod +x buildall.sh  # Add execute permission
```
