/*
 * Implementation PurpleSignal attachment receival (upon C → Java user request).
 */

#include <iostream>
#include "purplesignal.hpp"
#include "utils.hpp"

TypedJNIObject PurpleSignal::acceptAttachment(jobject attachmentPtr, const std::string & local_file_name) {
    jobject input_stream = accept_attachment(attachmentPtr, jvm->make_jstring(local_file_name));
    tjni_exception_check(jvm);
    // TODO: return std::function to read(). Adjust TypedJNI GetMethod so it captures the TypedJNIObject so it is not deleted while there are references to the Methods
    return TypedJNIObject(
        jvm->env, 
        jvm->find_class("java/io/InputStream").cls, 
        input_stream
    );
}

