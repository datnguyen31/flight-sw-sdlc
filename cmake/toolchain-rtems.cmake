# CMake toolchain file: cross-compile for an RTEMS BSP (DD-09).
#
# Default target: RTEMS 7, BSP sparc/leon3, installed at /opt/rtems7, run
# under Renode (tools/renode/). Override the cache variables to select
# another installed BSP, e.g. sparc/erc32 or RTEMS 6 arm/beagleboneblack:
#
#   cmake -S . -B build-target -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-rtems.cmake
#   cmake -S . -B build-erc32  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-rtems.cmake \
#         -DRTEMS_BSP=erc32
#
# The BSP's pkg-config file (lib/pkgconfig/<arch>-rtems<ver>-<bsp>.pc) bakes
# in the path of the machine it was built on, not where it is installed, so
# flags are derived from RTEMS_PREFIX here instead of trusting pkg-config.
set(RTEMS_PREFIX "/opt/rtems7" CACHE PATH "RTEMS kernel/toolchain install root")
set(RTEMS_VERSION "7" CACHE STRING "RTEMS major version (tool prefix)")
set(RTEMS_ARCH "sparc" CACHE STRING "RTEMS architecture")
set(RTEMS_BSP "leon3" CACHE STRING "RTEMS BSP name")

# Per-BSP CPU flags, as in the BSP's own pkg-config ABI_FLAGS.
if(RTEMS_BSP STREQUAL "leon3")
  set(_rtems_abi_default "-mcpu=leon3")
elseif(RTEMS_BSP STREQUAL "erc32")
  set(_rtems_abi_default "-mcpu=cypress")
elseif(RTEMS_BSP STREQUAL "beagleboneblack")
  set(_rtems_abi_default "-mcpu=cortex-a8")
else()
  set(_rtems_abi_default "")
endif()
set(RTEMS_ABI_FLAGS "${_rtems_abi_default}" CACHE STRING "BSP CPU flags")

set(RTEMS_TARGET_TRIPLE "${RTEMS_ARCH}-rtems${RTEMS_VERSION}")
set(RTEMS_BSP_LIBDIR "${RTEMS_PREFIX}/${RTEMS_TARGET_TRIPLE}/${RTEMS_BSP}/lib")

if(NOT EXISTS "${RTEMS_BSP_LIBDIR}")
  message(FATAL_ERROR
    "RTEMS BSP directory not found: ${RTEMS_BSP_LIBDIR}\n"
    "Check RTEMS_PREFIX (currently ${RTEMS_PREFIX}) and that the "
    "${RTEMS_ARCH}/${RTEMS_BSP} BSP is installed for ${RTEMS_TARGET_TRIPLE}.")
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
