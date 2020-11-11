/*
 * Implementation of all glue (C → Java) for account management (register, link verify).
 */

#include "../libsignal.hpp"
#include "utils.hpp"

void PurpleSignalConnection::register_account(bool voice) {
    ps.instance->GetMethod<void(jboolean)>("registerAccount")(voice);
    tjni_exception_check(ps.jvm);
}

void PurpleSignalConnection::link_account() {
    ps.instance->GetMethod<void()>("linkAccount")();
    tjni_exception_check(ps.jvm);
}

void PurpleSignalConnection::verify_account(const std::string & code, const std::string & pin) {
    ps.instance->GetMethod<void(jstring, jstring)>("verifyAccount")(
        ps.jvm->make_jstring(code), ps.jvm->make_jstring(pin)
    );
    tjni_exception_check(ps.jvm);
}
