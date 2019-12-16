#include <jni.h>
#include <stdint.h>

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
} PurpleSignalMessage;

void signal_handle_message_async(PurpleSignalMessage *psm);

int purplesignal_init(SignalJVM *ps);
void purplesignal_destroy(SignalJVM *ps);

int purplesignal_login(SignalJVM signaljvm, PurpleSignal *ps, uintptr_t connection, const char* username);
int purplesignal_close(SignalJVM sjvm, PurpleSignal *ps);
