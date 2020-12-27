/*
 * Implementation of native functions.
 * 
 * These are called from Java. They must *absolutely not* throw exceptions.
 * They should defer their work to the appropriate functions to be handled in Pidgin's main thread.
 * The PurpleSignalConnectionFunction body may throw an exception. It is caught in the the main thread.
 */

#include "de_hehoe_purple_signal_PurpleSignal.h"
#include "libsignal.hpp"
#include "connection.hpp"
#include "handler/async.hpp"
#include "handler/account.hpp"
#include "handler/message.hpp"
#include "purplesignal/utils.hpp"

JNIEXPORT void JNICALL Java_de_hehoe_purple_1signal_PurpleSignal_handleQRCodeNatively(JNIEnv *env, jclass cls, jlong pc, jstring jmessage) {
    auto do_in_main_thread = std::make_unique<PurpleSignalConnectionFunction>(
            [
                pc = reinterpret_cast<PurpleConnection *>(pc), 
                device_link_uri = tjni_jstring_to_stdstring(env, jmessage)
            ] () {
                const int zoom_factor = 4; // TODO: make this user-configurable
                std::string qr_code_data = signal_generate_qr_code(device_link_uri, zoom_factor);
                signal_show_qr_code(pc, qr_code_data, device_link_uri);
            }
        );
    PurpleSignalMessage *psm = new PurpleSignalMessage(do_in_main_thread, pc);
    signal_handle_message_async(psm);
}

JNIEXPORT void JNICALL Java_de_hehoe_purple_1signal_PurpleSignal_askRegisterOrLinkNatively(JNIEnv *env, jclass cls, jlong pc) {
    auto do_in_main_thread = std::make_unique<PurpleSignalConnectionFunction>(
            [pc = reinterpret_cast<PurpleConnection *>(pc)] () {
                signal_ask_register_or_link(pc);
            }
        );
    PurpleSignalMessage *psm = new PurpleSignalMessage(do_in_main_thread, pc);
    signal_handle_message_async(psm);
}

JNIEXPORT void JNICALL Java_de_hehoe_purple_1signal_PurpleSignal_askVerificationCodeNatively(JNIEnv *env, jclass cls, jlong pc) {
    auto do_in_main_thread = std::make_unique<PurpleSignalConnectionFunction>(
            [pc = reinterpret_cast<PurpleConnection *>(pc)] () {
                signal_ask_verification_code(pc);
            }
        );
    PurpleSignalMessage *psm = new PurpleSignalMessage(do_in_main_thread, pc);
    signal_handle_message_async(psm);
}

JNIEXPORT void JNICALL Java_de_hehoe_purple_1signal_PurpleSignal_handleErrorNatively(JNIEnv *env, jclass cls, jlong pc, jstring jmessage, jboolean fatal) {
    auto do_in_main_thread = std::make_unique<PurpleSignalConnectionFunction>(
        [message = tjni_jstring_to_stdstring(env, jmessage)] () {
            throw std::runtime_error(message);
        }
    );
    PurpleSignalMessage *psm = new PurpleSignalMessage(do_in_main_thread, pc);
    signal_handle_message_async(psm);
}

JNIEXPORT void JNICALL Java_de_hehoe_purple_1signal_PurpleSignal_logNatively(JNIEnv *env, jclass cls, jint level, jstring jmessage) {
    // writing to the console does not involve the GTK event loop and can happen asynchronously
    signal_debug(static_cast<PurpleDebugLevel>(level), tjni_jstring_to_stdstring(env, jmessage));
}

JNIEXPORT void JNICALL Java_de_hehoe_purple_1signal_PurpleSignal_handleMessageNatively(JNIEnv *env, jclass cls, jlong pc, jstring jchat, jstring jsender, jstring jmessage, jlong timestamp, jint flags) {
    auto do_in_main_thread = std::make_unique<PurpleSignalConnectionFunction>(
        [
            pc = reinterpret_cast<PurpleConnection *>(pc),
            chat = tjni_jstring_to_stdstring(env, jchat), 
            sender = tjni_jstring_to_stdstring(env, jsender), 
            message = tjni_jstring_to_stdstring(env, jmessage), 
            timestamp, 
            flags = static_cast<PurpleMessageFlags>(flags)
        ] () {
            signal_process_message(pc, chat, sender, message, timestamp, flags);
        }
    );
    PurpleSignalMessage *psm = new PurpleSignalMessage(do_in_main_thread, pc);
    signal_handle_message_async(psm);
}

/*
 * Retrieves a string from the connection's account's key value store.
 * I hope it is okay doing this asynchronously.
 */
JNIEXPORT jstring JNICALL Java_de_hehoe_purple_1signal_PurpleSignal_getSettingsStringNatively(JNIEnv *env, jclass, jlong jaccount, jstring jkey, jstring jdefault_value) {
    PurpleAccount * account = reinterpret_cast<PurpleAccount *>(jaccount);
    const std::string key = tjni_jstring_to_stdstring(env, jkey);
    const std::string default_value = tjni_jstring_to_stdstring(env, jdefault_value);
    const std::string value = purple_account_get_string(account, key.c_str(), default_value.c_str());
    return TypedJNIString(env, value).make_persistent();
}

/*
 * Writes a string to the account.
 */
JNIEXPORT void JNICALL Java_de_hehoe_purple_1signal_PurpleSignal_setSettingsStringNatively(JNIEnv *env, jclass, jlong jaccount, jstring jkey, jstring jvalue) {
    uintptr_t account = jaccount;
    auto do_in_main_thread = std::make_unique<PurpleSignalConnectionFunction>(
            [
                account = reinterpret_cast<PurpleAccount *>(account), 
                key = tjni_jstring_to_stdstring(env, jkey), 
                value = tjni_jstring_to_stdstring(env, jvalue)
            ] () {
                purple_account_set_string(account, key.c_str(), value.c_str());
            }
        );
    PurpleSignalMessage *psm = new PurpleSignalMessage(do_in_main_thread, 0, account);
    signal_handle_message_async(psm);
}

/*
 * Looking through all accounts, trying to find one handling the username we are looking for.
 * This is dangerous. I feel dirty.
 */
JNIEXPORT jlong JNICALL Java_de_hehoe_purple_1signal_PurpleSignal_lookupAccountByUsernameNatively(JNIEnv *env, jclass, jstring jusername) {
    PurpleAccount *out = 0;
    const std::string username = tjni_jstring_to_stdstring(env, jusername);
    for (GList *iter = purple_accounts_get_all(); iter != NULL && out == 0; iter = iter->next) {
        PurpleAccount *account = (PurpleAccount *)iter->data;
        const char *u = purple_account_get_username(account);
        const char *id = purple_account_get_protocol_id(account);
        if (!strcmp(SIGNAL_PLUGIN_ID, id) && username == u) {
            out = account;
        };
    }
    return (jlong)out;
}
