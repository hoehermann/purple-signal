/*
 * Implementation PurpleSignal (Java) instance management (create instance and login, close).
 */

#include "purplesignal.hpp"
#include "utils.hpp"
#include "../handler/async.hpp" // for signal_debug (not really needed)

PurpleSignal::PurpleSignal(TypedJNIEnv *jvm, uintptr_t account, const std::string & username) : 
    jvm(jvm), instance(jvm->find_class("de/hehoe/purple_signal/PurpleSignal").GetConstructor<jlong,jstring>()(
        account, jvm->make_jstring(username)
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