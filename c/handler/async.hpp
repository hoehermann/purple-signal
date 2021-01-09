/*
 * Declaration of handlers for asynchronous calls into purple GTK (Java → C).
 */

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <purple.h> // for PurpleDebugLevel

typedef std::function<void()> PurpleSignalConnectionFunction;

class PurpleSignalMessage {
    public:
    const uintptr_t account;
    const std::unique_ptr<PurpleSignalConnectionFunction> function;
    PurpleSignalMessage(const PurpleSignalMessage&) = delete;
    PurpleSignalMessage& operator=(const PurpleSignalMessage&) = delete;
    PurpleSignalMessage(std::unique_ptr<PurpleSignalConnectionFunction> & function, uintptr_t account);
};

void signal_handle_message_async(PurpleSignalMessage *psm);
void signal_debug(PurpleDebugLevel level, const std::string & message);
