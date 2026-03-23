add_library(kns_core
    src/engine/Event.cpp
    src/engine/EventQueue.cpp
    src/engine/PrintEvent.cpp
    src/engine/SimulationClock.cpp
    src/engine/SimulationEngine.cpp
    src/network/topology.cpp
    src/network/topology_loader.cpp
)

target_include_directories(kns_core PUBLIC
    include
)