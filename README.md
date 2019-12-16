# purple-signal

A libpurple/Pidgin plugin based on [signal-cli](https://github.com/AsamK/signal-cli) (for signal, formerly textsecure).

Tested on Ubuntu 18.04.

*This is a proof-of-concept, NOT meant for actual use.*

### Known Problems

* On shutdown, the Java VM does not terminate for about two minutes, preventing the plug-in from unloading.
