include(FetchContent)
FetchContent_Declare(unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG        v2.6.0)
FetchContent_MakeAvailable(unity)

add_executable(test_hk_command unit/test_hk_command.c)
target_link_libraries(test_hk_command PRIVATE app unity)
add_test(NAME test_hk_command COMMAND test_hk_command)

add_executable(test_hk_manager unit/test_hk_manager.c)
target_link_libraries(test_hk_manager PRIVATE app unity)
add_test(NAME test_hk_manager COMMAND test_hk_manager)
