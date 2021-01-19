#pragma once

#include "../submodules/typedjni/typedjni.hpp"

class PurpleSignal {
    public:
    TypedJNIEnv *jvm;
    private:
    TypedJNIObject instance; // reference to this connection's PurpleSignal (Java) instance.
    std::function<jint(jstring,jstring)> send_message; // reference to this connection's PurpleSignal (Java) instance's sendMessage method.
    std::function<jobject(jobject,jstring)> accept_attachment; // reference to this connection's PurpleSignal (Java) instance's acceptAttachment method.
    
    public:
    PurpleSignal() = delete;
    PurpleSignal(TypedJNIEnv *jvm, uintptr_t account, const std::string & username);
    
    // disconnecting
    int close();
    
    // account management
    void link_account();
    void register_account(bool voice, const std::string & captcha);
    void verify_account(const std::string & code, const std::string & captcha);
    
    // messaging
    int send_im(const std::string& who, const std::string& message);
    
    // attachment handling
    TypedJNIObject acceptAttachment(jobject attachmentPtr, const std::string & local_file_name);
};
