add_library(app STATIC
    src/hk_manager.c
    src/hk_command.c
    src/hk_critical_host.c) # RTEMS impl swapped in for the target build, Session 7
target_include_directories(app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Coding standard S3: compiler warnings as the first enforcement line.
target_compile_options(app PRIVATE
    -Wall -Wextra -Werror -Wconversion -Wshadow -Wpedantic)
