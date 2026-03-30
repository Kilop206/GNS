add_library(kns_core
    src/engine/events/Event.cpp
    src/engine/core/EventQueue.cpp
    src/engine/events/PrintEvent.cpp
    src/engine/time/SimulationClock.cpp
    src/engine/core/SimulationEngine.cpp
    src/network/Topology.cpp
    src/network/TopologyLoader.cpp
    src/network/Routing.cpp
)

target_include_directories(kns_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)