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
        // TODO: switch to C++ version of dirname
        char *path = dirname(g_strdup(lpFilename));
        // convert backslashes to slashes as Java expects
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
#ifndef __USE_GNU
#define __USE_GNU
#endif
#include <dlfcn.h>
char *find_own_path() {
    Dl_info info;
    if (dladdr((void*)(find_own_path), &info)) {
        return dirname(g_strdup(info.dli_fname));
    } else {
        return NULL;
    }
}
#endif

void purplesignal_error(uintptr_t pc, PurpleDebugLevel level, std::string message) {
    /*assert(level > 0);*/
    auto do_in_main_thread = std::make_unique<PurpleSignalConnectionFunction>(
        [level, message] (PurpleConnection *pc) {
            signal_process_error(pc, level, message);
        }
    );
    PurpleSignalMessage *psm = new PurpleSignalMessage(pc, do_in_main_thread);
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

char *purplesignal_init(const char *signal_cli_path, TypedJNIEnv *&sjvm) {
    if (sjvm != nullptr) {
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
        try {
            sjvm = new TypedJNIEnv(vm_args);
        } catch (std::exception & e) {
            return g_strdup(e.what()); // TODO: append res code
        }
        // TODO: convert to std::string
        g_free(librarypath);
        g_free(classpath);
        return NULL;
    }
}

void purplesignal_destroy(TypedJNIEnv * & sjvm) {
    if (sjvm == nullptr) {
        signal_debug(PURPLE_DEBUG_INFO, "Pointer already NULL during purplesignal_destroy(). Assuming no JVM ever started.");
    } else {
		try {
			// TODO: find PurpleSignal class only once, keep as global reference?
			TypedJNIClass psclass = sjvm->find_class("de/hehoe/purple_signal/PurpleSignal");
			signal_debug(PURPLE_DEBUG_INFO, "Waiting for all remaining Java threads…");
			psclass.GetStaticMethod<void()>("joinAllThreads")();
		} catch (std::exception & e) {
			signal_debug(PURPLE_DEBUG_INFO, std::string("Exception while waiting for Java threads: ") + e.what());
		}
        delete sjvm;
        sjvm = nullptr;
    }
}

void purplesignal_exception_check(TypedJNIEnv *tenv) {
    if (tenv->env->ExceptionCheck()) {
        jthrowable jexception = tenv->env->ExceptionOccurred();
        jstring jstr = TypedJNIMethod<jstring()>::get(
            tenv->env, 
            tenv->find_class("java/lang/Object").cls, 
            jexception, 
            "toString"
        )();
        jboolean isCopy = 0;
        const char * utf = tenv->env->GetStringUTFChars(jstr, &isCopy);
        std::string errmsg(utf);
        tenv->env->ReleaseStringUTFChars(jstr, utf);
        throw std::runtime_error(errmsg);
    }
}

void purplesignal_login(TypedJNIEnv* sjvm, PurpleSignal *ps, uintptr_t connection, const char* username, const char * settings_dir) {
    TypedJNIClass psclass = sjvm->find_class("de/hehoe/purple_signal/PurpleSignal");
    ps->instance = std::make_unique<TypedJNIObject>(
        psclass.GetConstructor<jlong,jstring,jstring>()(
            connection, sjvm->make_jstring(username), sjvm->make_jstring(settings_dir)
        )
    );
    ps->send_message = ps->instance->GetMethod<jint(jstring,jstring)>("sendMessage");
    //signal_debug(PURPLE_DEBUG_INFO, "Starting background thread…");
    //ps->instance->GetMethod<void(void)>("startReceiving")();
    purplesignal_exception_check(sjvm);
}

int purplesignal_close(const PurpleSignal & ps) {
    if (ps.instance == nullptr) {
        signal_debug(PURPLE_DEBUG_INFO, "Pointer already NULL during purplesignal_close(). Assuming no connection ever made.");
        return 1;
    }
    try {
        ps.instance->GetMethod<void()>("stopReceiving")();
    } catch (std::exception & e) {
        signal_debug(PURPLE_DEBUG_ERROR, e.what());
        return 0;
    }
    return 1;
}

JNIEXPORT void JNICALL Java_de_hehoe_purple_1signal_PurpleSignal_handleMessageNatively(JNIEnv *env, jclass cls, jlong pc, jstring jchat, jstring jsender, jstring jmessage, jlong timestamp, jint flags) {
    const char *chat = env->GetStringUTFChars(jchat, 0);
    const char *sender = env->GetStringUTFChars(jsender, 0);
    const char *message = env->GetStringUTFChars(jmessage, 0);
    auto do_in_main_thread = std::make_unique<PurpleSignalConnectionFunction>(
        [
            chat = std::string(chat), 
            sender = std::string(sender), 
            message = std::string(message), 
            timestamp, 
            flags = static_cast<PurpleMessageFlags>(flags)
        ] (PurpleConnection *pc) {
            signal_process_message(pc, chat, sender, message, timestamp, flags);
        }
    );
    PurpleSignalMessage *psm = new PurpleSignalMessage(pc, do_in_main_thread);
    env->ReleaseStringUTFChars(jmessage, message);
    env->ReleaseStringUTFChars(jchat, chat);
    env->ReleaseStringUTFChars(jsender, sender);
    signal_handle_message_async(psm);
}

JNIEXPORT void JNICALL Java_de_hehoe_purple_1signal_PurpleSignal_askRegisterOrLink(JNIEnv *env, jclass cls, jlong pc) {
    auto do_in_main_thread = std::make_unique<PurpleSignalConnectionFunction>(
            [] (PurpleConnection *pc) {
                signal_ask_register_or_link(pc);
            }
        );
    PurpleSignalMessage *psm = new PurpleSignalMessage(pc, do_in_main_thread);
    signal_handle_message_async(psm);
}

JNIEXPORT void JNICALL Java_de_hehoe_purple_1signal_PurpleSignal_handleErrorNatively(JNIEnv *env, jclass cls, jlong pc, jstring jmessage) {
    const char *message = env->GetStringUTFChars(jmessage, 0);
    purplesignal_error(pc, PURPLE_DEBUG_ERROR, message);
    env->ReleaseStringUTFChars(jmessage, message);
}

JNIEXPORT void JNICALL Java_de_hehoe_purple_1signal_PurpleSignal_logNatively(JNIEnv *env, jclass cls, jint level, jstring jmessage) {
    const char *message = env->GetStringUTFChars(jmessage, 0);
    signal_debug_async(static_cast<PurpleDebugLevel>(level), message);
    env->ReleaseStringUTFChars(jmessage, message);
}

int purplesignal_send(TypedJNIEnv *sjvm, PurpleSignal & ps, const char *who, const char *message) {
    if (sjvm == nullptr || ps.send_message == nullptr) {
        throw std::runtime_error("PurpleSignal has not been initialized.");
    }
    return ps.send_message(sjvm->make_jstring(who), sjvm->make_jstring(message));
}

void purplesignal_link(TypedJNIEnv *signaljvm, PurpleSignal & ps) {
    ps.instance->GetMethod<void()>("linkAccount")();
}

PurpleSignalMessage::PurpleSignalMessage(uintptr_t pc, std::unique_ptr<PurpleSignalConnectionFunction> & function) : pc(pc), function(std::move(function)) {};
