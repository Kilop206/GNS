add_executable(kns_app main.cpp
    gui/src/Window.cpp
    gui/src/MetricsPannel.cpp
)

target_link_libraries(kns_app 
    kns_core 
    imgui
)

if(WIN32)
    target_link_libraries(kns_app opengl32 gdi32 user32 shell32)
elseif(UNIX)
    target_link_libraries(kns_app GL dl)
endif()

add_custom_command(TARGET kns_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/topologies
    $<TARGET_FILE_DIR:kns_app>/topologies
)
