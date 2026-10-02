# CMake toolchain file: RTEMS 6, BSP arm/beagleboneblack (DD-08), for real
# hardware booted over TFTP. A preset of the generic toolchain-rtems.cmake.
#
# Usage:
#   cmake -S . -B build-rtems \
#         -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-rtems-beagleboneblack.cmake
#   cmake --build build-rtems
set(RTEMS_PREFIX "/opt/rtems6" CACHE PATH "RTEMS kernel/toolchain install root")
set(RTEMS_VERSION "6" CACHE STRING "RTEMS major version (tool prefix)")
set(RTEMS_ARCH "arm" CACHE STRING "RTEMS architecture")
set(RTEMS_BSP "beagleboneblack" CACHE STRING "RTEMS BSP name")

include(${CMAKE_CURRENT_LIST_DIR}/toolchain-rtems.cmake)
