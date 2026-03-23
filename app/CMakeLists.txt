add_executable(kns_app
    main.cpp
)

target_link_libraries(kns_app
    PRIVATE
    kns_core
)