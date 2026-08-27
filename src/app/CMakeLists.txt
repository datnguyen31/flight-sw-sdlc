# add_library(app
#     app.cpp
# )
# target_include_directories(app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(app INTERFACE)
target_include_directories(app INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
