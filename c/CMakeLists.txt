# based on skype4pidgin/skypeweb/CMakeLists.txt

find_package(JNI REQUIRED)

find_package(PkgConfig REQUIRED)

pkg_check_modules(PURPLE REQUIRED purple)

add_definitions(-Wall)

message(STATUS
"PURPLE_INCLUDE_DIRS: ${PURPLE_INCLUDE_DIRS}"
)
message(STATUS
"PURPLE_LIBRARY_DIRS: ${PURPLE_LIBRARY_DIRS}"
)

# TODO: check for DIRENT_INCLUDE_DIRS on Windows
# obtainable from https://github.com/tronkko/dirent/blob/master/include/dirent.h

include_directories(${PROJECT_NAME}
    ${PURPLE_INCLUDE_DIRS}
    ${JNI_INCLUDE_DIRS}
    ${DIRENT_INCLUDE_DIRS}
    )

set(SRC_LIST
    purple_compat.h
    libsignal.c
    libsignal-jni.h
    libsignal-jni.c
    )

add_library(${PROJECT_NAME} SHARED ${SRC_LIST})
#add_executable(${PROJECT_NAME} ${SRC_LIST})

target_link_directories(${PROJECT_NAME}
    PUBLIC ${PURPLE_LIBRARY_DIRS}
    )
    
get_target_property(_classDir purple_signal CLASSDIR)
add_custom_target(
    embed_purple_signal
    # TODO: find a CMake-ier way of generating this file
    ld -r -b binary -o ${CMAKE_CURRENT_BINARY_DIR}/PurpleSignal.o ${_classDir}/de/hehoe/purple_signal/PurpleSignal.class
)
add_dependencies(${PROJECT_NAME} embed_purple_signal)

target_link_libraries(${PROJECT_NAME}
    ${PURPLE_LIBRARIES}
    ${JNI_LIBRARIES}
    purple_signal_native # this is a target defined by the Java part. needs CMake 3.11
    ${CMAKE_CURRENT_BINARY_DIR}/PurpleSignal.o
    )

exec_program("${PKG_CONFIG_EXECUTABLE} --variable=plugindir purple 2>/dev/null"
    OUTPUT_VARIABLE LIB_INSTALL_DIR
    RETURN_VALUE PURPLE_PLUGINDIR_RET
    )

if (NOT PURPLE_PLUGINDIR_RET EQUAL 0)
    message(WARNING "${PKG_CONFIG_EXECUTABLE} --variable=plugindir purple -- returned a non-null error code")
else()
    install(TARGETS ${PROJECT_NAME} DESTINATION ${LIB_INSTALL_DIR})
endif()

file(STRINGS "../VERSION" SIGNAL_PLUGIN_VERSION)
message(STATUS "SIGNAL_PLUGIN_VERSION ${SIGNAL_PLUGIN_VERSION}")

# remove automatic prefix "lib" from build target name
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES PREFIX "")

target_compile_definitions(${PROJECT_NAME} PUBLIC SIGNAL_PLUGIN_VERSION="${SIGNAL_PLUGIN_VERSION}" OWN_FILE_NAME="${PROJECT_NAME}.dll")

if (MINGW)
    # TODO: find out why I thought this was necessary
    set_target_properties (${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "-Wl,--add-stdcall-alias"
        CLEAN_DIRECT_OUTPUT 1
    )
endif()
