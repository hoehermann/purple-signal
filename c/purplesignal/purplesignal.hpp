#pragma once

#include "../submodules/typedjni/typedjni.hpp"

class PurpleSignal {
    private:
    TypedJNIEnv *jvm;
    TypedJNIObject instance; // reference to this connection's PurpleSignal (Java) instance.
    std::function<jint(jstring,jstring)> send_message; // reference to this connection's PurpleSignal (Java) instance's sendMessage method.
    
    public:
    PurpleSignal() = delete;
    PurpleSignal(uintptr_t connection, uintptr_t account, TypedJNIEnv *jvm, const std::string & settings_directory, const std::string & username);
    
    // disconnecting
    int close();
    
    // account management
    void link_account();
    void register_account(bool voice, const std::string & captcha);
    void verify_account(const std::string & code, const std::string & pin);
    
    // messaging
    int send(const char *who, const char *message);
};

