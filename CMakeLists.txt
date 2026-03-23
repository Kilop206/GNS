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

add_subdirectory(core)
add_subdirectory(app)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

configure_file(
    ${CMAKE_SOURCE_DIR}/topologies/mesh5.json
    ${CMAKE_BINARY_DIR}/tests/mesh5.json
    COPYONLY
)

add_subdirectory(tests)
