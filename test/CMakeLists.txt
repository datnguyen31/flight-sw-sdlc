include(FetchContent)
FetchContent_Declare(unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG        v2.6.0)
FetchContent_MakeAvailable(unity)

# Stub parameter source and capture telemetry sink, linked only into tests.
add_library(hk_test_doubles STATIC support/hk_test_doubles.c)
target_include_directories(hk_test_doubles PUBLIC support)
target_link_libraries(hk_test_doubles PUBLIC app)

foreach(name
    test_hk_command
    test_hk_manager
    test_hk_sampler
    test_hk_report_builder
    test_hk_pus_encoder
    test_hk_collection)
  add_executable(${name} unit/${name}.c)
  target_link_libraries(${name} PRIVATE app hk_test_doubles unity)
  add_test(NAME ${name} COMMAND ${name})
endforeach()
