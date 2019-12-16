#include "libsignal-jni.h"
#include "de_hehoe_purple_signal_PurpleSignal.h"

int purplesignal_init(SignalJVM *sjvm) {
    if (sjvm->vm != NULL && sjvm->env != NULL) {
        printf("signal: jni pointers not null. JVM seems to be initialized.\n");
        return 1;
    } else {
        JavaVMInitArgs vm_args;
        const unsigned int nOptions = 1;
        JavaVMOption options[nOptions];
        options[0].optionString = "-Djava.class.path=/opt/signal-cli/lib/argparse4j-0.8.1.jar:/opt/signal-cli/lib/bcprov-jdk15on-1.64.jar:/opt/signal-cli/lib/commons-codec-1.9.jar:/opt/signal-cli/lib/commons-logging-1.2.jar:/opt/signal-cli/lib/core-1.51.0.0.jar:/opt/signal-cli/lib/curve25519-java-0.5.0.jar:/opt/signal-cli/lib/dbus-java-2.7.0.jar:/opt/signal-cli/lib/debug-1.1.1.jar:/opt/signal-cli/lib/hexdump-0.2.1.jar:/opt/signal-cli/lib/httpclient-4.4.jar:/opt/signal-cli/lib/httpcore-4.4.jar:/opt/signal-cli/lib/jackson-annotations-2.9.0.jar:/opt/signal-cli/lib/jackson-core-2.9.9.jar:/opt/signal-cli/lib/jackson-databind-2.9.9.2.jar:/opt/signal-cli/lib/libphonenumber-8.10.7.jar:/opt/signal-cli/lib/okhttp-3.12.1.jar:/opt/signal-cli/lib/okio-1.15.0.jar:/opt/signal-cli/lib/pkix-1.51.0.0.jar:/opt/signal-cli/lib/protobuf-java-2.5.0.jar:/opt/signal-cli/lib/prov-1.51.0.0.jar:/opt/signal-cli/lib/signal-cli-0.6.5.jar:/opt/signal-cli/lib/signal-metadata-java-0.0.3.jar:/opt/signal-cli/lib/signal-protocol-java-2.7.1.jar:/opt/signal-cli/lib/signal-service-java-2.13.9_unofficial_1.jar:/opt/signal-cli/lib/threetenbp-1.3.6.jar:/opt/signal-cli/lib/unix-0.5.1.jar:java/purple_signal.jar"; // TODO: do not hard-code, obviously. maybe https://stackoverflow.com/questions/12489/how-do-you-get-a-directory-listing-in-c with GString and g_string_append_printf
        // TODO: find own location, pass it on to -Djava.library.path=. maybe with https://stackoverflow.com/questions/43409167/find-location-of-loaded-shared-library-from-in-that-shared-library
        vm_args.options = options;
        vm_args.nOptions = nOptions;
        vm_args.version  = JNI_VERSION_1_8;
        jint res = JNI_CreateJavaVM(&sjvm->vm, (void **)&sjvm->env, &vm_args);
        return res == JNI_OK;
    }
}

void purplesignal_destroy(SignalJVM *sjvm) {
    if (sjvm && sjvm->vm && *sjvm->vm) {
        (*sjvm->vm)->DestroyJavaVM(sjvm->vm);
    }
    sjvm->vm = NULL;
    sjvm->env = NULL;
}

int purplesignal_login(SignalJVM sjvm, PurpleSignal *ps, uintptr_t connection, const char* username) {
    jclass cls = (*sjvm.env)->FindClass(sjvm.env, "de/hehoe/purple_signal/PurpleSignal");
    if (cls == NULL) {
        printf("signal: Failed to find class.\n");
        return 0;
    }
    jmethodID constructor = (*sjvm.env)->GetMethodID(sjvm.env, cls, "<init>", "(JLjava/lang/String;)V");
    if (constructor == NULL) {
        printf("signal: Failed to find constructor.\n");
        return 0;
    }
    jstring jusername = (*sjvm.env)->NewStringUTF(sjvm.env, username);
    jobject object = (*sjvm.env)->NewObject(sjvm.env, cls, constructor, connection, jusername);
    if (object == NULL) {
        printf("signal: Failed to create an instance of PurpleSignal.\n");
        return 0;
    }
    jmethodID method = (*sjvm.env)->GetMethodID(sjvm.env, cls, "receiveMessages", "()V");
    if (constructor == NULL) {
        printf("signal: Failed to find method receiveMessages.\n");
        return 0;
    }
    (*sjvm.env)->CallVoidMethod(sjvm.env, object, method);
    return 1;
}


JNIEXPORT void JNICALL Java_de_hehoe_purple_1signal_PurpleSignal_handleMessageNatively(JNIEnv *env, jclass cls, jlong pc, jstring who, jstring message, jlong timestamp) {
    printf("signal: DA NATIVE FUNCTION HAS BEEN CALLED!\n");
}

int main(int argc, char **argv) {
    const char* username = argv[1];
    SignalJVM sjvm = {0};
    printf("purplesignal_init: %d\n", purplesignal_init(&sjvm));
    PurpleSignal ps;
    printf("purplesignal_login: %d\n", purplesignal_login(sjvm, &ps, 0, username));
}
