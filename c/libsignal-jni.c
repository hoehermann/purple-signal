#include "libsignal-jni.h"
#include "de_hehoe_purple_signal_PurpleSignal.h"
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <assert.h>

#include <libgen.h>
#if defined(__MINGW32__) || defined(_WIN32)
#define CLASSPATH_SEPARATOR ';'
#include <dirent.h>
#include <windows.h>
#ifndef OWN_FILE_NAME
#error OWN_FILE_NAME not set.
#endif
char *find_own_path() {
    const DWORD nSize = 1024;
    char lpFilename[nSize];
    if (GetModuleFileName(GetModuleHandle(OWN_FILE_NAME), lpFilename, nSize) > 0) {
        char *path = dirname(g_strdup(lpFilename));
        for (char *p = path; *p != 0; p++) {
            if (*p == '\\') {
                *p = '/';
            }
        }
        return path;
    } else {
        return NULL;
    }
}
#else
#define CLASSPATH_SEPARATOR ':'
#define __USE_GNU
#include <dlfcn.h>
char *find_own_path() {
    Dl_info info;
    if (dladdr(find_own_path, &info)) {
        return dirname(g_strdup(info.dli_fname));
    } else {
        return NULL;
    }
}
#endif

void purplesignal_error(uintptr_t pc, int level, const char *message) {
    assert(level > 0);
    PurpleSignalMessage *psm = g_malloc0(sizeof(PurpleSignalMessage));
    psm->pc = pc;
    psm->error = level;
    psm->message = g_strdup(message);
    signal_handle_message_async(psm);
}

char *readdir_of_jars(const char *path, const char *prefix) {
    int signal_cli_jar_found = FALSE;
    char *out = NULL;
    GString *classpath = g_string_new(prefix);
    DIR *dp;
    struct dirent *ep;
    dp = opendir(path);
    if (dp == NULL) {
        out = g_strdup_printf("Could not open directory '%s'.", path);
    } else {
        while ((ep = readdir(dp)) != NULL) {
            const char *extension = strrchr(ep->d_name, '.');
            if (extension != NULL && strcmp(extension, ".jar") == 0) { // consider only .jar files
                g_string_append_printf(classpath, "%c%s/%s", CLASSPATH_SEPARATOR, path, ep->d_name);
                if (strstr(ep->d_name, "signal-cli") != NULL) {
                    signal_cli_jar_found = TRUE;
                }
            }
        }
        closedir(dp);
    }
    if (!signal_cli_jar_found) {
        out = g_strdup_printf("Directory '%s' contained no signal-cli*.jar.", path);
    }
    if (out == NULL) {
        return g_string_free(classpath, FALSE);
    } else {
        g_string_free(classpath, TRUE);
        return out;
    }
}

char *purplesignal_init(const char *signal_cli_path, SignalJVM *sjvm) {
    if (sjvm->vm != NULL && sjvm->env != NULL) {
        signal_debug_async(PURPLE_DEBUG_INFO, "jni pointers not null. JVM seems to be initialized already.");
        return NULL;
    } else {
        char *ownpath = find_own_path();
        // TODO: check for spaces
        // TODO: check whether %s/purple_signal.jar is readable
        char *prefix = g_strdup_printf("-Djava.class.path=%s/purple_signal.jar", ownpath);
        char *classpath = readdir_of_jars(signal_cli_path, prefix);
        g_free(prefix);
        char *librarypath = g_strdup_printf("-Djava.library.path=%s", ownpath);
        g_free(ownpath);
        signal_debug_async(PURPLE_DEBUG_INFO, classpath);
        signal_debug_async(PURPLE_DEBUG_INFO, librarypath);
        if (classpath[0] != '-') {
            // if classpath does not start with a minus, it contains an error message
            g_free(librarypath);
            return classpath;
        }

        JavaVMInitArgs vm_args;
        const unsigned int nOptions = 3;
        JavaVMOption options[nOptions];
        options[0].optionString = classpath;
        options[1].optionString = librarypath;
        options[2].optionString = "";
        if (purple_debug_is_enabled()) {
            options[2].optionString = "-agentlib:jdwp=transport=dt_socket,server=y,suspend=n,address=10044"; 
        }
        vm_args.options = options;
        vm_args.nOptions = nOptions;
        vm_args.version  = JNI_VERSION_1_8;
        jint res = JNI_CreateJavaVM(&sjvm->vm, (void **)&sjvm->env, &vm_args);
        g_free(librarypath);
        g_free(classpath);
        if (res == JNI_OK) {
            return NULL;
        } else {
            return g_strdup("Unable to initialize Java VM."); // TODO: append res code
        };
    }
}

void purplesignal_destroy(SignalJVM *sjvm) {
    if (sjvm && sjvm->vm && *sjvm->vm) {
        (*sjvm->vm)->DestroyJavaVM(sjvm->vm);
    }
    sjvm->vm = NULL;
    sjvm->env = NULL;
}

char * get_exception_message(JNIEnv *env, const char * fallback_message) {
    if ((*env)->ExceptionCheck(env)) {
        jthrowable jexception;
        jexception = (*env)->ExceptionOccurred(env);
        jboolean isCopy = 0;
        jclass jcls = (*env)->FindClass(env, "java/lang/Object");
        jmethodID toString = (*env)->GetMethodID(env, jcls, "toString", "()Ljava/lang/String;");
        jstring jstr = (*env)->CallObjectMethod(env, jexception, toString);
        const char * utf = (*env)->GetStringUTFChars(env, jstr, &isCopy);
        char * errmsg = g_strdup(utf);
        (*env)->ReleaseStringUTFChars(env, jstr, utf);
        return errmsg;
    } else {
        return g_strdup(fallback_message);
    }
}

