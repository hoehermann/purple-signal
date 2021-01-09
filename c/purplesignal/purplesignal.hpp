#pragma once

#include "../submodules/typedjni/typedjni.hpp"

class PurpleSignal {
    private:
    TypedJNIEnv *jvm;
    TypedJNIObject instance; // reference to this connection's PurpleSignal (Java) instance.
    std::function<jint(jstring,jstring)> send_message; // reference to this connection's PurpleSignal (Java) instance's sendMessage method.
    
    public:
    PurpleSignal() = delete;
    PurpleSignal(TypedJNIEnv *jvm, uintptr_t account, const std::string & username);
    
    // disconnecting
    int close();
    
    // account management
    void link_account();
    void register_account(bool voice, const std::string & captcha);
    void verify_account(const std::string & code, const std::string & pin);
    
    // messaging
    int send_im(const std::string& who, const std::string& message);
};
