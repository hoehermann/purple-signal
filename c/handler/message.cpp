#include "../purple_compat.h"
#include "../libsignal.hpp"

PurpleConversation *signal_find_conversation(const char *username, PurpleAccount *account) {
    PurpleIMConversation *imconv = purple_conversations_find_im_with_account(username, account);
    if (imconv == NULL) {
        imconv = purple_im_conversation_new(account, username);
    }
    PurpleConversation *conv = PURPLE_CONVERSATION(imconv);
    if (conv == NULL) {
        imconv = purple_conversations_find_im_with_account(username, account);
        conv = PURPLE_CONVERSATION(imconv);
    }
    return conv;
}

void
signal_display_message(PurpleConnection *pc, const std::string & chat, const std::string & sender, const std::string & message, const long timestamp, const PurpleMessageFlags flags)
{
    PurpleSignalConnection *sa = static_cast<PurpleSignalConnection*>(purple_connection_get_protocol_data(pc));
    PurpleConversation *conv = signal_find_conversation(chat.c_str(), sa->account);
    purple_conversation_write(conv, sender.c_str(), message.c_str(), flags, timestamp);
}

/*
 * Interprets a message. Handles login success and failure.
 */
void
signal_process_message(PurpleConnection *pc, const std::string & chat, const std::string & sender, const std::string & message, const long timestamp, const PurpleMessageFlags flags)
{
    //PurpleSignalConnection *sa = purple_connection_get_protocol_data(pc);
    //PurpleAccount *account = purple_connection_get_account(pc);
    long t = timestamp / 1000; // in Java, signal timestamps are milliseconds
    if (!t) {
        t = time(NULL);
    }
    signal_display_message(pc, chat, sender, message, t, flags);
}
