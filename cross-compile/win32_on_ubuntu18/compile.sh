#/bin/sh

export PKG_CONFIG_LIBDIR="$(realpath ${PIDGIN_DEV_PATH}/gtk+/lib/pkgconfig)"
mkdir -p ${PIDGIN_DEV_PATH}/pidgin-2.13.0/libpurple/lib/pkgconfig
export PKG_CONFIG_PATH="$(realpath ${PIDGIN_DEV_PATH}/pidgin-2.13.0/libpurple/lib/pkgconfig)"
cp -n "$(dirname "$0")/purple.pc" "$PKG_CONFIG_PATH/"

cp -n ${PIDGIN_DEV_PATH}/pidgin-2.13.0-offline/libpurple.dll ${PIDGIN_DEV_PATH}/pidgin-2.13.0/libpurple

cmake \
-DCMAKE_TOOLCHAIN_FILE=../cross-compile/win32_on_ubuntu18/toolchain.cmake \
-DCMAKE_MODULE_PATH=../cross-compile/win32_on_ubuntu18 \
-DPKG_CONFIG_EXECUTABLE="/usr/bin/pkg-config\;--define-prefix" \
-DJAVA_HOME=/usr/lib/jvm/java-11-openjdk-amd64 \
-DJAVA_HOME_WIN32=${PIDGIN_DEV_PATH}/jdk-11 \
-DSIGNAL_CLI_LIB_DIR=${PIDGIN_DEV_PATH}/signal-cli/0.6.12/lib \
-DDIRENT_INCLUDE_DIRS=${PIDGIN_DEV_PATH}/dirent \
-DCMAKE_BUILD_TYPE=Debug \
-DMINGW_CROSSCOMPILING=yes \
..

make
