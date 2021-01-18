/*
 * Implementation of handlers for incoming message (Java → C).
 * This is all to be executed on the main purple thread.
 */

#include "../connection.hpp"
#include "../libsignal.hpp"
#include "../purple_compat.h"

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
signal_display_message(PurpleConnection *pc, const std::string & chat, const std::string & sender, const std::string & message, const time_t timestamp, const PurpleMessageFlags flags)
{
    PurpleSignalConnection *sa = static_cast<PurpleSignalConnection*>(purple_connection_get_protocol_data(pc));
    PurpleConversation *conv = signal_find_conversation(chat.c_str(), sa->account);
    purple_conversation_write(conv, sender.c_str(), message.c_str(), flags, timestamp);
}

void
signal_process_message(PurpleConnection *pc, const std::string & chat, const std::string & sender, const std::string & message, time_t timestamp, const PurpleMessageFlags flags)
{
    if (
        flags & PURPLE_MESSAGE_SYSTEM && 
        !purple_account_get_bool(purple_connection_get_account(pc), SIGNAL_OPTION_SHOW_SYSTEM, SIGNAL_DEFAULT_SHOW_SYSTEM)
    ) {
        purple_debug_info(SIGNAL_PLUGIN_ID, "Not showing system message: %s", message.c_str());
        return;
    }
    if (!timestamp) {
        timestamp = time(NULL);
    }
    signal_display_message(pc, chat, sender, message, timestamp, flags);
}
