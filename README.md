# purple-signal

A libpurple/Pidgin plugin based on [signal-cli](https://github.com/AsamK/signal-cli) (for signal, formerly textsecure).

*This is a proof-of-concept, NOT meant for actual use.*

Tested on Ubuntu 18.04, wine 4.0.3 and Windows 10.

### Concept

libpurple plug-ins are written in C.  
The signal client library is written in Java.  
This project uses JNI to create a Java VM instance within the C part of the plug-in. An instance of the Jaba Class implementing the signal client is created. Upon receival of a message, a static method is called. This call-back method is a native method implemented in C. It forwards the data to libpurple.

### Status

While this works perfectly fine on Ubuntu, the JVM fails spectacularly on Windows (and in wine). I gave up on this project.

### Known Problems

* On shutdown, the Java VM does not terminate for about two minutes, preventing the plug-in from unloading.
