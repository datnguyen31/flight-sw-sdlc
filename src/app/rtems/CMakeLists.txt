# Built only when cross-compiling for RTEMS (see the toolchain file under
# cmake/). Produces a smoke-test image proving the toolchain integration;
# it is not the HK-TM application itself.
add_executable(rtems_smoke init.c)
target_link_libraries(rtems_smoke PRIVATE app fdir)

# Package the ELF into a U-Boot loadable image (rtems_smoke.img), the same
# objcopy -> gzip -> mkimage sequence used for beagleboneblack elsewhere in
# this toolchain family (see script_build.sh's rtems packaging step in
# /home/datng31/Apps/EO/fsw). This is what TFTP-boots on real hardware
# (DD-08); it has no effect on the ELF used for local inspection/analysis.
find_program(MKIMAGE_EXECUTABLE mkimage)
find_program(GZIP_EXECUTABLE gzip)

get_filename_component(_rtems_bindir "${CMAKE_C_COMPILER}" DIRECTORY)
get_filename_component(_rtems_cc_name "${CMAKE_C_COMPILER}" NAME)
string(REPLACE "gcc" "objcopy" _rtems_objcopy_name "${_rtems_cc_name}")
set(RTEMS_OBJCOPY "${_rtems_bindir}/${_rtems_objcopy_name}")

if(MKIMAGE_EXECUTABLE AND GZIP_EXECUTABLE AND EXISTS "${RTEMS_OBJCOPY}")
  # BeagleBone Black DRAM base; matches this BSP's link address (confirmed
  # against the linked ELF's entry point, ~0x80000040).
  set(RTEMS_IMAGE_LOAD_ADDR 0x80000000)
  set(RTEMS_IMAGE_BIN "${CMAKE_CURRENT_BINARY_DIR}/rtems_smoke.bin")
  set(RTEMS_IMAGE_GZ  "${RTEMS_IMAGE_BIN}.gz")
  set(RTEMS_IMAGE_OUT "${CMAKE_CURRENT_BINARY_DIR}/rtems_smoke.img")

  add_custom_command(
    OUTPUT "${RTEMS_IMAGE_OUT}"
    COMMAND "${RTEMS_OBJCOPY}" -O binary --strip-all
            "$<TARGET_FILE:rtems_smoke>" "${RTEMS_IMAGE_BIN}"
    COMMAND "${GZIP_EXECUTABLE}" -9 -f "${RTEMS_IMAGE_BIN}"
    COMMAND "${MKIMAGE_EXECUTABLE}" -A arm -O linux -T kernel
            -a ${RTEMS_IMAGE_LOAD_ADDR} -e ${RTEMS_IMAGE_LOAD_ADDR}
            -n "HK-TM RTEMS smoke" -d "${RTEMS_IMAGE_GZ}" "${RTEMS_IMAGE_OUT}"
    DEPENDS rtems_smoke
    BYPRODUCTS "${RTEMS_IMAGE_BIN}" "${RTEMS_IMAGE_GZ}"
    COMMENT "Packaging rtems_smoke as a U-Boot loadable image (rtems_smoke.img)"
    VERBATIM
  )
  add_custom_target(rtems_image ALL DEPENDS "${RTEMS_IMAGE_OUT}")
else()
  if(NOT MKIMAGE_EXECUTABLE)
    message(STATUS "mkimage not found: skipping rtems_smoke.img packaging "
                    "(install u-boot-tools to enable it)")
  elseif(NOT GZIP_EXECUTABLE)
    message(STATUS "gzip not found: skipping rtems_smoke.img packaging")
  else()
    message(STATUS "objcopy not found at ${RTEMS_OBJCOPY}: skipping "
                    "rtems_smoke.img packaging")
  endif()
endif()
