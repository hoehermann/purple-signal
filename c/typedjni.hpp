#include <jni.h>
#include <string>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <vector>

namespace TypedJNI {
template <typename T>
std::string GetTypeString(){
    static_assert(std::is_same<T,void>::value, "Cannot handle this type.");
    return "This actually never gets compiled.";
};
template <>
std::string GetTypeString<jstring>();
template <>
std::string GetTypeString<jint>();
template <>
std::string GetTypeString<jlong>();
template <>
std::string GetTypeString<void>();
template<typename T, typename... Args>
typename std::enable_if<sizeof...(Args) != 0, std::string>::type
GetTypeString() {
    std::cerr << "There are 1 + " << sizeof...(Args) << " Args here. First one resolves to " << GetTypeString<T>() << "." << std::endl;
    return GetTypeString<T>() + GetTypeString<Args...>();
};
template<typename... Args>
typename std::enable_if<sizeof...(Args) == 0, std::string>::type
GetTypeString() {
    std::cerr << "There are no Args here. Returning empty string." << std::endl;
    return "";
};

jmethodID GetStaticMethodID(JNIEnv *env, const jclass cls, const std::string name, const std::string & signature);
jmethodID GetMethodID(JNIEnv *env, const jclass cls, const std::string name, const std::string & signature);
} // namespace TypedJNI

// based on https://stackoverflow.com/questions/9065081/
template<typename T> 
class TypedJNIStaticMethod;
template<typename ...Args> 
class TypedJNIStaticMethod<void(Args...)>
{
    public:
    static std::function<void(Args...)> get(JNIEnv *env, const jclass cls, const std::string & name) {
        jmethodID mid = TypedJNI::GetStaticMethodID(env, cls, name, "("+TypedJNI::GetTypeString<Args...>()+")"+TypedJNI::GetTypeString<void>());
        return [env, cls, mid](Args... args) {
            env->CallStaticVoidMethod(cls, mid, args...);
        };
    }
};
template<typename ...Args> 
class TypedJNIStaticMethod<jint(Args...)>
{
    public:
    static std::function<jint(Args...)> get(JNIEnv *env, const jclass cls, const std::string name) {
        const jmethodID mid =  TypedJNI::GetStaticMethodID(env, cls, name, "("+ TypedJNI::GetTypeString<Args...>()+")"+ TypedJNI::GetTypeString<jint>());
        return [env, cls, mid](Args... args)-> jint {
            return env->CallStaticIntMethod(cls, mid, args...);
        };
    }
};

template<typename T> 
class TypedJNIMethod;
template<typename ...Args> 
class TypedJNIMethod<void(Args...)>
{
    public:
    static std::function<void(Args...)> get(JNIEnv *env, const jclass cls, const jobject obj, const std::string & name) {
        jmethodID mid = TypedJNI::GetMethodID(env, cls, name, "("+TypedJNI::GetTypeString<Args...>()+")"+TypedJNI::GetTypeString<void>());
        return [env, obj, mid](Args... args) {
            env->CallVoidMethod(obj, mid, args...);
        };
    }
};
template<typename ...Args> 
class TypedJNIMethod<jint(Args...)>
{
    public:
    static std::function<jint(Args...)> get(JNIEnv *env, const jclass cls, const jobject obj, const std::string name) {
        const jmethodID mid =  TypedJNI::GetMethodID(env, cls, name, "("+ TypedJNI::GetTypeString<Args...>()+")"+ TypedJNI::GetTypeString<jint>());
        return [env, obj, mid](Args... args)-> jint {
            return env->CallIntMethod(obj, mid, args...);
        };
    }
};

class TypedJNIObject {
    private:
    JNIEnv *env;
    jclass cls;
    jobject obj;
    public:
    TypedJNIObject(JNIEnv *env, jclass cls, jobject obj);
    template<typename... Args>
    std::function<Args...> GetMethod(const std::string name) {
        return TypedJNIMethod<Args...>::get(env, cls, obj, name);
    }
    // TODO: clean up object / reference count on destruction
};

template<typename ...Args> 
class TypedJNIConstructor
{
    public:
    static std::function<TypedJNIObject(Args...)> get(JNIEnv *env, const jclass cls) {
        const jmethodID mid = TypedJNI::GetMethodID(env, cls, "<init>", "("+TypedJNI::GetTypeString<Args...>()+")"+TypedJNI::GetTypeString<void>());
        return [env, cls, mid](Args... args) -> TypedJNIObject {
            return TypedJNIObject(env, cls, env->NewObject(cls, mid, args...));
        };
    }
};

class TypedJNIClass {
    private:
    JNIEnv *env;
    public:
    const jclass cls;
    TypedJNIClass(JNIEnv *env, jclass cls);
    template<typename... Args>
    std::function<Args...> GetStaticMethod(const std::string name) {
        return TypedJNIStaticMethod<Args...>::get(env, cls, name);
    }
    template<typename... Args>
    std::function<TypedJNIObject(Args...)> GetConstructor() {
        return TypedJNIConstructor<Args...>::get(env, cls);
    }
};

class TypedJNIString {
    private:
    jstring jstr;
    JNIEnv *env;
    public:
    TypedJNIString(JNIEnv *env, const std::string & str);
    virtual ~TypedJNIString();
    operator jstring() const;
};

class TypedJNIEnv {
    public:
    JavaVM *vm;
    JNIEnv *env;
    TypedJNIEnv(const TypedJNIEnv&) = delete;
    TypedJNIEnv& operator=(const TypedJNIEnv&) = delete;
    TypedJNIEnv(JavaVMInitArgs vm_args);
    virtual ~TypedJNIEnv();
    TypedJNIClass find_class(std::string name);
    TypedJNIString make_jstring(const std::string & str);
};
