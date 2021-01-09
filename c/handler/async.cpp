/*
 * Implementation of handlers for asynchronous calls into purple GTK (Java → C).
 */
 
#include "async.hpp"
#include "../purplesignal/error.hpp"
#include "../purple_compat.h"
#include <purple.h>

bool signal_purple_account_exists(PurpleAccount *acc) {
    int account_exists = false;
    for (GList *iter = purple_accounts_get_all(); iter != nullptr && account_exists == false; iter = iter->next) {
        const PurpleAccount *account = (PurpleAccount *)iter->data;
        if (account == acc) {
            account_exists = true;
        };
    }
    return account_exists;
}

PurpleConnection * signal_purple_account_get_connection(PurpleAccount *acc) {
    if (signal_purple_account_exists(acc)) {
        return purple_account_get_connection(acc);
    } else {
        return NULL;
    }
}

/*
 * Handler for a message. Called inside of the GTK eventloop.
 *
 * @return Whether to execute again. Always FALSE.
 */
gboolean
signal_handle_message_mainthread(gpointer data)
{
    PurpleSignalMessage *psm = (PurpleSignalMessage *)data;
    PurpleAccount *acc = (PurpleAccount *)psm->account;
    try {
        (*psm->function)();
    } catch (PurpleSignalError & e) {
        PurpleConnection *pc = signal_purple_account_get_connection(acc);
        if (pc) {
            PurpleConnectionError error = PURPLE_CONNECTION_ERROR_OTHER_ERROR;
            if (!e.is_fatal) {
                // not a fatal error – try to reconnect
                error = PURPLE_CONNECTION_ERROR_NETWORK_ERROR; // this error type is one of the few indicating a non-fatal error
            }
            purple_connection_error(pc, error, e.what());
        } else {
            purple_debug_error(
                "signal", "Error handling asynchronous message without connection: %s\n", e.what()
            );
        }
    } catch (std::exception & e) {
        PurpleConnection *pc = signal_purple_account_get_connection(acc);
        if (pc) {
            purple_connection_error(pc, PURPLE_CONNECTION_ERROR_OTHER_ERROR, e.what());
        } else {
            purple_debug_error(
                "signal", "Error handling asynchronous message without connection: %s\n", e.what()
            );
        }
    }
    delete psm;
    return FALSE;
}

/*
 * Handler for a message received by signal.
 * Called by the JavaVM (outside of the GTK eventloop).
 */
void
signal_handle_message_async(PurpleSignalMessage *psm)
{
    purple_timeout_add(0, signal_handle_message_mainthread, (void*)psm); // yes, this is indeed neccessary – we checked
}

void signal_debug(PurpleDebugLevel level, const std::string & message) {
    purple_debug(level, "signal", "%s\n", message.c_str());
}

PurpleSignalMessage::PurpleSignalMessage(std::unique_ptr<PurpleSignalConnectionFunction> & function, uintptr_t account) : account(account), function(std::move(function)) {};
