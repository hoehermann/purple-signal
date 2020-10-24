#include "../libsignal.hpp"
#include "utils.hpp"

int PurpleSignalConnection::send(const char *who, const char *message) {
    if (ps.jvm == nullptr || ps.send_message == nullptr) {
        throw std::runtime_error("PurpleSignal has not been initialized.");
    }
    return ps.send_message(*ps.jvm->make_jstring(who), *ps.jvm->make_jstring(message));
}

