# Installation

### Windows and Linux

By default, purple-signal uses the data directory provided by signal-cli (`~/.local/share/signal-cli` on Linux). This means you can switch back and forth between signal-cli and Pidgin with purple-signal, but do *not* use both at the same time.

### Windows

1. [a x86 (32-bit) Java](https://adoptopenjdk.net/releases.html), e.g. [11.0.9](https://github.com/AdoptOpenJDK/openjdk11-binaries/releases/download/jdk-11.0.9%2B11.1/OpenJDK11U-jdk_x86-32_windows_hotspot_11.0.9_11.zip)  
  Unpack it to a directory*. I assume `c:\opt\openjdk-11`.
  Modify your user's PATH environment variable to include `c:\opt\openjdk-11\jre\bin\client;`.
1. [signal-cli](https://github.com/AsamK/signal-cli/releases/tag/v0.6.11)  
  Unpack it to a directory*. I assume `c:\opt\signal-cli`.  
1. [Pidgin](https://sourceforge.net/projects/pidgin/files/Pidgin/2.13.0/pidgin-2.13.0-offline.exe/download)  
  Install it to a directory*. I assume `c:\opt\pidgin`.  
  Modify your user's PATH environment variable to include `c:\opt\pidgin;c:\opt\pidgin\Gtk\bin;`
1. [purple-signal](https://buildbot.hehoe.de/purple-signal/builds/)  
  Both purple-signal.dll and purple_signal.jar are needed.  
  Put them into `c:\opt\pidgin\plugins`.  
  Launch Pidgin, create a new account. Enter your phone number (internationalized format +491234567890) for a username. Set the signal-cli path to `c:\opt\signal-cli\lib`.

*) It may or may not be necessary for that directory *not* to have spaces in the entire path. The default `C:\Program Files (x86)` may *not* work. I did not check thoroughly.

### Linux

1. Java  
  Have a Java JRE installed (tested with OpenJDK 11). Architecture should match your system (must match Pidgin's machine type).
1. [signal-cli](https://github.com/AsamK/signal-cli/releases/tag/v0.6.11)  
  Unpack it to a directory* spaces. I assume `/opt/signal-cli`.
1. Pidgin  
  Installed via your distribution's package manager.
1. purple-signal  
  Build with `cmake` and `make`. `javac`, `gcc` and `libpurple-dev` dependencies such as `libglib-dev` are required.  
  Copy or link libpurple-signal.so and purple_signal.jar into `~/.purple/plugins`.  
  Launch Pidgin, create a new account. Enter your phone number (internationalized format +491234567890) for a username. Set the signal-cli path to `/opt/signal-cli`.