char *purplesignal_login(SignalJVM sjvm, PurpleSignal *ps, uintptr_t connection, const char* username, const char * settings_dir) {
    ps->class = (*sjvm.env)->FindClass(sjvm.env, "de/hehoe/purple_signal/PurpleSignal");
    if (ps->class == NULL) {
        return get_exception_message(sjvm.env, "Failed to find PurpleSignal class.");
    }
    jmethodID constructor = (*sjvm.env)->GetMethodID(sjvm.env, ps->class, "<init>", "(JLjava/lang/String;Ljava/lang/String;)V");
    if (constructor == NULL) {
        return g_strdup("Failed to find PurpleSignal constructor.");
    }
    // TODO: find out if jstrings need to be destroyed after usage
    // https://stackoverflow.com/questions/6238785/newstringutf-and-freeing-memory
    jstring jusername = (*sjvm.env)->NewStringUTF(sjvm.env, username);
    if (jusername == NULL) {
        return g_strdup("NewStringUTF failed.");
    }
    jstring jsettings_dir = (*sjvm.env)->NewStringUTF(sjvm.env, settings_dir);
    if (jsettings_dir == NULL) {
        return g_strdup("NewStringUTF failed.");
    }
    jlong jconnection = connection;
    ps->instance = (*sjvm.env)->NewObject(sjvm.env, ps->class, constructor, jconnection, jusername, jsettings_dir);
    if (ps->instance == NULL) {
        return get_exception_message(sjvm.env, "Failed to create PurpleSignal instance.");
    }
    jmethodID method = (*sjvm.env)->GetMethodID(sjvm.env, ps->class, "startReceiving", "()V");
    if (method == NULL) {
        return g_strdup("Failed to find method startReceiving.");
    }
    signal_debug_async(PURPLE_DEBUG_INFO, "Starting background thread…");
    (*sjvm.env)->CallVoidMethod(sjvm.env, ps->instance, method);
    return NULL;
}

int purplesignal_close(SignalJVM sjvm, PurpleSignal *ps) {
    if (sjvm.vm == NULL || sjvm.env == NULL || ps->class == NULL || ps->instance == NULL) {
        signal_debug_async(PURPLE_DEBUG_INFO, "Pointer already NULL during purplesignal_close(). Assuming no connection ever made.");
        return 1;
    }
    jmethodID method = (*sjvm.env)->GetMethodID(sjvm.env, ps->class, "stopReceiving", "()V");
    if (method == NULL) {
        signal_debug_async(PURPLE_DEBUG_ERROR, "Failed to find method stopReceiving.");
        return 0;
    }
    (*sjvm.env)->CallVoidMethod(sjvm.env, ps->instance, method);
    return 1;
}

JNIEXPORT void JNICALL Java_de_hehoe_purple_1signal_PurpleSignal_handleMessageNatively(JNIEnv *env, jclass cls, jlong pc, jstring jchat, jstring jsender, jstring jmessage, jlong timestamp, jint flags) {
    const char *chat = (*env)->GetStringUTFChars(env, jchat, 0);
    const char *sender = (*env)->GetStringUTFChars(env, jsender, 0);
    const char *message = (*env)->GetStringUTFChars(env, jmessage, 0);
    PurpleSignalMessage *psm = g_malloc0(sizeof(PurpleSignalMessage));
    psm->pc = pc;
    psm->chat = g_strdup(chat);
    psm->sender = g_strdup(sender);
    psm->message = g_strdup(message);
    psm->timestamp = timestamp;
    psm->flags = flags;
    (*env)->ReleaseStringUTFChars(env, jmessage, message);
    (*env)->ReleaseStringUTFChars(env, jchat, chat);
    (*env)->ReleaseStringUTFChars(env, jsender, sender);
    signal_handle_message_async(psm);
}

JNIEXPORT void JNICALL Java_de_hehoe_purple_1signal_PurpleSignal_handleErrorNatively(JNIEnv *env, jclass cls, jlong pc, jstring jmessage) {
    const char *message = (*env)->GetStringUTFChars(env, jmessage, 0);
    purplesignal_error(pc, PURPLE_DEBUG_ERROR, message);
    (*env)->ReleaseStringUTFChars(env, jmessage, message);
}

JNIEXPORT void JNICALL Java_de_hehoe_purple_1signal_PurpleSignal_logNatively(JNIEnv *env, jclass cls, jint level, jstring jmessage) {
    const char *message = (*env)->GetStringUTFChars(env, jmessage, 0);
    signal_debug_async(level, message);
    (*env)->ReleaseStringUTFChars(env, jmessage, message);
}

int purplesignal_send(SignalJVM sjvm, PurpleSignal *ps, uintptr_t pc, const char *who, const char *message) {
    if (sjvm.vm == NULL || sjvm.env == NULL || ps->class == NULL || ps->instance == NULL) {
        purplesignal_error(pc, PURPLE_DEBUG_ERROR, "PurpleSignal has not been initialized.");
        return 1;
    }
    jmethodID method = (*sjvm.env)->GetMethodID(sjvm.env, ps->class, "sendMessage", "(Ljava/lang/String;Ljava/lang/String;)I");
    if (method == NULL) {
        purplesignal_error(pc, PURPLE_DEBUG_ERROR, "Failed to find method sendMessage.\n");
        return 0;
    }
    jstring jwho = (*sjvm.env)->NewStringUTF(sjvm.env, who);
    jstring jmessage = (*sjvm.env)->NewStringUTF(sjvm.env, message);
    jint ret = (*sjvm.env)->CallIntMethod(sjvm.env, ps->instance, method, jwho, jmessage);
    return ret;
}
