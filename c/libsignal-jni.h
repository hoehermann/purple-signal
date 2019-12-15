#include <jni.h>

typedef struct {
    JavaVM *vm;
    JNIEnv *env;
} PurpleSignal;

int purplesignal_init(PurpleSignal *ps);
void purplesignal_deinit(PurpleSignal *ps);
