/*
 * Declaration of handlers for asynchronous calls into purple GTK (Java → C).
 */

#pragma once

#include <memory>
#include <functional>
#include <purple.h>

typedef std::function<void(PurpleConnection *pc)> PurpleSignalConnectionFunction;

class PurpleSignalMessage {
    public:
    const uintptr_t pc;
    const std::unique_ptr<PurpleSignalConnectionFunction> function;
    PurpleSignalMessage(const PurpleSignalMessage&) = delete;
    PurpleSignalMessage& operator=(const PurpleSignalMessage&) = delete;
    PurpleSignalMessage(uintptr_t pc, std::unique_ptr<PurpleSignalConnectionFunction> & function);
};

void signal_handle_message_async(PurpleSignalMessage *psm);
void signal_debug(PurpleDebugLevel level, const std::string & message);
