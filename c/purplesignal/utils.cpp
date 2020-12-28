/*
 * Utility functions when doing JNI stuff.
 */

#include "utils.hpp"
#include <locale>
#include <codecvt>

std::string tjni_jstring_to_stdstring(JNIEnv *env, jstring jstr) {
    // based on https://stackoverflow.com/questions/5859673/
    const jchar * jchars = env->GetStringChars(jstr, NULL); // this probably creates a copy most of the time (at least Intel's implementation does so, says https://www.ibm.com/support/knowledgecenter/SSYKE2_7.0.0/com.ibm.java.lnx.70.doc/diag/understanding/jni_iscopy_flag.html)
    // according to https://stackoverflow.com/questions/7921016/, GetStringUTFChars delivers neither UTF-8 nor standard UTF-16
    // conversion from https://gist.github.com/gchudnov/c1ba72d45e394180e22f
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,char16_t> convert; 
    static_assert(sizeof(jchar) == sizeof(char16_t), "jchar is not as long as char16_t");
    std::string stdstr = convert.to_bytes(reinterpret_cast<const char16_t *>(jchars)); // this always creates another copy
    env->ReleaseStringChars(jstr, jchars);
    return stdstr;
}

void tjni_exception_check(TypedJNIEnv *tenv) {
    if (tenv->env->ExceptionCheck()) {
        jthrowable jexception = tenv->env->ExceptionOccurred();
        if (true) { // TODO: "if purple -d active"
            TypedJNIMethod<void()>::get(
                tenv->env, 
                tenv->find_class("java/lang/Throwable").cls, 
                jexception, 
                "printStackTrace"
            )();
        }
        jstring jstr = TypedJNIMethod<jstring()>::get(
            tenv->env, 
            tenv->find_class("java/lang/Object").cls, 
            jexception, 
            "toString"
        )();
        throw std::runtime_error(tjni_jstring_to_stdstring(tenv->env, jstr));
    }
}
