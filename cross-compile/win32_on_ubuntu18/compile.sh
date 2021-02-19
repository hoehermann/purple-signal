#/bin/sh

cp -n ${PIDGIN_DEV_PATH}/pidgin-2.13.0-offline/libpurple.dll ${PIDGIN_DEV_PATH}/pidgin-2.13.0/libpurple

cmake \
-DCMAKE_TOOLCHAIN_FILE=../cross-compile/win32_on_ubuntu18/toolchain.cmake \
-DCMAKE_MODULE_PATH=../cross-compile/win32_on_ubuntu18 \
-DJAVA_HOME=/usr/lib/jvm/java-11-openjdk-amd64 \
-DJAVA_HOME_WIN32=${PIDGIN_DEV_PATH}/jdk-11 \
-DPURPLE_INCLUDE_DIRS="${PIDGIN_DEV_PATH}/pidgin-2.13.0/libpurple;${PIDGIN_DEV_PATH}/gtk+/include/glib-2.0;${PIDGIN_DEV_PATH}/gtk+/lib/glib-2.0/include" \
-DPURPLE_LIBRARIES="${PIDGIN_DEV_PATH}/pidgin-2.13.0-offline/libpurple.dll;${PIDGIN_DEV_PATH}/gtk+/bin/libglib-2.0-0.dll" \
-DDIRENT_INCLUDE_DIRS=${PIDGIN_DEV_PATH}/dirent \
-DCMAKE_BUILD_TYPE=Debug \
-DMINGW_CROSSCOMPILING=yes \
..

make
