cmake_minimum_required(VERSION 3.11)
set(PROJECT_NAME "purple-signal")
project(${PROJECT_NAME})
set(SIGNAL_CLI_JAR "signal-cli-0.10.3.jar")

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
    pkg_check_modules(PURPLE REQUIRED purple)
    pkg_get_variable(PURPLE_PLUGIN_DIR purple plugindir)
    pkg_get_variable(PURPLE_DATA_DIR purple datarootdir)
endif()

add_subdirectory(java)
add_subdirectory(c)
