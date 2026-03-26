add_executable(kns_app
    main.cpp
)

target_link_libraries(kns_app
    PRIVATE
    kns_core
)

add_custom_command(TARGET kns_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/topologies
    $<TARGET_FILE_DIR:kns_app>/topologies
)