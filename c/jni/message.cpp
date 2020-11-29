#include "purplesignal.hpp"
#include "utils.hpp"

int PurpleSignal::send(const char *who, const char *message) {
    return send_message(jvm->make_jstring(who), jvm->make_jstring(message));
}

