file(GLOB_RECURSE KNS_SOURCES CONFIGURE_DEPENDS
    src/*.cpp
)

add_library(kns_core ${KNS_SOURCES})

target_include_directories(kns_core PUBLIC include)