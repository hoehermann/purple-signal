/*
 * Utility functions when doing JNI stuff.
 */

#include "utils.hpp"

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
        jboolean isCopy = 0;
        const char * utf = tenv->env->GetStringUTFChars(jstr, &isCopy);
        std::string errmsg(utf);
        tenv->env->ReleaseStringUTFChars(jstr, utf);
        throw std::runtime_error(errmsg);
    }
}
