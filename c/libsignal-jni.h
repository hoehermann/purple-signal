#include "typedjni.hpp"
#include "libsignal.h"
#include <stdint.h>
#include <memory>

typedef struct {
    std::unique_ptr<TypedJNIObject> instance; // reference to this connection's PurpleSignal (Java) instance.
    std::function<jint(jstring,jstring)> send_message; // reference to this connection's PurpleSignal (Java) instance's sendMessage method.
} PurpleSignal;

char *purplesignal_init(const char *signal_cli_path, TypedJNIEnv * & sjvm);
void purplesignal_destroy(TypedJNIEnv * & ps);

void purplesignal_login(TypedJNIEnv *signaljvm, PurpleSignal *ps, uintptr_t connection, const char *username, const char *settings_directory);
int purplesignal_close(const PurpleSignal & ps);
int purplesignal_send(TypedJNIEnv *signaljvm, PurpleSignal & ps, const char *who, const char *message);
void purplesignal_link(TypedJNIEnv *signaljvm, PurpleSignal & ps);

// defined by libsignal.cpp
void signal_handle_message_async(PurpleSignalMessage *psm);
void signal_debug_async(PurpleDebugLevel level, const char *message);
void signal_debug(PurpleDebugLevel level, const std::string & message);
