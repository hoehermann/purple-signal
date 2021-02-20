find_package(Java 1.8.0 REQUIRED)
find_package(Java COMPONENTS Development)
find_package(JNI REQUIRED)

set(SIGNAL_CLI_LIB_DIR "/opt/signal-cli/lib" CACHE PATH "Path to signal-cli's jar files.")
if (NOT EXISTS ${SIGNAL_CLI_LIB_DIR}/${SIGNAL_CLI_JAR})
    set(SIGNAL_CLI_TGZ "${CMAKE_BINARY_DIR}/signal-cli-${SIGNAL_CLI_VERSION}.tar.gz")
    if(NOT EXISTS ${SIGNAL_CLI_TGZ})
        file(DOWNLOAD "https://github.com/AsamK/signal-cli/releases/download/v${SIGNAL_CLI_VERSION}/signal-cli-${SIGNAL_CLI_VERSION}.tar.gz" ${SIGNAL_CLI_TGZ} SHOW_PROGRESS) # TODO: check status
    endif()
    set(SIGNAL_CLI_LIB_DIR "${CMAKE_BINARY_DIR}/signal-cli-${SIGNAL_CLI_VERSION}/lib")
    if(NOT EXISTS ${SIGNAL_CLI_LIB_DIR}/${SIGNAL_CLI_JAR})
        execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzvf ${SIGNAL_CLI_TGZ} RESULT_VARIABLE tar_result) # TODO: find out how this performs on Windows
        message(STATUS "tar_result is ${tar_result}")
        #file(ARCHIVE_EXTRACT INPUT ${SIGNAL_CLI_TGZ}) # this needs at least CMake 3.18
    endif()
endif()
if (NOT EXISTS ${SIGNAL_CLI_LIB_DIR}/${SIGNAL_CLI_JAR})
    message(FATAL_ERROR "${SIGNAL_CLI_JAR} not found. Please set SIGNAL_CLI_LIB_DIR to the directory with signal-cli's jar files.")
endif()
message(STATUS "SIGNAL_CLI_LIB_DIR: ${SIGNAL_CLI_LIB_DIR}")
file(GLOB SIGNAL_CLI_JARS "${SIGNAL_CLI_LIB_DIR}/*.jar")
set(CMAKE_JAVA_INCLUDE_PATH ${SIGNAL_CLI_JARS})
#message(STATUS "CMAKE_JAVA_INCLUDE_PATH ${CMAKE_JAVA_INCLUDE_PATH}")

include(UseJava)

set(CMAKE_JAVA_COMPILE_FLAGS -encoding utf-8)

# https://stackoverflow.com/questions/3179216/what-is-the-convention-for-word-separator-in-java-package-names
add_jar(purple_signal 
    src/de/hehoe/purple_signal/PurpleSignal.java
    src/de/hehoe/purple_signal/PurpleSignalGroupUtils.java
    src/de/hehoe/purple_signal/SignalMessagePrinter.java # debug tool
    submodules/signal-cli/lib/src/main/java/org/asamk/signal/manager/Manager.java
    submodules/signal-cli/lib/src/main/java/org/asamk/signal/manager/storage/SignalAccount.java
    submodules/signal-cli/lib/src/main/java/org/asamk/signal/manager/storage/groups/JsonGroupStore.java
    ENTRY_POINT de/hehoe/purple_signal/PurpleSignal
    GENERATE_NATIVE_HEADERS purple_signal_native # this needs at least CMake 3.11
)
