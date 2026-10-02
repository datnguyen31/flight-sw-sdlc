# The critical section is the one RTOS-dependent piece of the logic library:
# a no-op on the host, a priority-inheritance mutex on RTEMS (hk_critical.h).
if(RTEMS)
  set(HK_CRITICAL_SRC src/hk_critical_rtems.c)
else()
  set(HK_CRITICAL_SRC src/hk_critical_host.c)
endif()

add_library(app STATIC
    src/hk_manager.c
    src/hk_command.c
    src/hk_param_table.c
    src/hk_sampler.c
    src/hk_report_builder.c
    src/hk_pus_encoder.c
    src/hk_collection.c
    ${HK_CRITICAL_SRC})
target_include_directories(app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
# The FDIR attach points: hk_collection.c (monitor) and hk_command.c
# (CLEAR_SAFE_MODE). src/fdir/CMakeLists.txt links back to app.
target_link_libraries(app PUBLIC fdir)

target_compile_options(app PRIVATE ${HK_STRICT_WARNINGS})
