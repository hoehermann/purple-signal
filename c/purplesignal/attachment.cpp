/*
 * Implementation PurpleSignal attachment receival (upon C → Java user request).
 */

#include <iostream>
#include "purplesignal.hpp"
#include "utils.hpp"

void PurpleSignal::receiveAttachment(jobject attachmentPtr, const std::string & local_file_name, const uintptr_t xfer) {
    std::cerr << "PurpleSignal::receiveAttachment" << std::endl;
    receive_attachment(attachmentPtr, jvm->make_jstring(local_file_name), xfer);
    tjni_exception_check(jvm);
}

