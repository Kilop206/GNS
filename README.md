# GNS – Graph Network Simulator

GNS is a C++ discrete-event network simulator built from scratch
for learning systems programming and network modeling.

This project is currently in early development (MVP stage).

---

## Features (current stage)

- Basic project structure with CMake
- Node class implementation
- Executable builds successfully

---

## Requirements

- CMake 3.16+
- C++17 compatible compiler
    - MSVC (Visual Studio 2022)
    - GCC
    - Clang

---

## How to Build

```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
