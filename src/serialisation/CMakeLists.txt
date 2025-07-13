set(SERIAL_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src/serialisation")

file(GLOB_RECURSE SERIAL_SOURCES CONFIGURE_DEPENDS
    "${SERIAL_SOURCE_DIR}/*.cpp"
    "${SERIAL_SOURCE_DIR}/*.hpp"
    "${SERIAL_SOURCE_DIR}/*.h"
)

add_library(serialisation STATIC ${SERIAL_SOURCES})
target_include_directories(serialisation PUBLIC "${SERIAL_SOURCE_DIR}")

set_property(TARGET serialisation PROPERTY CXX_STANDARD 20)