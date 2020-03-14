find_package(Java 1.8.0 EXACT REQUIRED)
find_package(Java COMPONENTS Development)
#find_package(JNI REQUIRED)
set(JAVA_HOME /var/www/www-buildbot/pidgin-devel/jdk8u232-b09)
set(JAVA_AWT_LIBRARY ${JAVA_HOME}/jre/bin/jawt.dll)
set(JAVA_JVM_LIBRARY ${JAVA_HOME}/jre/bin/client/jvm.dll)
set(JAVA_INCLUDE_PATH ${JAVA_HOME}/include)
set(JAVA_INCLUDE_PATH2 ${JAVA_HOME}/include/win32)
set(JAVA_AWT_INCLUDE_PATH ${JAVA_INCLUDE_PATH})
set(JNI_LIBRARIES ${JAVA_AWT_LIBRARY} ${JAVA_JVM_LIBRARY})
set(JNI_INCLUDE_DIRS ${JAVA_INCLUDE_PATH} ${JAVA_INCLUDE_PATH2})
set(JNI_FOUND TRUE)

set(SIGNAL_CLI_LIB_DIR "/opt/signal-cli/lib" CACHE STRING "Path to signal-cli's jar files.")
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
#message(STATUS
#JNI_INCLUDE_DIRS ${JNI_INCLUDE_DIRS} "\n"
#JNI_LIBRARIES ${JNI_LIBRARIES} "\n"
#JNI_FOUND ${JNI_FOUND} "\n"
#JAVA_AWT_LIBRARY ${JAVA_AWT_LIBRARY} "\n"
#JAVA_JVM_LIBRARY ${JAVA_JVM_LIBRARY} "\n"
#JAVA_INCLUDE_PATH ${JAVA_INCLUDE_PATH} "\n"
#JAVA_INCLUDE_PATH2 ${JAVA_INCLUDE_PATH2} "\n"
#JAVA_AWT_INCLUDE_PATH ${JAVA_AWT_INCLUDE_PATH} "\n"
#)
