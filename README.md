# purple-signal

A libpurple/Pidgin plugin based on [signal-cli](https://github.com/AsamK/signal-cli) (for signal, formerly textsecure).

Developed on Ubuntu 18.04. Tested wine 5.0 and Windows 10.

![Instant Message](/screenshot_win32.png?raw=true "Instant Message on Windows Screenshot")  

### Download

Binaries are availiable at https://buildbot.hehoe.de/purple-signal/builds/.

### Concept

libpurple plug-ins are written in C.  
The signal client library is written in Java.  
This project uses JNI to create a Java VM instance within the C part of the plug-in. An instance of the Java Class implementing the signal client is created. Upon receival of a message, a static method is called. This call-back method is a native method implemented in C. It forwards the data to libpurple.  
This may be considered dark magic by some.

### Features

* Link to an existing account  
  * Use the provided code with signal-cli
  * Generate a QR code with `qrencode` or any [online service](https://www.the-qrcode-generator.com/) to scan with the official app running on a smartphone
* Simple Messaging
  * one-to-one conversation
  * Group conversation

### Missing Features

* Register a new account
* Attachments
* Proper group chats
* Group chat management (explicit join, leave, administration, …)
