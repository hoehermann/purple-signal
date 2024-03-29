# purple-signal

A libpurple/Pidgin plugin for signal (formerly textsecure). Using [modified parts](https://github.com/hoehermann/signal-cli/) of [signal-cli](https://github.com/AsamK/signal-cli) which in turn is employing [a fork of the official implementation](https://github.com/Turasa/libsignal-service-java).

![Instant Message](/screenshot_win32.png?raw=true "Instant Message on Windows Screenshot")  

### State

**This plug-in is no longer being developed.** The author writes:

> This plug-in essentially was a C/C++ wrapper around the (official) Java implementation libsignal-service-java. However, Signal itself is moving away from their own reference implementation. They now favour libsignal-client, which implements the same functionality in Rust. This plug-in ended up as a C wrapper around a Java compatibility layer around a Rust binary which is hard to build. Everything is moving in different directions and nothing is stable. Even nodejs seems to be involved at some point. 
>
> Moving away from the platform-independent Java implementation, Signal has pulled support win32 so hard [not even third parties can maintain it](https://github.com/signalapp/Signal-Desktop/issues/1636#issuecomment-1051237708). Maybe [purple-signald](https://github.com/hoehermann/libpurple-signald) will be revived so at least Linux users can have Signal in Pidgin again.

### Download

Binaries once were availiable at https://buildbot.hehoe.de/purple-signal/builds/.  

See [INSTALL](INSTALL.md) on how to install this plug-in.  
If you want to build the plug-in yourself see [BUILD](BUILD.md).

### Concept

libpurple plug-ins are written in C.  
The signal client library is written in Java.  
This project uses JNI to create a Java VM instance within the C part of the plug-in. An instance of the Java Class implementing the signal client is created. Upon receival of a message, a static method is called. This call-back method is a native method implemented in C. It forwards the data to libpurple.  
This may be considered dark magic by some.

Please note this is the third purple plugin I have ever written. I still have no idea what I am doing.

### Alternative Backends

These back-ends were considered, but not used:

* [libsignal-service-java](https://github.com/signalapp/Signal-Android/tree/master/libsignal/service) cannot link to existing accounts
* [SignalProtocolKit](https://github.com/signalapp/SignalProtocolKit) deprecated, recommends libsignal-protocol-c
* [libsignal-protocol-c](https://github.com/signalapp/libsignal-protocol-c) development seems to have stalled in 2018
* [textsecure](https://github.com/nanu-c/textsecure) "The API (…) is in flux"
* [libsignal-client](https://github.com/signalapp/libsignal-client) "use outside Signal not yet recommended"

### Features

This plug-in is a proof-of-concept with very little features:

* Register a new account (without captcha only)
* Link to an existing account  
  Use the provided code text with signal-cli  
  or  
  Scan the provided QR code with with the official app
* Simple Messaging
  * one-to-one conversation
  * group conversations (linked only)
  * receiving attachments
* Contact list download

Please note that, as of now, you have to add your contacts manually and they appear "offline" in the buddy list.

For more functionality, check out [purple-signald](https://github.com/hoehermann/libpurple-signald). It offers more features, but it depends on a third-party service and thus is Linux only.

### Missing Features

* Retry failed messages
* Sending attachments
* Proper group chats
* Group chat management (explicit join, leave, administration, …)
* everything else

### Known Problems

* May require new link or registration after a crash. Your number can be banned temporarily.
* If the plug-in crashes while handling a message, the message is lost.
* The message cache is not implemented and I have no idea what that may entrail. Probably the above.
* First message from purple-signal to official signal is not sent properly.
* Avatars are downloaded into working directory.
* It takes two minutes for the internal JVM to shut down. Or it crashes when terminating Pidgin. This issue disappears and reappears erratically.
