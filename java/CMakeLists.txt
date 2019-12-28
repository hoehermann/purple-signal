find_package(Java 8 REQUIRED) # 9 and 11 were tested. compiles with 8.
find_package(Java COMPONENTS Development)
find_package(JNI REQUIRED)

set(SIGNAL_CLI_LIB_DIR /opt/signal-cli/lib)
file(GLOB SIGNAL_CLI_JARS ${SIGNAL_CLI_LIB_DIR}/*.jar)
if (NOT SIGNAL_CLI_JARS)
    message(FATAL_ERROR "signal-cli not found. Please set SIGNAL_CLI_LIB_DIR to the directory with signal-cli's jar files.")
endif()
set(CMAKE_JAVA_INCLUDE_PATH ${SIGNAL_CLI_JARS})
#message(STATUS "CMAKE_JAVA_INCLUDE_PATH ${CMAKE_JAVA_INCLUDE_PATH}")
include(UseJava)
add_jar(purple_signal
    src/de/hehoe/purple_signal/PurpleSignal.java
    ENTRY_POINT de/hehoe/purple_signal/PurpleSignal
    GENERATE_NATIVE_HEADERS purple_signal_native # this needs at least CMake 3.11
)
