cmake_minimum_required(VERSION 3.16)
project(flight_sw LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra)

add_subdirectory(src/app)
add_subdirectory(src/fdir)

enable_testing()
add_subdirectory(test)
