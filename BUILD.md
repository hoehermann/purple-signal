# Building

### Dependencies:

* GCC (tested with 9.4.0)
* Java JDK (tested with OpenJDK 17)
* signal-cli (cmake will download an appropriate version of signal-cli automatically)

### Linux:

    git clone https://github.com/hoehermann/purple-signal
    purple-signal
    git submodule update --init
    mkdir build
    cd build
    cmake ..
    cmake --build .

### Windows:

*Due to changes in the signal implementation, building on win32 is no longer possible.*
