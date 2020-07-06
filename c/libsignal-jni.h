#include <jni.h>
#include <stdint.h>
#include <purple.h>

typedef struct {
    JavaVM *vm;
    JNIEnv *env;
} SignalJVM;

typedef struct {
    jclass class; // a reference to the PurpleSignal (Java) class.
    jobject instance; // reference to this connection's PurpleSignal (Java) instance.
} PurpleSignal;

typedef struct {
    uintptr_t pc;
    const char *who;
    const char *message;
    signed long timestamp;
    PurpleMessageFlags flags;
    int error;
} PurpleSignalMessage;

void signal_handle_message_async(PurpleSignalMessage *psm);
void signal_debug_async(int level, const char *message);

char *purplesignal_init(const char *signal_cli_path, SignalJVM *sjvm);
void purplesignal_destroy(SignalJVM *ps);

char *purplesignal_login(SignalJVM signaljvm, PurpleSignal *ps, uintptr_t connection, const char *username, const char *settings_directory);
int purplesignal_close(SignalJVM sjvm, PurpleSignal *ps);
int purplesignal_send(SignalJVM signaljvm, PurpleSignal *ps, uintptr_t connection, const char *who, const char *message);
