set(SERIAL_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src/serialisation")

file(GLOB_RECURSE SERIAL_HEADERS CONFIGURE_DEPENDS
        "${SERIAL_SOURCE_DIR}/*.hpp"
        "${SERIAL_SOURCE_DIR}/*.h"
)

file(GLOB_RECURSE SERIAL_SOURCES CONFIGURE_DEPENDS
        "${SERIAL_SOURCE_DIR}/*.cpp"
)

add_library(serialisation STATIC ${SERIAL_SOURCES} ${SERIAL_HEADERS})
target_include_directories(serialisation PUBLIC "${SERIAL_SOURCE_DIR}")
target_compile_features(serialisation PUBLIC cxx_std_20)