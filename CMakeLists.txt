cmake_minimum_required(VERSION 3.15)
project(ffl_testing LANGUAGES CXX)

# ----------- Options -----------

option(RIO_USE_GLEW "Use GLEW instead of GLAD (only if you have issues with it)" OFF)
option(RIO_GLES "Build to target OpenGL ES 3.0 (Needed for Raspberry Pi, ANGLE, etc.)" OFF)
option(RIO_NO_CLIP_CONTROL "Needed for compatibility with OpenGL versions prior to 4.5, or any GPU from before ~2015. Enable this if you have such a system or see upside down textures. This is enabled when using RIO_GLES." OFF)

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
endif()

# ----------- Add Subprojects -----------

# RIO Sources ---------------------------

# Use globbed sources:
file(GLOB_RECURSE RIO_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/rio/src/*.cpp")
add_library(rio STATIC ${RIO_SOURCES}) # Add RIO as a static library.

target_include_directories(rio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/rio/include)

# Link external dependencies:
target_link_libraries(rio PUBLIC glfw) # glfw

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

if(RIO_USE_GLEW)
    target_compile_definitions(rio PRIVATE RIO_USE_GLEW)
endif()

if(RIO_GLES)
    target_compile_definitions(rio PUBLIC RIO_GLES)
endif()

if(RIO_NO_CLIP_CONTROL)
    target_compile_definitions(rio PUBLIC RIO_NO_CLIP_CONTROL)
endif()

# ----------- Main Sources -----------

set(SRC_FILES
    src/main.cpp
    src/Model.cpp
    src/RootTask.cpp

    src/Shader.cpp
)

# ----------- Create Target -----------

# Add program sources to target.
add_executable(${TARGET_NAME} ${SRC_FILES})

target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Link RIO and FFL to the main target.
target_link_libraries(${TARGET_NAME} PRIVATE ffl-for-rio rio)

# ----------- Summary -----------
message(STATUS "FFL-Testing: Build type: ${CMAKE_BUILD_TYPE}")
