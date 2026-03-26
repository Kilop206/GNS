function(copy_topologies target_name)
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/topologies
        $<TARGET_FILE_DIR:${target_name}>/topologies
    )
endfunction()

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

copy_topologies(kns_tests)

catch_discover_tests(kns_tests
    WORKING_DIRECTORY $<TARGET_FILE_DIR:kns_tests>
)