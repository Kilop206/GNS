add_executable(tests
    test_event_queue.cpp
)

target_link_libraries(tests
    PRIVATE
    core
    Catch2::Catch2WithMain
)

include(CTest)
include(Catch)

catch_discover_tests(tests)