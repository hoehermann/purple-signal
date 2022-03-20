/*
 * Implementation of all glue (C → Java) for account management (register, link verify).
 */

#include "utils.hpp"
#include "purplesignal.hpp"

#include <iostream> // for debugging

void PurpleSignal::register_account(bool voice, const std::string & captcha) {
    std::cerr << "PurpleSignal::register_account pre" << std::endl;
    instance.GetMethod<void(jboolean, jstring)>("registerAccount")(voice, jvm->make_jstring(captcha));
    std::cerr << "PurpleSignal::register_account post" << std::endl;
    tjni_exception_check(jvm);
    std::cerr << "PurpleSignal::register_account fin" << std::endl;
}

void PurpleSignal::link_account() {
    instance.GetMethod<void()>("linkAccount")();
    tjni_exception_check(jvm);
}

void PurpleSignal::verify_account(const std::string & code, const std::string & captcha) {
    instance.GetMethod<void(jstring, jstring)>("verifyAccount")(
        jvm->make_jstring(code), jvm->make_jstring(captcha)
    );
    tjni_exception_check(jvm);
}
