#pragma once

#include "../typedjni.hpp"

class PurpleSignal {
    public:
    static TypedJNIEnv *jvm; // static member – only one Java VM over all connections
    std::unique_ptr<TypedJNIObject> instance; // reference to this connection's PurpleSignal (Java) instance.
    std::function<jint(jstring,jstring)> send_message; // reference to this connection's PurpleSignal (Java) instance's sendMessage method.
    PurpleSignal(const std::string & signal_cli_path);
    static void destroy();
};

