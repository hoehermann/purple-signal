/*
 * Implementation of native functions.
 * 
 * These are called from Java. They must *absolutely not* throw exceptions.
 * They should defer their work to the appropriate functions to be handled in Pidgin's main thread.
 * The PurpleSignalConnectionFunction body may throw an exception. It is caught in the the main thread.
 */

#include "de_hehoe_purple_signal_PurpleSignal.h"
#include "libsignal.hpp"
#include "handler/async.hpp"
#include "handler/account.hpp"
#include "handler/message.hpp"

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
            throw std::runtime_error(message);
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

/*
 * Retrieves a string from the connection's account's key value store.
 * I hope it is okay doing this asynchronously.
 */
JNIEXPORT jstring JNICALL Java_de_hehoe_purple_1signal_PurpleSignal_getSettingsStringNatively(JNIEnv *env, jclass, jlong jpc, jstring jkey, jstring jdefault_value) {
    uintptr_t pc = jpc;
    const char *key = env->GetStringUTFChars(jkey, 0);
    const char *default_value = env->GetStringUTFChars(jdefault_value, 0);
    PurpleSignalConnection *sa = (PurpleSignalConnection *)purple_connection_get_protocol_data((PurpleConnection *)pc);
    const char *value = purple_account_get_string(sa->account, key, default_value);
    return env->NewStringUTF(value);
}

/*
 * Writes a string to the account.
 * TODO: Wrap into a PurpleSignalMessage and to this on the main thread?
 */
JNIEXPORT void JNICALL Java_de_hehoe_purple_1signal_PurpleSignal_setSettingsStringNatively(JNIEnv *env, jclass, jlong jpc, jstring jkey, jstring jvalue) {
    uintptr_t pc = jpc;
    const char *key = env->GetStringUTFChars(jkey, 0);
    const char *value = env->GetStringUTFChars(jvalue, 0);
    PurpleSignalConnection *sa = (PurpleSignalConnection *)purple_connection_get_protocol_data((PurpleConnection *)pc);
    purple_account_set_string(sa->account, key, value);
    env->ReleaseStringUTFChars(jkey, key);
    env->ReleaseStringUTFChars(jvalue, value);
}

/*
 * Looking through all currently active connections, trying to find one handling the username we are looking for.
 * This is dangerous. I feel dirty.
 */
JNIEXPORT jlong JNICALL Java_de_hehoe_purple_1signal_PurpleSignal_lookupUsernameNatively(JNIEnv *env, jclass, jstring jusername) {
    PurpleConnection *connection_ptr = 0;
    const char *username = env->GetStringUTFChars(jusername, 0);
    
    {
        GList * connection = purple_connections_get_connecting();
        while (connection != NULL && connection_ptr == 0) {
            PurpleConnection *pc = (PurpleConnection *)connection->data;
            PurpleAccount * account = purple_connection_get_account(pc);
            const char *u = purple_account_get_username(account);
            const char *id = purple_account_get_protocol_id(account);
            if (!strcmp(SIGNAL_PLUGIN_ID, id) && !strcmp(username, u)) {
                connection_ptr = pc;
            };
            connection = connection->next;
        }
    }
    {
        GList * connection = purple_connections_get_all();
        while (connection != NULL && connection_ptr == 0) {
            PurpleConnection *pc = (PurpleConnection *)connection->data;
            PurpleAccount * account = purple_connection_get_account(pc);
            const char *u = purple_account_get_username(account);
            const char *id = purple_account_get_protocol_id(account);
            if (!strcmp(SIGNAL_PLUGIN_ID, id) && !strcmp(username, u)) {
                connection_ptr = pc;
            };
            connection = connection->next;
        }
    }
    
    env->ReleaseStringUTFChars(jusername, username);
    return (jlong)connection_ptr;
}
