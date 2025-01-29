cmake_minimum_required(VERSION 3.15)
project(ffl_testing LANGUAGES CXX)

# ----------- Options -----------

option(RIO_USE_GLEW "Use GLEW instead of GLAD (only if you have issues with it)" OFF)
option(RIO_GLES "Build to target OpenGL ES 3.0 (Needed for Raspberry Pi, ANGLE, etc.)" OFF)
option(RIO_NO_CLIP_CONTROL "Needed for compatibility with OpenGL versions prior to 4.5, or any GPU from before ~2015. Enable this if you have such a system or see upside down textures. This is enabled when using RIO_GLES." OFF)
option(RIO_USE_OSMESA "Uses OSMesa, which provides off-screen rendering without having to run X11. Enable this if you are using a VPS, but not if your server has a GPU, OSMesa only uses software rendering." OFF)

option(NO_GLTF "Disable glTF export feature" OFF)
option(USE_SYSTEMD_SOCKET "Use systemd socket activation. Don't enable this if you don't know what it is." OFF)

# Sentry integration.
set(USE_SENTRY_DSN "" CACHE STRING "Sentry DSN (leave blank to disable Sentry)")

set(CMAKE_CACHE_ARGS_INCLUDE_HELP TRUE) # Include option descriptions with cmake -LH.

# Target Options ----------------

set(TARGET_NAME "ffl_testing_2") # Set target name.

# Configure the target binary to build outside of the build directory.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../") # Main source directory.

if(MSVC) # MSVC/Visual Studio needs a specific tweak to emit to the source dir
    foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES}) # Iterate through each config type (Debug, Release)
        string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} "${CMAKE_SOURCE_DIR}")
    endforeach()
endif()

# ----------- Compiler Settings -----------

# RIO/FFL need C++17, but MSVC wants it to be C++20.
if(MSVC)
    set(CMAKE_CXX_STANDARD 20)
    # MSVC: warning level 3
    #add_compile_options(/W3)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    # GCC/Clang: -Wall
    #add_compile_options(-Wall -Wno-unused-private-field -Wno-missing-braces)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-unused-private-field -Wno-missing-braces")
endif()

# Debug/Release Compiler Options ----------

# Use debug as the primary build target.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Set what compile options should be included for each build type.

# MSVC needs different compile options.
if(MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "/ZI /Od /DRIO_DEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG /DRIO_RELEASE")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/O2 /ZI /DNDEBUG /DRIO_RELEASE")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "/O1 /DNDEBUG /DRIO_RELEASE")
else()
    # GCC/Clang compile options.
    set(CMAKE_CXX_FLAGS_DEBUG "-g3 -DRIO_DEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -DRIO_RELEASE")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG -DRIO_RELEASE")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG -DRIO_RELEASE")
endif()


# ----------- External Dependencies -----------

# RIO Dependencies ----------------------------
find_package(glfw3 REQUIRED)
if(NOT glfw3_FOUND)
    message(FATAL_ERROR "GLFW3 is required but not found.")
endif()

if(RIO_USE_GLEW) # Use GLEW as an alternative to GLAD.
    find_package(GLEW REQUIRED)
    if(NOT GLEW_FOUND)
        message(FATAL_ERROR "GLEW was enabled but not found. Please install libglew-dev or whatever the equivalent is or simply omit RIO_USE_GLEW.")
    endif()
    # Find OpenGL library as well.
    find_package(OpenGL REQUIRED)
    if(NOT OpenGL_FOUND)
        message(FATAL_ERROR "OpenGL library was not found, and this is needed for GLEW. Please install the development package for your system (e.g., libgl-dev) or simply omit RIO_USE_GLEW.")
    endif()
endif()

# Program Dependencies ------------------------

if(USE_SYSTEMD_SOCKET) # For systemd socket activation.
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SYSTEMD REQUIRED libsystemd)
    if(NOT SYSTEMD_FOUND)
        message(FATAL_ERROR "USE_SYSTEMD_SOCKET was specified, but the systemd library was not found. Please install libsystemd-dev or whatever the equivalent is.")
    endif()
endif()


