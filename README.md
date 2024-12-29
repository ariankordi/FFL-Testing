**Check the renderer-server-prototype branch if you want to run a service that renders Miis.**

# FFL-Testing
Sample using [the FFL decomp](https://github.com/aboood40091/ffl) and [RIO framework](https://github.com/aboood40091/rio), all originally written by [AboodXD](https://github.com/aboood40091), to render Miis on PC and Wii U via WUT.

https://github.com/user-attachments/assets/cdc358e4-0b1f-4b93-b7fd-f61fa4d215e1

* Note that it's a known issue for Mii textures to look corrupted on Wii U. This does not happen with the original decompilation, only the RIO branch.

## Running
1. Clone the repo, _recursively_.
    ```
    git clone --recursive https://github.com/ariankordi/FFL-Testing
    ```
If you forgot or want to update, then after `git pull`, do this: `git submodule update --init`

2. Install requirements.

    This project needs GLFW3 and zlib. The commands below will install them.

    * Ubuntu/Debian: `sudo apt install libglfw3-dev zlib1g-dev libgl1-mesa-dev`
    * Fedora/RHEL: `sudo dnf install glfw-devel zlib-devel mesa-libGL-devel`
    * Arch/Manjaro: `sudo pacman -S glew glfw zlib` (⚠️ UNTESTED)
    * Windows MSYS2 MINGW64: `pacman -S mingw-w64-x86_64-glfw mingw-w64-x86_64-zlib`
        - Make sure to also install the basics: `pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-pkg-config` (untested)
    * Windows MSVC/Visual Studio: Use vcpkg. [Follow Microsoft's tutorial](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started?pivots=shell-powershell)...
        - ... and then: `vcpkg install glfw3 zlib`
        - Don't forget to pass `CMAKE_TOOLCHAIN_FILE` when you use vcpkg with CMake. [Read here for more info: https://learn.microsoft.com/en-us/vcpkg/users/buildsystems/cmake-integration](https://learn.microsoft.com/en-us/vcpkg/users/buildsystems/cmake-integration)

3. Build using the CMake.

    If you don't already have them, you will also need: git, g++, cmake, pkg-config.
    ```
    cmake -S . -B build  # Configure project.
    cmake --build build  # Compile to "build".
    ```

    **If you are building for Wii U, instead, do:** **`make -f Makefile.wut`**

    Here are some additional things to know.
    * To list options, use `cmake -LH`.
        - To apply an option: `cmake -S . -B build` **`-DRIO_GLES=1`**, etc.
    * Settings:
        - Build type: `cmake -S . -B build` **`-DCMAKE_BUILD_TYPE=Release`**
        - CXXFLAGS/Optimizations: `cmake -S . -B build` **`-DCMAKE_CXX_FLAGS="-O3 -march=native"`**
        - Jobs: `cmake --build build` **`-j4`** (4 = num of jobs/cores)

    * Clean:
        - Either delete build (be careful!), or
        - `cmake --build build --target clean`
    * (On Visual Studio: As long as you have vcpkg configured and everything installed, it should? just work by loading the folder)

3. Obtain the resource file, FFLResHigh.dat.
    * This contains the meshes and textures needed to render Mii characters and is absolutely required to get anywhere with this.
    * You can get it from many sources:
        - It can be extracted from a Wii U using an FTP program:
            - `sys/title/0005001b/10056000/content/FFLResHigh.dat`
        - You can extract it from a Miitomo install (_it's not in the APK/IPA_):
            - In cache (`Library/Application Support/cache` on iOS), here: `res/asset/model/character/mii/AFLResHigh_2_3.dat`
        - It can also be downloaded from archive.org:
            * https://web.archive.org/web/20180502054513/http://download-cdn.miitomo.com/native/20180125111639/android/v2/asset_model_character_mii_AFLResHigh_2_3_dat.zip
            * Extract the above and rename `AFLResHigh_2_3.dat` to `FFLResHigh.dat`.
        - (As well as `AFLResHigh_2_3.dat`, `AFLResHigh.dat` will work too. All AFL resources have to be renamed to `FFLResHigh.dat`.)
        - In the Kadokawa breach, there is a Wii U tool called FFLUtility, located here: `dwango/projects/マルチデバイス/品証/WiiU/Tool/FFL/downloadimage/FFLUtilityJP_p01` - if you decrypt it with dev keys, then in `fs/content/nonproduct/miicapture/resource/`, you will find `FFLResPoster.dat`. This is a copy of FFLResHigh.dat with extremely high quality (512px) textures. If you successfully find this, share the love and enjoy.
    * Place that file in the root of this repo.
        - This file contains models and textures needed to render Miis and this program will not work without it.
4. Run `ffl_testing_2`, and pray that it works.
    * If it crashes with a segfault or shows you a blank screen, make sure you have `FFLResHigh.dat` and it can open it.

### Showing your own Miis
As of 2024-06-14, I added a change that would let you change which Miis this program renders, by reading files in the `place_ffsd_files_here` folder.

As the name implies, you should be able to take any Mii in a 96 byte FFSD/FFLStoreData file, and place it in that folder.

* Only the length of the file is checked, not the extension or name. It needs to be 96 bytes.

* Then, it will just read each file sequentially, looping through all of them.
* You can also just put one file there and that one Mii will spin continuously.

#### If you don't have any FFSD files...
* You can search for any *.ffsd or *.cfsd (CFLStoreData = FFLStoreData) file in this repo:
    - https://github.com/HEYimHeroic/MiiDataFiles

* Grab the Mii from your Nintendo Network ID using the mii-unsecure.ariankordi.net API, if it is still up by the time you are reading this.
    - `curl --verbose --header "Accept: application/octet-stream" https://mii-unsecure.ariankordi.net/mii_data/JasmineChlora --output JasmineChlora.ffsd`
        * Replace `JasmineChlora` with your own NNID, of course.
* I believe this script decrypts Mii QR Code data directly to FFSD/CFSD format: https://gist.github.com/jaames/96ce8daa11b61b758b6b0227b55f9f78
    - I use it with ZBar like so: `zbarimg --quiet --raw --oneshot -Sbinary image.jpg | python3 mii-qr.py /dev/stdin ./mii.ffsd`
        * Replace: `image.jpg`, `mii.ffsd`
    - I also made a scanner in JS, though it gives you Base64 that you have to decode: https://jsfiddle.net/arian_/h31tdg7r/1/

### Compiling GLFW3
<details>
<summary>
GLFW3 is needed to run any RIO application on PC, which this is.

If you can't install it (not on Linux or MSYS2), or need the latest version, let's build it.

**(You probably don't need to follow these)**
</summary>

### GLFW3:
* `git clone https://github.com/glfw/glfw && cd glfw`
* `cmake -S . -B build`
	- If you are cross compiling, append: `-D CMAKE_TOOLCHAIN_FILE=CMake/x86_64-w64-mingw32.cmake -D CMAKE_INSTALL_PREFIX=/usr/local/x86_64-w64-mingw32/`
* `cmake --build build -j8`
* (sudo) `cmake --install build`
### Now it should be available to pkg-config
Try: `pkg-config --libs zlib glfw3`

(Unless it complains about needing `glu`)
#### If you are still reading
NOTE from 2024-06-02: To cross compile this from Linux to Windows, I used the following command:
`
TOOLCHAIN_PREFIX=x86_64-w64-mingw32- make LDFLAGS="-L/dev/shm/glfw/build/src/ -lz -L/dev/shm/glew/lib/ -lglew32 -lglfw3 -lopengl32 -lgdi32 -lws2_32
`

Where I have glew and glfw built at /dev/shm.

While pkg-config worked, letting me need only the TOOLCHAIN_PREFIX set, for whatever reason it wasn't building and threw lots of linking errors saying it couldn't link tons of symbols from glew32 even though it literally finds it and opens the library, so... IDK.
</details>
