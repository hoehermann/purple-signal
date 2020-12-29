# purple-signal

A libpurple/Pidgin plugin for signal (formerly textsecure). Using [modified parts](https://github.com/hoehermann/signal-cli/) of [signal-cli](https://github.com/AsamK/signal-cli) which in turn is employing [a fork of the official implementation](https://github.com/Turasa/libsignal-service-java).

Developed on Ubuntu 18.04. Tested wine 5.0 and Windows 10.

![Instant Message](/screenshot_win32.png?raw=true "Instant Message on Windows Screenshot")  

### Download

Binaries are availiable at https://buildbot.hehoe.de/purple-signal/builds/. See [INSTALL](INSTALL.md) on how to install this plug-in.

### Concept

libpurple plug-ins are written in C.  
The signal client library is written in Java.  
This project uses JNI to create a Java VM instance within the C part of the plug-in. An instance of the Java Class implementing the signal client is created. Upon receival of a message, a static method is called. This call-back method is a native method implemented in C. It forwards the data to libpurple.  
This may be considered dark magic by some.

Please note this is the third purple plugin I have ever written. I still have no idea what I am doing.

### Features

This plug-in is a proof-of-concept with very little features:

* Register a new account
* Link to an existing account  
  Use the provided code text with signal-cli  
  or  
  Scan the provided QR code with with the official app
* Simple Messaging
  * one-to-one conversation
  * Group conversation

Please note that, as of now, you have to add your contacts manually and they appear "offline" in the buddy list.

For more functionality, check out [purple-signald](https://github.com/hoehermann/libpurple-signald). It offers more features, but it depends on a third-party service and thus is Linux only.

### Missing Features

* Retry failed messages
* Attachments
* Proper group chats
* Contact list download
* Group chat management (explicit join, leave, administration, …)
* everything else

### Known Problems

* First message from purple-signal to official signal is not sent properly.
* Windows binary is almost unusable.

#### Previously Known Problems

* It takes two minutes for the internal JVM to shut down. Or it crashes when terminating Pidgin. This issue miraculously went away.
