# Running FFL-Testing
As of 2024-05-31, I added a Makefile that should more or less "just" work on Linux, or Windows if you have MSYS2.
1. Clone the repo, _recursively_.
    ```
    git clone --recursive https://github.com/ariankordi/FFL-Testing
    ```
2. Install requirements.

    This project needs GLEW, GLFW3, OpenGL, and zlib. The commands below will install them.

    * Ubuntu/Debian: `sudo apt install libglew-dev libglfw3-dev zlib1g-dev libgl1-mesa-dev`
    * Fedora/RHEL: `sudo dnf install glew-devel glfw-devel zlib-devel mesa-libGL-devel`
    * Arch/Manjaro: `sudo pacman -S glew glfw zlib` (⚠️ UNTESTED)
    * MSYS2 MINGW64 (Windows): `pacman -S mingw-w64-x86_64-glew mingw-w64-x86_64-glfw mingw-w64-x86_64-zlib`
        - Make sure to also install the basics: `pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make mingw-w64-x86_64-pkg-config`
        - **Use `mingw32-make` instead of `make`**
        - As of 2024-06-02, this doesn't copy DLLs from MinGW64's environment to yours so you can launch the program in the command line but not standalone.

    If you are building with WUT, you need to install `ppc-zlib`.
3. Build using the makefile.

    If you don't already have them, you will also need: git, g++, make, pkg-config.
    ```
    make -j4
    ```
    To build with WUT, use: `make wut -j4`
3. Obtain required files.
    * You'll need to extract the following file from a Wii U:
        - `sys/title/0005001b/10056000/content/FFLResHigh.dat`
    * Place that file in the root of this repo.
        - This file contains models and textures needed to render Miis and this program will not work without it.
4. Run `ffl_testing_2_debug64`, and pray that it works.

### Compiling dependencies GLEW and GLFW3
<details>
<summary>
Both are needed to run any RIO application on PC.

If you can't install them (not on Linux or MSYS2), let's build them.

**(You probably don't need to follow these)**
</summary>

### GLEW:
* `git clone https://github.com/nigels-com/glew && cd glew`
* If you are cross compiling (From Linux to Windows...):
  - Also run `export SYSTEM=linux-mingw64` (OR... `msys-win64`, `mingw-win32`, `darwin-arm64`...)
* `make -j8`
* (sudo) `make install`
### GLFW3:
* `git clone https://github.com/glfw/glfw && cd glfw`
* `cmake -S . -B build`
	- If you are cross compiling, append: `-D CMAKE_TOOLCHAIN_FILE=CMake/x86_64-w64-mingw32.cmake -D CMAKE_INSTALL_PREFIX=/usr/local/x86_64-w64-mingw32/`
* `cmake --build build -j8`
* (sudo) `cmake --install build`
### Now they should be available to pkg-config
Try: `pkg-config --libs zlib glew glfw3`

(Unless it complains about needing `glu`)
#### If you are still reading
NOTE from 2024-06-02: To cross compile this from Linux to Windows, I used the following command:
`
TOOLCHAIN_PREFIX=x86_64-w64-mingw32- make LDFLAGS="-L/dev/shm/glfw/build/src/ -lz -L/dev/shm/glew/lib/ -lglew32 -lglfw3 -lopengl32 -lgdi32 -lws2_32
`

Where I have glew and glfw built at /dev/shm.

While pkg-config worked, letting me need only the TOOLCHAIN_PREFIX set, for whatever reason it wasn't building and threw lots of linking errors saying it couldn't link tons of symbols from glew32 even though it literally finds it and opens the library, so... IDK.
</details>
