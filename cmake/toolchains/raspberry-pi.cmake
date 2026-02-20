# CMake toolchain file for cross-compiling to Raspberry Pi (64-bit ARM)
#
# Usage:
#   cmake -B build-pi \
#       -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/raspberry-pi.cmake \
#       -DCMAKE_BUILD_TYPE=Release \
#       -DBUILD_TESTS=OFF
#   cmake --build build-pi -j$(nproc)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Cross-compiler (install via: apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu)
set(CMAKE_C_COMPILER   aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

# Sysroot — uncomment and set if using a full Pi rootfs for headers/libs
# set(CMAKE_SYSROOT /path/to/pi-rootfs)

# Search for programs on the host, libraries/headers on the target
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
