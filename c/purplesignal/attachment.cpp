/*
 * Implementation PurpleSignal attachment receival (upon C → Java user request).
 */

#include <iostream>
#include <glib.h> // not actually necessary, but I really like g_memdup
#include "purplesignal.hpp"
#include "utils.hpp"

static_assert(std::is_same<guchar **, unsigned char **>::value, "guchar ** must be unsigned char **");

std::function<int(unsigned char **buffer)> PurpleSignal::acceptAttachment(jobject attachment, const std::string & local_file_name) {
    // accept attachment and get reference to Java object
    jobject jinput_stream = accept_attachment(attachment, jvm->make_jstring(local_file_name));
    tjni_exception_check(jvm);
    // wrap into TypedJNIObject, specifying the Java class along the way
    TypedJNIObject input_stream(
        jvm->env, 
        jvm->find_class("java/io/InputStream").cls, 
        jinput_stream
    );
    // get access to the Java InputStream's read(…) method
    auto read_into_buffer = input_stream.GetMethod<jint(jbyteArray)>("read");
    // provide wrapper to caller
    return [
        this,
        read_into_buffer
    ](unsigned char **buffer) -> int {
        jbyteArray jbuffer = jvm->env->NewByteArray(4096);
        if (!jbuffer) {
            throw std::runtime_error("Unable to allocate Java buffer.");
        }
        int read = 0;
        while (read == 0) {
            read = read_into_buffer(jbuffer);
        }
        if (read > 0) {
            jbyte* b = jvm->env->GetByteArrayElements(jbuffer, NULL);
            if (!b) {
                throw std::runtime_error("Unable to access Java buffer.");
            }
            *buffer = static_cast<guchar *>(g_memdup(b, read));
            jvm->env->ReleaseByteArrayElements(jbuffer, b, JNI_ABORT);
        } else if (read == -1) {
            // in purple -1 means "error",
            // in Java, -1 means "end of file",
            // so return 0 for empty last read
            read = 0;
        }
        jvm->env->DeleteLocalRef(jbuffer);
        return read;
    };
}

