#include "libsignal-jni.h"

int purplesignal_init(SignalJVM *signalvm) {
    if (signalvm->vm != NULL && signalvm->env != NULL) {
        printf("signal: jni pointers not null. JVM seems to be initialized.\n");
        return 1;
    } else {
        JavaVMInitArgs vm_args;
        const unsigned int nOptions = 1;
        JavaVMOption options[nOptions];
        options[0].optionString = "-Djava.class.path=/opt/signal-cli/lib/argparse4j-0.8.1.jar:/opt/signal-cli/lib/bcprov-jdk15on-1.64.jar:/opt/signal-cli/lib/commons-codec-1.9.jar:/opt/signal-cli/lib/commons-logging-1.2.jar:/opt/signal-cli/lib/core-1.51.0.0.jar:/opt/signal-cli/lib/curve25519-java-0.5.0.jar:/opt/signal-cli/lib/dbus-java-2.7.0.jar:/opt/signal-cli/lib/debug-1.1.1.jar:/opt/signal-cli/lib/hexdump-0.2.1.jar:/opt/signal-cli/lib/httpclient-4.4.jar:/opt/signal-cli/lib/httpcore-4.4.jar:/opt/signal-cli/lib/jackson-annotations-2.9.0.jar:/opt/signal-cli/lib/jackson-core-2.9.9.jar:/opt/signal-cli/lib/jackson-databind-2.9.9.2.jar:/opt/signal-cli/lib/libphonenumber-8.10.7.jar:/opt/signal-cli/lib/okhttp-3.12.1.jar:/opt/signal-cli/lib/okio-1.15.0.jar:/opt/signal-cli/lib/pkix-1.51.0.0.jar:/opt/signal-cli/lib/protobuf-java-2.5.0.jar:/opt/signal-cli/lib/prov-1.51.0.0.jar:/opt/signal-cli/lib/signal-cli-0.6.5.jar:/opt/signal-cli/lib/signal-metadata-java-0.0.3.jar:/opt/signal-cli/lib/signal-protocol-java-2.7.1.jar:/opt/signal-cli/lib/signal-service-java-2.13.9_unofficial_1.jar:/opt/signal-cli/lib/threetenbp-1.3.6.jar:/opt/signal-cli/lib/unix-0.5.1.jar:java/purple_signal.jar"; // TODO: do not hard-code, obviously. maybe https://stackoverflow.com/questions/12489/how-do-you-get-a-directory-listing-in-c with GString and g_string_append_printf
        vm_args.options = options;
        vm_args.nOptions = nOptions;
        vm_args.version  = JNI_VERSION_1_8;
        jint res = JNI_CreateJavaVM(&signalvm->vm, (void **)&signalvm->env, &vm_args);
        return res == JNI_OK;
    }
}

void purplesignal_deinit(SignalJVM *signalvm) {
    if (signalvm && signalvm->vm && *signalvm->vm) {
        (*signalvm->vm)->DestroyJavaVM(signalvm->vm);
    }
    signalvm->vm = NULL;
    signalvm->env = NULL;
}

int purplesignal_login(SignalJVM signalvm) {
    jclass cls = (*signalvm.env)->FindClass(signalvm.env, "de/hehoe/purple_signal/PurpleSignal");
    if (cls == NULL) {
        printf("signal: Failed to find class.\n");
        return 0;
    }
    jmethodID constructor = (*signalvm.env)->GetMethodID(signalvm.env, cls, "<init>", "(Ljava/lang/String;)V");
	if (constructor == NULL) {
		printf("signal: Failed to find constructor.\n");
		return 0;
	}
    return 1;
}

int main(int argc, char **argv) {
    SignalJVM signalvm = {0};
    printf("purplesignal_init: %d\n", purplesignal_init(&signalvm));
    printf("purplesignal_login: %d\n", purplesignal_login(signalvm));
}
