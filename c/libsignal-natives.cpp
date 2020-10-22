/*
 * Implementation of native functions.
 * 
 * These are called from Java. They must *absolutely not* throw exceptions.
 * They should defer their work to the appropriate functions to be handled in Pidgin's main thread.
 * The PurpleSignalConnectionFunction body may throw an exception. It is caught in the the main thread.
 */

#include "de_hehoe_purple_signal_PurpleSignal.h"
#include "libsignal.h"
#include "handler.hpp"
#include "libsignal-account.h"

PurpleSignalMessage::PurpleSignalMessage(uintptr_t pc, std::unique_ptr<PurpleSignalConnectionFunction> & function) : 
    pc(pc), function(std::move(function)) {};

JNIEXPORT void JNICALL Java_de_hehoe_purple_1signal_PurpleSignal_handleQRCodeNatively(JNIEnv *env, jclass cls, jlong pc, jstring jmessage) {
    const char *message = env->GetStringUTFChars(jmessage, 0);
    auto do_in_main_thread = std::make_unique<PurpleSignalConnectionFunction>(
            [device_link_uri = std::string(message)] (PurpleConnection *pc) {
                const int zoom_factor = 4; // TODO: make this user-configurable
                std::string qr_code_data = signal_generate_qr_code(device_link_uri, zoom_factor);
                signal_show_qr_code(pc, qr_code_data, device_link_uri);
            }
        );
    env->ReleaseStringUTFChars(jmessage, message);
    PurpleSignalMessage *psm = new PurpleSignalMessage(pc, do_in_main_thread);
    signal_handle_message_async(psm);
}

JNIEXPORT void JNICALL Java_de_hehoe_purple_1signal_PurpleSignal_askRegisterOrLinkNatively(JNIEnv *env, jclass cls, jlong pc) {
    auto do_in_main_thread = std::make_unique<PurpleSignalConnectionFunction>(
            [] (PurpleConnection *pc) {
                signal_ask_register_or_link(pc);
            }
        );
    PurpleSignalMessage *psm = new PurpleSignalMessage(pc, do_in_main_thread);
    signal_handle_message_async(psm);
}

JNIEXPORT void JNICALL Java_de_hehoe_purple_1signal_PurpleSignal_askVerificationCodeNatively(JNIEnv *env, jclass cls, jlong pc) {
    auto do_in_main_thread = std::make_unique<PurpleSignalConnectionFunction>(
            [] (PurpleConnection *pc) {
                signal_ask_verification_code(pc);
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
