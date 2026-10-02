# Built only when cross-compiling for RTEMS (cmake/toolchain-rtems.cmake).
# hk_app is the HK-TM service image: the host-tested logic library, the two
# RTEMS task wrappers, and this BSP-level glue (simulated parameter source,
# console telemetry sink, RTEMS time, Init).
add_executable(hk_app
    init.c
    hk_param_source_sim.c
    hk_tm_sink_console.c
    hk_time_rtems.c
    ../tasks/hk_collection_task.c
    ../tasks/hk_command_task.c)
set_target_properties(hk_app PROPERTIES SUFFIX ".exe")
target_include_directories(hk_app PRIVATE ../tasks)
target_link_libraries(hk_app PRIVATE app fdir)
target_compile_options(hk_app PRIVATE ${HK_STRICT_WARNINGS})

if(RTEMS_BSP STREQUAL "leon3")
  # End-to-end run under Renode (DD-09): cmake --build <dir> --target run_renode
  add_custom_target(run_renode
    COMMAND ${PROJECT_SOURCE_DIR}/tools/renode/run_hk_app.sh $<TARGET_FILE:hk_app>
    DEPENDS hk_app
    USES_TERMINAL
    COMMENT "Running hk_app on Renode (LEON3) and decoding its telemetry")
endif()

if(NOT RTEMS_BSP STREQUAL "beagleboneblack")
  return()
endif()

# Package the ELF into a U-Boot loadable image (hk_app.img), the same
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
  set(RTEMS_IMAGE_BIN "${CMAKE_CURRENT_BINARY_DIR}/hk_app.bin")
  set(RTEMS_IMAGE_GZ  "${RTEMS_IMAGE_BIN}.gz")
  set(RTEMS_IMAGE_OUT "${CMAKE_CURRENT_BINARY_DIR}/hk_app.img")

  add_custom_command(
    OUTPUT "${RTEMS_IMAGE_OUT}"
    COMMAND "${RTEMS_OBJCOPY}" -O binary --strip-all
            "$<TARGET_FILE:hk_app>" "${RTEMS_IMAGE_BIN}"
    COMMAND "${GZIP_EXECUTABLE}" -9 -f "${RTEMS_IMAGE_BIN}"
    COMMAND "${MKIMAGE_EXECUTABLE}" -A arm -O linux -T kernel
            -a ${RTEMS_IMAGE_LOAD_ADDR} -e ${RTEMS_IMAGE_LOAD_ADDR}
            -n "HK-TM service" -d "${RTEMS_IMAGE_GZ}" "${RTEMS_IMAGE_OUT}"
    DEPENDS hk_app
    BYPRODUCTS "${RTEMS_IMAGE_BIN}" "${RTEMS_IMAGE_GZ}"
    COMMENT "Packaging hk_app as a U-Boot loadable image (hk_app.img)"
    VERBATIM
  )
  add_custom_target(rtems_image ALL DEPENDS "${RTEMS_IMAGE_OUT}")
else()
  if(NOT MKIMAGE_EXECUTABLE)
    message(STATUS "mkimage not found: skipping hk_app.img packaging "
                    "(install u-boot-tools to enable it)")
  elseif(NOT GZIP_EXECUTABLE)
    message(STATUS "gzip not found: skipping hk_app.img packaging")
  else()
    message(STATUS "objcopy not found at ${RTEMS_OBJCOPY}: skipping "
                    "hk_app.img packaging")
  endif()
endif()
