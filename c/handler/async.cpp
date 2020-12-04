/*
 * Implementation of handlers for asynchronous calls into purple GTK (Java → C).
 */
 
#include <purple.h>
#include "../purple_compat.h"
#include "async.hpp"

gboolean
signal_check_connection_existance(PurpleConnection *pc) {
    int connection_exists = false;
    {
        GList * connection = purple_connections_get_connecting();
        while (connection != NULL && connection_exists == false) {
            connection_exists = connection->data == pc;
            connection = connection->next;
        }
    }
    {
        GList * connection = purple_connections_get_all();
        while (connection != NULL && connection_exists == false) {
            connection_exists = connection->data == pc;
            connection = connection->next;
        }
    }
    return connection_exists;
}

gboolean
signal_check_account_existance(PurpleAccount *acc) {
    int account_exists = false;
    for (GList *iter = purple_accounts_get_all(); iter != nullptr && account_exists == false; iter = iter->next) {
        const PurpleAccount *account = (PurpleAccount *)iter->data;
        if (account == acc) {
            account_exists = true;
        };
    }
    return account_exists;
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
    PurpleConnection *pc = (PurpleConnection *)psm->pc;
    PurpleAccount *acc = (PurpleAccount *)psm->account;
    bool execute = false;
    // TODO: move this check into psm->function?
    if (acc != 0) {
        if (signal_check_account_existance(acc)) {
            execute = true;
        } else {
            purple_debug_info(
                "signal", "Not handling message for non-existant account %p.\n", acc
            );
        }
    }
    if (pc != 0) {
        if (signal_check_connection_existance(pc)) {
            execute = true;
        } else {
            purple_debug_info(
                "signal", "Not handling message for non-existant connection %p.\n", pc
            );
        }
    }
    if (execute) {
        try {
            (*psm->function)();
        } catch (std::exception & e) {
            if (pc != 0) {
                purple_connection_error(pc, PURPLE_CONNECTION_ERROR_OTHER_ERROR, e.what());
            } else {
                purple_debug_error(
                    "signal", "Error handling asynchronous message without connection: %s\n", e.what()
                );
            }
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


PurpleSignalMessage::PurpleSignalMessage(std::unique_ptr<PurpleSignalConnectionFunction> & function, uintptr_t pc, uintptr_t account) : 
    pc(pc), account(account), function(std::move(function)) {};