# Configure Sentry and Git info if USE_SENTRY_DSN isn't blank
if(NOT USE_SENTRY_DSN STREQUAL "")
    # Search for Sentry library.
    find_package(sentry REQUIRED)

    # Fetch Git hash
    execute_process(
        COMMAND git rev-parse HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Define compile-time constants for DSN and release
    set(SENTRY_RELEASE "${CMAKE_PROJECT_NAME}@${GIT_COMMIT_HASH}")
    add_compile_definitions(
        USE_SENTRY_DSN="${USE_SENTRY_DSN}"
        SENTRY_RELEASE="${SENTRY_RELEASE}"
    )
endif()

# ----------- Add Subprojects -----------

# RIO Sources ---------------------------

# Use globbed sources:
file(GLOB_RECURSE RIO_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/rio/src/*.cpp")
add_library(rio STATIC ${RIO_SOURCES}) # Add RIO as a static library.

target_include_directories(rio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/rio/include)

# Link external dependencies:
target_link_libraries(rio PUBLIC glfw)

if(RIO_USE_GLEW) # Link GLEW optionally.
    # GLEW requires linking OpenGL libraries at build time.
    target_link_libraries(rio PUBLIC GLEW OpenGL::GL)
endif()

# FFL Project ----------------------------

# Pass RIO and ninTexUtils paths.
set(FFL_WITH_RIO "${CMAKE_CURRENT_SOURCE_DIR}/rio" CACHE PATH "")
set(FFL_WITH_NINTEXUTILS "${CMAKE_CURRENT_SOURCE_DIR}/ninTexUtils" CACHE PATH "")
# Explicitly choose "for-rio" as the mode for FFL.
set(FFL_MODE "for-rio" CACHE STRING "")

set(FFL_BUILD_SHARED OFF)
set(FFL_BUILD_STATIC ON)  # Build FFL as a static library.
# All features unused by FFL-Testing:
set(FFL_NO_FS ON)         # Do not use FS or database for the moment.
set(FFL_NO_DATABASE_FILE ON) # (This is already enabled by above)
#set(FFL_MLC_PATH .) # Set to current working directory.
set(FFL_NO_DATABASE_RANDOM ON) # Used in master branch.
set(FFL_NO_MIDDLE_DB ON)
set(FFL_NO_DRAW_MASK_ALPHA_VALUES ON)
set(FFL_ENABLE_NEW_MASK_ONLY_FLAG ON)

add_subdirectory(ffl) # Use FFL's CMakeLists.

# ----------- Defines -----------

# Always disable controller support.
target_compile_definitions(rio PRIVATE RIO_NO_CONTROLLERS_WIN)

if(RIO_USE_OSMESA)
    target_compile_definitions(rio PRIVATE RIO_USE_OSMESA)
endif()

# Needs to propagate to FFL:
if(RIO_USE_GLEW)
    target_compile_definitions(rio PRIVATE RIO_USE_GLEW)
    target_compile_definitions(ffl-for-rio PUBLIC RIO_USE_GLEW)
endif()
if(RIO_GLES)
    target_compile_definitions(rio PUBLIC RIO_GLES)
    target_compile_definitions(ffl-for-rio PUBLIC RIO_GLES)
endif()
if(RIO_NO_CLIP_CONTROL)
    target_compile_definitions(rio PUBLIC RIO_NO_CLIP_CONTROL)
    target_compile_definitions(ffl-for-rio PUBLIC RIO_NO_CLIP_CONTROL)
endif()

# ----------- Main Sources -----------

set(SRC_FILES
    src/main.cpp
    src/Model.cpp
    src/RootTask.cpp
    src/DataUtils.cpp
    src/BodyModel.cpp
    src/RenderTexture.cpp
    src/HeadwearModel.cpp

    src/Shader.cpp
    src/ShaderSwitch.cpp
    src/ShaderMiitomo.cpp
)

# Conditionally exclude glTF support.
if(NOT NO_GLTF)
    list(APPEND SRC_FILES src/tinygltf_impl.cpp src/GLTFExportCallback.cpp)
endif()

# ----------- Create Target -----------

# Add program sources to target.
add_executable(${TARGET_NAME} ${SRC_FILES})

target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Link RIO and FFL to the main target.
target_link_libraries(${TARGET_NAME} PRIVATE ffl-for-rio rio)

# Program-specific defines: -----------

if(USE_SYSTEMD_SOCKET) # Listen on systemd socket activation.
    target_compile_definitions(${TARGET_NAME} PRIVATE USE_SYSTEMD_SOCKET)
endif()

if(NO_GLTF) # Disable glTF exporter/exclude tinygltf if it is not needed.
    target_compile_definitions(${TARGET_NAME} PRIVATE NO_GLTF)
endif()

# Link libraries specific to main program.

if(USE_SYSTEMD_SOCKET AND SYSTEMD_FOUND)
    message(STATUS "FFL-Testing: systemd socket support enabled")
    target_link_libraries(${TARGET_NAME} PRIVATE ${SYSTEMD_LIBRARIES})
endif()

if(NOT USE_SENTRY_DSN STREQUAL "" AND sentry_FOUND)
    message(STATUS "FFL-Testing: Sentry enabled with DSN: ${USE_SENTRY_DSN}")
    target_link_libraries(${TARGET_NAME} PRIVATE sentry::sentry)
endif()

if(WIN32) # Explicitly link winsock on Windows.
    target_link_libraries(${TARGET_NAME} PRIVATE wsock32 ws2_32)
endif()

# ----------- Summary -----------
message(STATUS "FFL-Testing: Build type: ${CMAKE_BUILD_TYPE}")
