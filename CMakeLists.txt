cmake_minimum_required(VERSION 3.16)
project(flight_sw LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra)

# Coding standard S3: compiler warnings as the first enforcement line,
# applied to all flight code (logic library and RTEMS image).
set(HK_STRICT_WARNINGS -Wall -Wextra -Werror -Wconversion -Wshadow -Wpedantic)

add_subdirectory(src/app)
add_subdirectory(src/fdir)

if(RTEMS)
  add_subdirectory(src/app/rtems)
endif()

# Unit tests build and run on the host; they do not cross-compile for RTEMS.
if(NOT CMAKE_CROSSCOMPILING)
  enable_testing()
  add_subdirectory(test)
endif()
