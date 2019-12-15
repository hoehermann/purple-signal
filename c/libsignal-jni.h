#include <jni.h>

typedef struct {
    JavaVM *vm;
    JNIEnv *env;
} SignalJVM;

int purplesignal_init(SignalJVM *ps);
void purplesignal_deinit(SignalJVM *ps);
