# System test image (Session 9; verification/system-test-spec.md). Built
# only when cross-compiling for RTEMS. It links the same target glue as
# the flight image (hk_service: tasks, startup, console sink, time) and
# replaces only the parameter provider and the ground script with the rig.
# Run it with test/system/run_system_tests.sh, or:
#   cmake --build <dir> --target run_system_tests
add_executable(hk_system_test
    st_main.c
    st_param_provider.c)
set_target_properties(hk_system_test PROPERTIES SUFFIX ".exe")
target_include_directories(hk_system_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(hk_system_test PRIVATE hk_service)
target_compile_options(hk_system_test PRIVATE ${HK_STRICT_WARNINGS})

if(RTEMS_BSP STREQUAL "leon3")
  add_custom_target(run_system_tests
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run_system_tests.sh $<TARGET_FILE:hk_system_test>
    DEPENDS hk_system_test
    USES_TERMINAL
    COMMENT "Running the HK-TM system tests (ST-01..ST-10) on Renode (LEON3)")
endif()
