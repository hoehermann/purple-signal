#include <jni.h>

typedef struct {
    JavaVM *vm;
    JNIEnv *env;
} SignalJVM;

typedef struct {
} PurpleSignal;

int purplesignal_init(SignalJVM *ps);
void purplesignal_deinit(SignalJVM *ps);

int purplesignal_login(SignalJVM signaljvm, PurpleSignal *ps, const char* username);
