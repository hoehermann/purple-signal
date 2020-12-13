#pragma once

#include "../submodules/typedjni/typedjni.hpp"

/**
 * Checks whether an exception has occurred in the Java Runtime Environment.
 * 
 * Asks the Java VM to print the stack-trace.
 * 
 * Re-throws the exception message as std::runtime_error.
 */
void tjni_exception_check(TypedJNIEnv *tenv);

std::string tjni_jstring_to_stdstring(JNIEnv *env, jstring jstr);
