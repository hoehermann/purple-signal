/*
 * Implementation of native functions.
 * These are called from Java. They must absolutely not throw exceptions.
 * They should defer their work to the appropriate functions to be handled in Pidgin's main thread.
 */

#include "libsignal-jni.h"
#include "de_hehoe_purple_signal_PurpleSignal.h"

JNIEXPORT void JNICALL Java_de_hehoe_purple_1signal_PurpleSignal_askRegisterOrLinkNatively(JNIEnv *env, jclass cls, jlong pc) {
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
    auto do_in_main_thread = std::make_unique<PurpleSignalConnectionFunction>(
        [message = std::string(message)] (PurpleConnection *pc) {
            signal_process_error(pc, PURPLE_DEBUG_ERROR, message);
        }
    );
    env->ReleaseStringUTFChars(jmessage, message);
    PurpleSignalMessage *psm = new PurpleSignalMessage(pc, do_in_main_thread);
    signal_handle_message_async(psm);
}

JNIEXPORT void JNICALL Java_de_hehoe_purple_1signal_PurpleSignal_logNatively(JNIEnv *env, jclass cls, jint level, jstring jmessage) {
    const char *message = env->GetStringUTFChars(jmessage, 0);
    // writing to the console does not involve the GTK event loop and can happen asynchronously
    signal_debug(static_cast<PurpleDebugLevel>(level), message);
    env->ReleaseStringUTFChars(jmessage, message);
}
