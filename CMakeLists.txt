cmake_minimum_required(VERSION 3.10)
project(shooter)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

include(${CMAKE_SOURCE_DIR}/cmake/CPM.cmake)

set(ZLIB_BUILD_TESTING OFF)
set(ENET_TEST OFF)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(BUILD_EXAMPLES OFF)

CPMAddPackage("gh:raysan5/raylib#6.0")
CPMAddPackage("gh:zpl-c/enet#v2.6.5")
CPMAddPackage("gh:madler/zlib#v1.3.2")
CPMAddPackage("gh:raysan5/raygui#master")

file(GLOB Client
    ${CMAKE_SOURCE_DIR}/src/client/*.cpp
)

file(GLOB Server
    ${CMAKE_SOURCE_DIR}/src/server/*.cpp
)

file(GLOB Shared
    ${CMAKE_SOURCE_DIR}/src/shared/*.cpp
)

add_executable(${PROJECT_NAME} ${Client} ${Shared})
add_executable(${PROJECT_NAME}-server ${Server} ${Shared})

target_link_libraries(${PROJECT_NAME} PUBLIC raylib enet_static zlibstatic)
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR}/src/client
    ${CMAKE_SOURCE_DIR}/src/shared
    ${raygui_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME}-server PUBLIC enet_static zlibstatic)
target_include_directories(${PROJECT_NAME}-server PUBLIC
    ${CMAKE_SOURCE_DIR}/src/server
    ${CMAKE_SOURCE_DIR}/src/shared
)

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
)