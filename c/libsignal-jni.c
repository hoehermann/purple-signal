#include "libsignal-jni.h"

int purplesignal_init(PurpleSignal *ps) {
    if (ps->vm != NULL && ps->env != NULL) {
        printf("signal: jni pointers not null. JVM seems to be initialized.");
        return 1;
    } else {
        JavaVMInitArgs vm_args;
        vm_args.version  = JNI_VERSION_1_8;
        vm_args.nOptions = 0;
        jint res = JNI_CreateJavaVM(&ps->vm, (void **)&ps->env, &vm_args);
        return res == JNI_OK;
    }
}

void purplesignal_deinit(PurpleSignal *ps) {
    if (ps && ps->vm && *ps->vm) {
        (*ps->vm)->DestroyJavaVM(ps->vm);
    }
    ps->vm = NULL;
    ps->env = NULL;
}
