add_executable(kns_tests
    test_topology.cpp
    test_event_queue.cpp
    test_simulation_clock.cpp
)

target_link_libraries(kns_tests
    PRIVATE
    kns_core
    Catch2::Catch2WithMain
)

catch_discover_tests(kns_tests)