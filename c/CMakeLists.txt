# based on skype4pidgin/skypeweb/CMakeLists.txt

find_package(JNI REQUIRED)
find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
	pkg_check_modules(PURPLE REQUIRED purple)
endif()

message(STATUS
"PURPLE_INCLUDE_DIRS: ${PURPLE_INCLUDE_DIRS}"
)
message(STATUS
"PURPLE_LIBRARY_DIRS: ${PURPLE_LIBRARY_DIRS}"
)

# TODO: check for DIRENT_INCLUDE_DIRS on Windows
# obtainable from https://github.com/tronkko/dirent/blob/master/include/dirent.h
#include(CheckIncludeFileCXX)
#set(CMAKE_REQUIRED_INCLUDES ...)
#CHECK_INCLUDE_FILE_CXX(dirent.h DIRENT_HEADER_FOUND) ...

set(SRC_LIST
    purple_compat.h
    libsignal.hpp
    libsignal.cpp
    environment.hpp
    environment.cpp
    connection.hpp
    connection.cpp
    buddies.hpp
    buddies.cpp
    natives.cpp
    handler/account.cpp
    handler/account.hpp
    handler/async.cpp
    handler/async.hpp
    handler/message.cpp
    handler/message.hpp
    handler/attachment.cpp
    handler/attachment.hpp
    purplesignal/purplesignal.hpp
    purplesignal/account.cpp
    purplesignal/construction.cpp
    purplesignal/message.cpp
    purplesignal/attachment.cpp
    purplesignal/utils.cpp
    purplesignal/utils.hpp
    purplesignal/error.cpp
    purplesignal/error.hpp
    submodules/typedjni/typedjni.hpp
    submodules/typedjni/typedjni.cpp
    submodules/qrcode/cpp/QrCode.cpp
    submodules/qrcode/cpp/QrCode.hpp
    )

#set(CMAKE_CXX_STANDARD 17)

set(TARGET_NAME ${PROJECT_NAME})

# target name is not "signal" as there could be collisions with other (system-supplied) libraries called "signal"
add_library(${TARGET_NAME} SHARED ${SRC_LIST})

if (${MINGW_CROSSCOMPILING})
target_compile_definitions(${TARGET_NAME} PRIVATE MINGW_CROSSCOMPILING)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(${TARGET_NAME} PRIVATE 
		-Werror 
		-Wall 
		#-Wextra 
		#-Wconversion 
		#-Wsign-conversion 
		-Wsign-compare
		# TODO: push these when including purple.h only
		-DGLIB_DISABLE_DEPRECATION_WARNINGS
		-Wno-write-strings
		-Wno-parentheses
	)
endif()

target_include_directories(${TARGET_NAME} PRIVATE
    ${PURPLE_INCLUDE_DIRS}
    ${JNI_INCLUDE_DIRS}
    ${DIRENT_INCLUDE_DIRS}
    )

target_link_directories(${TARGET_NAME}
    PUBLIC ${PURPLE_LIBRARY_DIRS}
    )

target_link_libraries(${TARGET_NAME}
    ${PURPLE_LIBRARIES}
    ${JNI_LIBRARIES}
    purple_signal_native # this is a target defined by the Java part. needs CMake 3.11
    )

option(EMBED_JAVA_BYTECODE "Embed PurpleSignal.class into shared library." OFF)
if (EMBED_JAVA_BYTECODE)
    get_target_property(_classDir purple_signal CLASSDIR)
    add_custom_target(
        embed_purple_signal
        # TODO: find a CMake-ier way of generating this file
        ld -r -b binary -o ${CMAKE_CURRENT_BINARY_DIR}/PurpleSignal.o ${_classDir}/de/hehoe/purple_signal/PurpleSignal.class
    )
    add_dependencies(purple-signal embed_purple_signal)
    target_link_libraries(${TARGET_NAME}
        ${CMAKE_CURRENT_BINARY_DIR}/PurpleSignal.o
        )
endif()

if (PKG_CONFIG_FOUND)
	exec_program("${PKG_CONFIG_EXECUTABLE} --variable=plugindir purple 2>/dev/null"
		OUTPUT_VARIABLE LIB_INSTALL_DIR
		RETURN_VALUE PURPLE_PLUGINDIR_RET
		)
	if (NOT PURPLE_PLUGINDIR_RET EQUAL 0)
		message(WARNING "${PKG_CONFIG_EXECUTABLE} --variable=plugindir purple -- returned a non-null error code")
	else()
		install(TARGETS ${TARGET_NAME} DESTINATION ${LIB_INSTALL_DIR})
	endif()
endif()

file(STRINGS "../VERSION" SIGNAL_PLUGIN_VERSION)
message(STATUS "SIGNAL_PLUGIN_VERSION ${SIGNAL_PLUGIN_VERSION}")

target_compile_definitions(${TARGET_NAME} PUBLIC SIGNAL_PLUGIN_VERSION="${SIGNAL_PLUGIN_VERSION}" OWN_FILE_NAME="${TARGET_NAME}.dll" SIGNAL_CLI_JAR="${SIGNAL_CLI_JAR}")

if (MINGW)
    # remove automatic prefix "lib" from build target name
    SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES PREFIX "")

    # on win32, function names are decorated with @*-suffixes.
    # aliases are needed for the JVM to find the native functions
    #TODO: move this to target_link_libraries below
    set_target_properties (${TARGET_NAME} PROPERTIES
        LINK_FLAGS "-Wl,--add-stdcall-alias"
        CLEAN_DIRECT_OUTPUT 1 #TODO: is this necessary?
    )

    # see https://github.com/hoehermann/purple-signal/issues/1#issuecomment-745527414
    # and https://stackoverflow.com/questions/38694058/cmake-linking-statically-against-libgcc-and-libstdc-into-a-shared-library
    # but also see https://github.com/kcat/alure/commit/e9f8f60
    # and https://stackoverflow.com/questions/13768515/how-to-do-static-linking-of-libwinpthread-1-dll-in-mingw
    target_link_libraries(${TARGET_NAME} -static-libgcc -static-libstdc++)
endif()
