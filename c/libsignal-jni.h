#include <jni.h>
#include <stdint.h>

typedef struct {
    JavaVM *vm;
    JNIEnv *env;
} SignalJVM;

typedef struct {
} PurpleSignal;

int purplesignal_init(SignalJVM *ps);
void purplesignal_destroy(SignalJVM *ps);

int purplesignal_login(SignalJVM signaljvm, PurpleSignal *ps, uintptr_t connection, const char* username);
