/*
 * Implementation of handlers for asynchronous calls into purple GTK (Java → C).
 */
 
#include <purple.h>
#include "../purple_compat.h"
#include "async.hpp"

gboolean
signal_check_connection_existance(PurpleConnection *pc) {
    int connection_exists = 0;
    {
        GList * connection = purple_connections_get_connecting();
        while (connection != NULL && connection_exists == 0) {
            connection_exists = connection->data == pc;
            connection = connection->next;
        }
    }
    {
        GList * connection = purple_connections_get_all();
        while (connection != NULL && connection_exists == 0) {
            connection_exists = connection->data == pc;
            connection = connection->next;
        }
    }
    return connection_exists;
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
    int connection_exists = signal_check_connection_existance(pc);
    if (connection_exists == 0) {
        purple_debug_info(
            "signal", "Not handling message for non-existant connection %p.\n", pc
        );
    } else {
        try {
            (*psm->function)(pc);
        } catch (std::exception & e) {
            purple_connection_error(pc, PURPLE_CONNECTION_ERROR_OTHER_ERROR, e.what());
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


PurpleSignalMessage::PurpleSignalMessage(uintptr_t pc, std::unique_ptr<PurpleSignalConnectionFunction> & function) : 
    pc(pc), function(std::move(function)) {};