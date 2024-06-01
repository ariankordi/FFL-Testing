# Running FFL-Testing
As of 2024-05-31, I added a Makefile that should more or less "just" work on Linux.
1. Clone the repo, _recursively_.
    ```
    git clone --recursive https://github.com/ariankordi/FFL-Testing
    ```
2. Have all of these installed on Debian/Ubuntu, and hope that this is the complete list of dependencies.
    ```
    sudo apt install libglew-dev libglfw3-dev zlib1g-dev libgl1-mesa-dev
    ```
    You need GLEW, GLFW3, OpenGL, and zlib. You need to install `ppc-zlib` if you are building with WUT.
3. Build using the makefile.
    ```
    make -j4
    ```
    To build for WUT, use: `make -f Makefile.wut -j4`
3. Obtain required files.
    * You'll need to extract the following folder from a Wii U MLC:
        - `sys/title/0005001b/10056000/content`
    * Copy that folder, and make the folder structure above, then place sys in  the root of the repo.
    * This contains the `FFLResHigh.dat` and `FFLResMiddle.dat` resources that this needs to run.
4. Pray that it works.

I plan to add support to compile with MinGW to target Windows, and/or a Visual Studio project. Soon.
