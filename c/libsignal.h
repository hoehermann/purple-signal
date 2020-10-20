#pragma once

#include <purple.h>
#include "libsignal-jni.h"
#include <memory>
#include <functional>

/*
 * Holds all information related to this account (connection) instance.
 */
typedef struct {
    PurpleAccount *account;
    PurpleConnection *pc;
    PurpleSignal ps;
} SignalAccount;

void signal_process_error(PurpleConnection *pc, const PurpleDebugLevel level, const std::string & message);
void signal_process_message(PurpleConnection *pc, const std::string & chat, const std::string & sender, const std::string & message, const long timestamp, const PurpleMessageFlags flags);
void signal_ask_register_or_link(PurpleConnection *pc);
