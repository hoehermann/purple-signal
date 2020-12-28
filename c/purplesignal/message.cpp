/*
 * Implementation PurpleSignal (C → Java) messaging (send message).
 * Message reception is handled by ../natives.cpp and deferred to ../handler/message.cpp.
 */

#include "purplesignal.hpp"
#include "utils.hpp"

int PurpleSignal::send_im(const std::string & who, const std::string & message) {
    int ret = send_message(jvm->make_jstring(who), jvm->make_jstring(message));
    tjni_exception_check(jvm);
    return ret;
}

