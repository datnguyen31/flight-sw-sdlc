# add_library(fdir
#     fdir.cpp
# )
# target_include_directories(fdir PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(fdir INTERFACE)
target_include_directories(fdir INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
