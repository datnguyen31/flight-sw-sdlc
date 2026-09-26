# CMake toolchain file: cross-compile for RTEMS 6, BSP arm/beagleboneblack.
#
# Usage:
#   cmake -S . -B build-rtems \
#         -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-rtems-beagleboneblack.cmake
#   cmake --build build-rtems
#
# RTEMS_PREFIX points at the RTEMS 6 kernel/toolchain install (the arm-rtems6
# GCC and the BSP's compiled kernel and headers). The BSP's own pkg-config
# file (lib/pkgconfig/arm-rtems6-beagleboneblack.pc) bakes in the path of the
# machine it was built on, not where it is actually installed, so this file
# derives flags directly from RTEMS_PREFIX instead of trusting pkg-config.
set(RTEMS_PREFIX "/opt/rtems6" CACHE PATH "RTEMS 6 kernel/toolchain install root")
set(RTEMS_ARCH "arm")
set(RTEMS_BSP "beagleboneblack")
# BSP's core clock, per the BSP's own pkg-config Cflags/Ldflags (ABI_FLAGS).
set(RTEMS_ABI_FLAGS "-mcpu=cortex-a8")

set(RTEMS_TARGET_TRIPLE "${RTEMS_ARCH}-rtems6")
set(RTEMS_BSP_LIBDIR "${RTEMS_PREFIX}/${RTEMS_TARGET_TRIPLE}/${RTEMS_BSP}/lib")

if(NOT EXISTS "${RTEMS_BSP_LIBDIR}")
  message(FATAL_ERROR
    "RTEMS BSP directory not found: ${RTEMS_BSP_LIBDIR}\n"
    "Check RTEMS_PREFIX (currently ${RTEMS_PREFIX}) and that the "
    "${RTEMS_BSP} BSP is built for ${RTEMS_ARCH}-rtems6.")
endif()

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ${RTEMS_ARCH})

set(CMAKE_C_COMPILER "${RTEMS_PREFIX}/bin/${RTEMS_TARGET_TRIPLE}-gcc")
set(CMAKE_CXX_COMPILER "${RTEMS_PREFIX}/bin/${RTEMS_TARGET_TRIPLE}-g++")
set(CMAKE_ASM_COMPILER "${RTEMS_PREFIX}/bin/${RTEMS_TARGET_TRIPLE}-gcc")

# RTEMS applications have no host OS to link a normal executable against
# until BSP-specific flags (below) are applied; skip CMake's own
# try-compile executable check in favour of a static-library probe.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# -qrtems is RTEMS's own GCC convenience flag: combined with -B<bsp lib>,
# it selects the BSP's start files, specs, and default link script.
set(CMAKE_C_FLAGS_INIT "${RTEMS_ABI_FLAGS} -isystem${RTEMS_BSP_LIBDIR}/include")
set(CMAKE_CXX_FLAGS_INIT "${RTEMS_ABI_FLAGS} -isystem${RTEMS_BSP_LIBDIR}/include")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${RTEMS_ABI_FLAGS} -B${RTEMS_BSP_LIBDIR} -qrtems -Wl,--gc-sections")

set(CMAKE_FIND_ROOT_PATH "${RTEMS_BSP_LIBDIR}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# RTEMS has no dlopen-style shared objects for a target like this one.
set(BUILD_SHARED_LIBS OFF)

set(RTEMS TRUE)
