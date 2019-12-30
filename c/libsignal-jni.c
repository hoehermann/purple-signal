#include "libsignal-jni.h"
#include "de_hehoe_purple_signal_PurpleSignal.h"
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <assert.h>

/**
 * Debug levels.
 *
 * from libpurple/debug.h
 * I do not want this module to depend on purple directly, though.
 */
typedef enum
{
	PURPLE_DEBUG_ALL = 0,  /**< All debug levels.              */
	PURPLE_DEBUG_MISC,     /**< General chatter.               */
	PURPLE_DEBUG_INFO,     /**< General operation Information. */
	PURPLE_DEBUG_WARNING,  /**< Warnings.                      */
	PURPLE_DEBUG_ERROR,    /**< Errors.                        */
	PURPLE_DEBUG_FATAL     /**< Fatal errors.                  */

} PurpleDebugLevel;

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
        signal_debug_async(PURPLE_DEBUG_INFO, "jni pointers not null. JVM seems to be initialized already.\n");
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
        if (classpath[0] != '-') {
            g_free(librarypath);
            return classpath;
        }

        JavaVMInitArgs vm_args;
        const unsigned int nOptions = 3;
        JavaVMOption options[nOptions];
        options[0].optionString = classpath;
        options[1].optionString = librarypath;
        options[2].optionString = "-agentlib:jdwp=transport=dt_socket,server=y,suspend=n,address=10044"; // TODO: conditionally enable this only in debug builds. Or if Pidgin was started in debug mode?
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

const char *purplesignal_login(SignalJVM sjvm, PurpleSignal *ps, uintptr_t connection, const char* username) {
    signal_debug_async(PURPLE_DEBUG_INFO, "Looking up class…");
    ps->class = (*sjvm.env)->FindClass(sjvm.env, "de/hehoe/purple_signal/PurpleSignal");
    if (ps->class == NULL) {
        (*sjvm.env)->ExceptionDescribe(sjvm.env);
        return "Failed to find PurpleSignal class (more information might be printed to the command-line).";
    }
    signal_debug_async(PURPLE_DEBUG_INFO, "Looking up constructor…");
    jmethodID constructor = (*sjvm.env)->GetMethodID(sjvm.env, ps->class, "<init>", "(JLjava/lang/String;)V");
    if (constructor == NULL) {
        return "Failed to find PurpleSignal constructor.";
    }
    jstring jusername = (*sjvm.env)->NewStringUTF(sjvm.env, username);
    // TODO: find out if jstrings need to be destroyed after usage
    signal_debug_async(PURPLE_DEBUG_INFO, "Creating new instance…");
    ps->instance = (*sjvm.env)->NewObject(sjvm.env, ps->class, constructor, connection, jusername);
    if (ps->instance == NULL) {
        return "Failed to create an instance of PurpleSignal.";
    }
    signal_debug_async(PURPLE_DEBUG_INFO, "Looking up mwthod…");
    jmethodID method = (*sjvm.env)->GetMethodID(sjvm.env, ps->class, "startReceiving", "()V");
    if (method == NULL) {
        return "Failed to find method startReceiving.";
    }
    signal_debug_async(PURPLE_DEBUG_INFO, "Starting background thread…");
    (*sjvm.env)->CallVoidMethod(sjvm.env, ps->instance, method);
    return NULL;
}

int purplesignal_close(SignalJVM sjvm, PurpleSignal *ps) {
    if (sjvm.vm == NULL || sjvm.env == NULL || ps->class == NULL || ps->instance == NULL) {
        signal_debug_async(PURPLE_DEBUG_INFO, "Pointer already NULL during purplesignal_close(). Assuming no connection ever made.\n");
        return 1;
    }
    jmethodID method = (*sjvm.env)->GetMethodID(sjvm.env, ps->class, "stopReceiving", "()V");
    if (method == NULL) {
        signal_debug_async(PURPLE_DEBUG_ERROR, "Failed to find method stopReceiving.\n");
        return 0;
    }
    (*sjvm.env)->CallVoidMethod(sjvm.env, ps->instance, method);
    return 1;
}

JNIEXPORT void JNICALL Java_de_hehoe_purple_1signal_PurpleSignal_handleMessageNatively(JNIEnv *env, jclass cls, jlong pc, jstring jwho, jstring jmessage, jlong timestamp) {
    signal_debug_async(PURPLE_DEBUG_INFO, "DA NATIVE FUNCTION HAS BEEN CALLED!\n");
    const char *who = (*env)->GetStringUTFChars(env, jwho, 0);
    const char *message = (*env)->GetStringUTFChars(env, jmessage, 0);
    PurpleSignalMessage *psm = g_malloc0(sizeof(PurpleSignalMessage));
    psm->pc = pc;
    psm->who = g_strdup(who);
    psm->message = g_strdup(message);
    psm->timestamp = timestamp;
    (*env)->ReleaseStringUTFChars(env, jmessage, message);
    (*env)->ReleaseStringUTFChars(env, jwho, who);
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
    jmethodID method = (*sjvm.env)->GetMethodID(sjvm.env, ps->class, "sendMessage", "(Ljava/lang/String;Ljava/lang/String;)V");
    if (method == NULL) {
        purplesignal_error(pc, PURPLE_DEBUG_ERROR, "Failed to find method sendMessage.\n");
        return 0;
    }
    jstring jwho = (*sjvm.env)->NewStringUTF(sjvm.env, who);
    jstring jmessage = (*sjvm.env)->NewStringUTF(sjvm.env, message);
    (*sjvm.env)->CallVoidMethod(sjvm.env, ps->instance, method, jwho, jmessage);
    // TODO: call with int as return type
    return 1;
}

int main(int argc, char **argv) {
    const char* username = argv[1];
    SignalJVM sjvm = {0};
    printf("purplesignal_init: %s\n", purplesignal_init(".", &sjvm));
    PurpleSignal ps;
    printf("purplesignal_login: %s\n", purplesignal_login(sjvm, &ps, 0, username));
}
