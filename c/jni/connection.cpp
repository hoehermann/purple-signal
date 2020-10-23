/*
 * Implementation of all glue (C → Java) for connection management (establish / login, close).
 */

#include "../libsignal.hpp"
#include "../handler/async.hpp"
#include "utils.hpp"

void PurpleSignalConnection::login(const char* username, const std::string & settings_dir) {
    TypedJNIClass psclass = ps.jvm->find_class("de/hehoe/purple_signal/PurpleSignal");
    ps.instance = std::make_unique<TypedJNIObject>(
        psclass.GetConstructor<jlong,jstring,jstring>()(
            uintptr_t(connection), ps.jvm->make_jstring(username), ps.jvm->make_jstring(settings_dir)
        )
    );
    ps.send_message = ps.instance->GetMethod<jint(jstring,jstring)>("sendMessage");
    //signal_debug(PURPLE_DEBUG_INFO, "Starting background thread…");
    //ps->instance->GetMethod<void(void)>("startReceiving")();
    tjni_exception_check(ps.jvm);
}

int PurpleSignalConnection::close() {
    if (ps.instance == nullptr) {
        signal_debug(PURPLE_DEBUG_INFO, "Pointer already NULL during purplesignal_close(). Assuming no connection ever made.");
        return 1;
    }
    try {
        ps.instance->GetMethod<void()>("stopReceiving")();
        tjni_exception_check(ps.jvm);
    } catch (std::exception & e) {
        // this is non-critical (connection is being closed anyway)
        signal_debug(PURPLE_DEBUG_ERROR, e.what());
        return 0;
    }
    return 1;
}