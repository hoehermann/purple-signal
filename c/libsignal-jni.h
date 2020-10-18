#include "typedjni.hpp"
#include <stdint.h>
#include <purple.h>
#include <memory>

typedef struct {
    // TODO: use unique_ptr?
    std::shared_ptr<TypedJNIClass> psclass; // a reference to the PurpleSignal (Java) class.
    std::shared_ptr<TypedJNIObject> instance; // reference to this connection's PurpleSignal (Java) instance.
} PurpleSignal;

typedef struct {
    uintptr_t pc;
    const char *chat;
    const char *sender;
    const char *message;
    /*signed*/ long timestamp;
    PurpleMessageFlags flags;
    PurpleDebugLevel error;
} PurpleSignalMessage;

void signal_handle_message_async(PurpleSignalMessage *psm);
void signal_debug_async(PurpleDebugLevel level, const char *message);

char *purplesignal_init(const char *signal_cli_path, TypedJNIEnv * & sjvm);
void purplesignal_destroy(TypedJNIEnv * & ps);

char *purplesignal_login(TypedJNIEnv *signaljvm, PurpleSignal *ps, uintptr_t connection, const char *username, const char *settings_directory);
int purplesignal_close(const PurpleSignal & ps);
int purplesignal_send(TypedJNIEnv *signaljvm, PurpleSignal *ps, uintptr_t connection, const char *who, const char *message);
