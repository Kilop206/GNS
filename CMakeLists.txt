cmake_minimum_required(VERSION 3.20)
project(KNS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
include(CTest)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.5.2
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(Catch2)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)

set(GLFW_DIR ${CMAKE_SOURCE_DIR}/external/glfw/include/glfw-3.4.bin.WIN64)

add_library(glfw STATIC IMPORTED)
set_target_properties(glfw PROPERTIES
    IMPORTED_LOCATION ${GLFW_DIR}/lib-vc2022/glfw3.lib
    INTERFACE_INCLUDE_DIRECTORIES ${GLFW_DIR}/include
)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui)

add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC 
    ${IMGUI_DIR}
    ${GLFW_DIR}/include
)
target_link_libraries(imgui PUBLIC glfw)

add_subdirectory(core)
add_subdirectory(app)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

configure_file(
    ${CMAKE_SOURCE_DIR}/topologies/mesh5.json
    ${CMAKE_BINARY_DIR}/tests/mesh5.json
    COPYONLY
)

add_subdirectory(tests)
