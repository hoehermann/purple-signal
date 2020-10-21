#pragma once

#include <memory>
#include <purple.h>
#include <stdint.h>
#include "typedjni.hpp"

typedef std::function<void(PurpleConnection *pc)> PurpleSignalConnectionFunction;

class PurpleSignalMessage {
    public:
    const uintptr_t pc;
    const std::unique_ptr<PurpleSignalConnectionFunction> function;
    PurpleSignalMessage(const PurpleSignalMessage&) = delete;
    PurpleSignalMessage& operator=(const PurpleSignalMessage&) = delete;
    PurpleSignalMessage(uintptr_t pc, std::unique_ptr<PurpleSignalConnectionFunction> & function);
};

typedef struct {
    std::unique_ptr<TypedJNIObject> instance; // reference to this connection's PurpleSignal (Java) instance.
    std::function<jint(jstring,jstring)> send_message; // reference to this connection's PurpleSignal (Java) instance's sendMessage method.
} PurpleSignal;

char *purplesignal_init(const char *signal_cli_path, TypedJNIEnv * & sjvm);
void purplesignal_destroy(TypedJNIEnv * & ps);

void purplesignal_login(TypedJNIEnv *signaljvm, PurpleSignal *ps, uintptr_t connection, const char *username, const std::string & settings_directory);
int purplesignal_close(const PurpleSignal & ps);
int purplesignal_send(TypedJNIEnv *signaljvm, PurpleSignal & ps, const char *who, const char *message);
void purplesignal_link(TypedJNIEnv *signaljvm, PurpleSignal & ps);
void purplesignal_register(TypedJNIEnv *signaljvm, PurpleSignal & ps, bool voice);

// defined by libsignal.cpp
void signal_handle_message_async(PurpleSignalMessage *psm);
void signal_debug(PurpleDebugLevel level, const std::string & message);
