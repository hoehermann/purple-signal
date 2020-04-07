#/bin/sh
export PKG_CONFIG_LIBDIR=$(realpath ../../gtk+/lib/pkgconfig)
export PKG_CONFIG_PATH=$(realpath ../../pidgin-2.13.0/libpurple/lib/pkgconfig)

cmake \
-DCMAKE_TOOLCHAIN_FILE=../cross-compile/win32_on_ubuntu18/toolchain.cmake \
-DCMAKE_MODULE_PATH=../cross-compile/win32_on_ubuntu18 \
-DPKG_CONFIG_EXECUTABLE="/usr/bin/pkg-config\;--define-prefix" \
-DJAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64 \
-DJAVA_HOME_WIN32=../../jdk8u232-b09 \
-DSIGNAL_CLI_LIB_DIR=../../signal-cli-0.6.5/lib \
-DDIRENT_INCLUDE_DIRS=../../dirent \
-DCMAKE_BUILD_TYPE=Debug \
..
