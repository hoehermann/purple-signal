#include "libsignal.h"

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
    long t = timestamp;
    if (!t) {
        t = time(NULL);
    }
    signal_display_message(pc,  chat, sender, message, t, flags);
}

/*
 * Interprets an error. Disables the account.
 */
void
signal_process_error(PurpleConnection *pc, const PurpleDebugLevel level, const std::string & message)
{
    purple_debug(level, "signal", "%s\n", message.c_str());
    purple_connection_error(pc, PURPLE_CONNECTION_ERROR_OTHER_ERROR, message.c_str());
}

void
signal_ask_verification_code_ok_cb(PurpleConnection *pc, const char *code) {
    PurpleSignalConnection *sa = static_cast<PurpleSignalConnection *>(purple_connection_get_protocol_data(pc));
    purplesignal_verify(signaljvm, sa->ps, code, "");
}

void
signal_ask_verification_code_cancel_cb(PurpleConnection *pc, const char *code) {
    purple_connection_error(pc, PURPLE_CONNECTION_ERROR_INVALID_USERNAME, "Cannot continue without verification.");
}

void
signal_ask_verification_code(PurpleConnection *pc) {
    PurpleSignalConnection *sa = static_cast<PurpleSignalConnection *>(purple_connection_get_protocol_data(pc));
    purple_request_input(
        pc /*handle*/, 
        "Verification" /*title*/, 
        "Please enter verification code" /*primary*/,
        NULL /*secondary*/, 
        NULL /*default_value*/, 
        false /*multiline*/,
        false /*masked*/, 
        NULL /*hint*/,
        "OK" /*ok_text*/, 
        G_CALLBACK(signal_ask_verification_code_ok_cb),
        "Cancel" /*cancel_text*/, 
        G_CALLBACK(signal_ask_verification_code_cancel_cb),
        sa->account /*account*/, 
        NULL /*who*/, 
        NULL /*conv*/,
        pc /*user_data*/
    );
}

const int SIGNAL_ACCOUNT_LINK = 0;
const int SIGNAL_ACCOUNT_REGISTER = 1;
const int SIGNAL_ACCOUNT_VERIFY = 2;

void
signal_ask_register_or_link_ok_cb(PurpleConnection *pc, int choice) {
    PurpleSignalConnection *sa = static_cast<PurpleSignalConnection *>(purple_connection_get_protocol_data(pc));
    try {
        switch (choice) {
            case SIGNAL_ACCOUNT_LINK: purplesignal_link(signaljvm, sa->ps); break;
            case SIGNAL_ACCOUNT_REGISTER: purplesignal_register(signaljvm, sa->ps, false); break;
            case SIGNAL_ACCOUNT_VERIFY: signal_ask_verification_code(pc); break;
            default: signal_debug(PURPLE_DEBUG_ERROR, "User dialogue returned with invalid choice.");
        }
    } catch (std::exception & e) {
        purple_connection_error(pc, PURPLE_CONNECTION_ERROR_OTHER_ERROR,e.what());
    }   
}

void
signal_ask_register_or_link_cancel_cb(PurpleConnection *pc, int choice) {
    purple_connection_error(pc, PURPLE_CONNECTION_ERROR_INVALID_USERNAME, "Cannot continue without account.");
}

void
signal_ask_register_or_link(PurpleConnection *pc) {
    PurpleSignalConnection *sa = static_cast<PurpleSignalConnection *>(purple_connection_get_protocol_data(pc));
    purple_request_choice(
        pc, 
        "Unknown account", "Please choose",
        NULL, 0,
        "OK", G_CALLBACK(signal_ask_register_or_link_ok_cb),
        "Cancel", G_CALLBACK(signal_ask_register_or_link_cancel_cb),
        sa->account, NULL, NULL, 
        pc, 
        "Link to existing account", SIGNAL_ACCOUNT_LINK, 
        "Register new", SIGNAL_ACCOUNT_REGISTER, 
        "Re-enter existing verification code", SIGNAL_ACCOUNT_VERIFY, 
        NULL
    );
}
