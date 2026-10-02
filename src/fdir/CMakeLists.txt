# FDIR increment (Session 8): limit monitoring, PUS Service 5 events, and
# the safe-mode controller, kept apart from the baseline application in
# src/app. fdir and app depend on each other: the monitor reads app's
# report type and emits through app's telemetry sink, and app's collection
# cycle and command handler call into fdir at one point each
# (design/fdir-design.md). CMake links mutually dependent static libraries
# by repeating them on the link line.
add_library(fdir STATIC
    src/hk_limits.c
    src/hk_monitor.c
    src/hk_event.c
    src/hk_safemode.c)
target_include_directories(fdir PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(fdir PUBLIC app)

target_compile_options(fdir PRIVATE ${HK_STRICT_WARNINGS})
