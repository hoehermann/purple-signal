cmake_minimum_required (VERSION 2.8)

# stolen from skype4pidgin/skypeweb/CMakeLists.txt

set(PROJECT_NAME "signal" C)
set(VERSION_MAJOR "0")
set(VERSION_MINOR "0")
set(VERSION_PATCH "0")

project(${PROJECT_NAME})

find_package(PkgConfig REQUIRED)

pkg_check_modules(GLIB REQUIRED  glib-2.0)
pkg_check_modules(PURPLE REQUIRED purple)

add_definitions(-Wall)

include_directories(
    ${PURPLE_INCLUDE_DIRS}
    ${GLIB2_INCLUDE_DIRS}
    )

set(SRC_LIST
    purple_compat.h
    libsignal.c
    )

add_library(${PROJECT_NAME} SHARED ${SRC_LIST})

target_link_libraries(${PROJECT_NAME}
    ${PURPLE_LIBRARIES}
    ${GLIB2_LIBRARIES}
    )

link_directories(
    ${PURPLE_LIBRARY_DIRS}
    )

exec_program("${PKG_CONFIG_EXECUTABLE} --variable=plugindir purple 2>/dev/null"
    OUTPUT_VARIABLE LIB_INSTALL_DIR
    RETURN_VALUE PURPLE_PLUGINDIR_RET
    )

if (NOT PURPLE_PLUGINDIR_RET EQUAL 0)
    message(FATAL_ERROR "${PKG_CONFIG_EXECUTABLE} --variable=plugindir purple -- returned a non-null error code")
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION ${LIB_INSTALL_DIR})

exec_program("${PKG_CONFIG_EXECUTABLE} --variable=datadir purple 2>/dev/null"
    OUTPUT_VARIABLE PURPLE_DATADIR
    RETURN_VALUE PURPLE_DATADIR_RET
    )

if (NOT PURPLE_DATADIR_RET EQUAL 0)
    message(FATAL_ERROR "${PKG_CONFIG_EXECUTABLE} --variable=datadir purple -- returned a non-null error code")
endif()

file(STRINGS "../VERSION" SIGNAL_PLUGIN_VERSION)
message(STATUS "SIGNAL_PLUGIN_VERSION ${SIGNAL_PLUGIN_VERSION}")
target_compile_definitions(${PROJECT_NAME} PUBLIC SIGNAL_PLUGIN_VERSION="${SIGNAL_PLUGIN_VERSION}")
