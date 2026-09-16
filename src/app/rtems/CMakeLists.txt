# Built only when cross-compiling for RTEMS (see the toolchain file under
# cmake/). Produces a smoke-test image proving the toolchain integration;
# it is not the HK-TM application itself.
add_executable(rtems_smoke init.c)
target_link_libraries(rtems_smoke PRIVATE app fdir)
