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

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui)
set(IMGUI_FILE_DIALOG ${CMAKE_SOURCE_DIR}/external/imguifiledialog)

add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/imgui_impl_opengl3.cpp
    ${IMGUI_FILE_DIALOG}/ImGuiFileDialog.cpp
)

target_include_directories(imgui PUBLIC 
    ${IMGUI_DIR}
    ${IMGUI_FILE_DIALOG}
)

target_link_libraries(imgui PUBLIC glfw)

if(NOT WIN32)
    target_link_libraries(imgui PUBLIC ${CMAKE_DL_LIBS})
endif()

add_subdirectory(core)
add_subdirectory(app)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

configure_file(
    ${CMAKE_SOURCE_DIR}/topologies/mesh5.json
    ${CMAKE_BINARY_DIR}/tests/mesh5.json
    COPYONLY
)

add_subdirectory(tests)