cmake_minimum_required(VERSION 3.16)

project(GNS VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Warnings
if (MSVC)
  add_compile_options(/W4 /permissive-)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# lista simples de fontes
set(SRC_FILES
  src/main.cpp
  src/node.cpp
)

add_executable(gns ${SRC_FILES})
target_include_directories(gns PRIVATE ${PROJECT_SOURCE_DIR}/include)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
