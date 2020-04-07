# stolen from skype4pidgin/skypeweb/CMakeLists.txt

find_package(JNI REQUIRED)

find_package(PkgConfig REQUIRED)

#set(_PKG_CONFIG_EXECUTABLE ${PKG_CONFIG_EXECUTABLE})
#pkg_check_modules(GLIB2 REQUIRED  glib-2.0)
pkg_check_modules(PURPLE REQUIRED purple)
#set(PKG_CONFIG_EXECUTABLE ${_PKG_CONFIG_EXECUTABLE})

add_definitions(-Wall)

#message(STATUS
#"GLIB2_INCLUDE_DIRS: ${GLIB2_INCLUDE_DIRS}"
#)
message(STATUS
"PURPLE_INCLUDE_DIRS: ${PURPLE_INCLUDE_DIRS}"
)

include_directories(
    /var/www/www-buildbot/pidgin-devel/dirent
    #/var/www/www-buildbot/pidgin-devel/pidgin-2.13.0/libpurple
    ${PURPLE_INCLUDE_DIRS}
    #${GLIB2_INCLUDE_DIRS}
    #/var/www/www-buildbot/pidgin-devel/gtk+/lib/glib-2.0/include/
    #/var/www/www-buildbot/pidgin-devel/gtk+/include/glib-2.0/
    ${JNI_INCLUDE_DIRS}
    )

set(SRC_LIST
    purple_compat.h
    libsignal.c
    libsignal-jni.h
    libsignal-jni.c
    )

add_library(${PROJECT_NAME} SHARED ${SRC_LIST})
#add_executable(${PROJECT_NAME} ${SRC_LIST})

target_link_libraries(${PROJECT_NAME}
    #${PURPLE_LIBRARIES}
    /var/www/www-buildbot/pidgin-devel/pidgin-2.13.0-offline/liburple.dll
    #${GLIB2_LIBRARIES}
    /var/www/www-buildbot/pidgin-devel/pidgin-2.13.0-offline/libglib-2.0-0.dll
    ${JNI_LIBRARIES}
    purple_signal_native # this is a target defined by the Java part. needs CMake 3.11
    )

link_directories(
    ${PURPLE_LIBRARY_DIRS}
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
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES PREFIX "")
target_compile_definitions(${PROJECT_NAME} PUBLIC SIGNAL_PLUGIN_VERSION="${SIGNAL_PLUGIN_VERSION}" OWN_FILE_NAME="${PROJECT_NAME}.dll")
if (MINGW)
    set_target_properties (${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "-Wl,--add-stdcall-alias"
        CLEAN_DIRECT_OUTPUT 1
    )
endif()
