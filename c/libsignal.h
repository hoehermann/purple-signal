#pragma once

#include <purple.h>
#include "libsignal-jni.h"
#include <memory>
#include <functional>

/*
 * Holds all information related to this connection instance.
 */
class PurpleSignalConnection {
    public:
    static TypedJNIEnv *jvm; // static member – only one Java VM over all connections
    PurpleAccount *account;
    PurpleConnection *connection;
    PurpleSignal ps;
    PurpleSignalConnection(PurpleAccount *account, PurpleConnection *pc, const std::string & signal_lib_directory);
    void login(const char *username, const std::string & settings_directory);
    int close();
    int send(const char *who, const char *message);
};

void signal_process_error(PurpleConnection *pc, const PurpleDebugLevel level, const std::string & message);
void signal_process_message(PurpleConnection *pc, const std::string & chat, const std::string & sender, const std::string & message, const long timestamp, const PurpleMessageFlags flags);
void signal_ask_register_or_link(PurpleConnection *pc);
void signal_ask_verification_code(PurpleConnection *pc);
