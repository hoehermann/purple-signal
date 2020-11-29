/*
 * Implementation PurpleSignal (Java) instance management (create instance and login, close).
 */

#include "purplesignal.hpp"
#include "../handler/async.hpp" // for signal_debug (not really needed)
#include "utils.hpp"

PurpleSignal::PurpleSignal(uintptr_t connection, uintptr_t account, TypedJNIEnv *jvm, const std::string & settings_directory, const std::string & username) : 
    jvm(jvm), instance(jvm->find_class("de/hehoe/purple_signal/PurpleSignal").GetConstructor<jlong,jlong,jstring,jstring>()(
        connection, account, jvm->make_jstring(username), jvm->make_jstring(settings_directory)
    )) {
    send_message = instance.GetMethod<jint(jstring,jstring)>("sendMessage");
    tjni_exception_check(jvm);
}

int PurpleSignal::close() {
    try {
        instance.GetMethod<void()>("stopReceiving")();
        tjni_exception_check(jvm);
    } catch (std::exception & e) {
        // this is non-critical (connection is being closed anyway)
        signal_debug(PURPLE_DEBUG_ERROR, std::string("Ignoring exception which occurred while closing connection: ") + e.what());
        return 0;
    }
    return 1;
}