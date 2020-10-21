/*
 *   signal plugin for libpurple
 *   Copyright (C) 2019 Hermann Höhne
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

 /*
  * Please note this is the third purple plugin I have ever written.
  * I still have no idea what I am doing.
  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __GNUC__
#include <unistd.h>
#endif

#define _(a) (a)

#include "purple_compat.h"
#if PURPLE_VERSION_CHECK(3, 0, 0)
#error Purple3 not supported.
#endif

#define SIGNAL_PLUGIN_ID "prpl-hehoe-signal"
#ifndef SIGNAL_PLUGIN_VERSION
#error Must set SIGNAL_PLUGIN_VERSION in Makefile
#endif
#define SIGNAL_PLUGIN_WEBSITE "https://github.com/hoehermann/purple-signal"

#define SIGNAL_STATUS_STR_ONLINE   "online"
#define SIGNAL_STATUS_STR_OFFLINE  "offline"
#define SIGNAL_STATUS_STR_MOBILE   "mobile"

#define SIGNAL_OPTION_LIBDIR "signal-cli-lib-dir"
#define SIGNAL_DEFAULT_LIBDIR ""
#define SIGNAL_OPTION_SETTINGS_DIR "signal-cli-settings-dir"
#define SIGNAL_DEFAULT_SETTINGS_DIR ""

#include "libsignal.h"

TypedJNIEnv *signaljvm = nullptr; // only one Java VM over all connections

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
    SignalAccount *sa = static_cast<SignalAccount*>(purple_connection_get_protocol_data(pc));
    PurpleConversation *conv = signal_find_conversation(chat.c_str(), sa->account);
    purple_conversation_write(conv, sender.c_str(), message.c_str(), flags, timestamp);
}

/*
 * Interprets a message. Handles login success and failure.
 */
void
signal_process_message(PurpleConnection *pc, const std::string & chat, const std::string & sender, const std::string & message, const long timestamp, const PurpleMessageFlags flags)
{
    //SignalAccount *sa = purple_connection_get_protocol_data(pc);
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

const int SIGNAL_ACCOUNT_LINK = 0;
const int SIGNAL_ACCOUNT_REGISTER = 1;

void
signal_ask_register_or_link_ok_cb(PurpleConnection *pc, int choice) {
    SignalAccount *sa = static_cast<SignalAccount *>(purple_connection_get_protocol_data(pc));
    if (choice == SIGNAL_ACCOUNT_LINK) {
        purplesignal_link(signaljvm, sa->ps);
    } else {
        purple_connection_error(pc, PURPLE_CONNECTION_ERROR_AUTHENTICATION_FAILED, "Registering not implemented.");
    }
}

void
signal_ask_register_or_link_cancel_cb(PurpleConnection *pc, int choice) {
    purple_connection_error(pc, PURPLE_CONNECTION_ERROR_INVALID_USERNAME, "Cannot continue without account.");
}

void
signal_ask_register_or_link(PurpleConnection *pc) {
    SignalAccount *sa = static_cast<SignalAccount *>(purple_connection_get_protocol_data(pc));
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
        NULL
    );
}

extern "C" {

static const char *
signal_list_icon(PurpleAccount *account, PurpleBuddy *buddy)
{
    return "signal";
}

void
signal_login(PurpleAccount *account)
{
    PurpleConnection *pc = purple_account_get_connection(account);

    const char *libdir = purple_account_get_string(account, SIGNAL_OPTION_LIBDIR, SIGNAL_DEFAULT_LIBDIR);
    if (!libdir[0]) {
        purple_connection_error(
            pc, 
            PURPLE_CONNECTION_ERROR_OTHER_ERROR, 
            _("Path to signal-cli's lib directory is empty. Set it appropriately (e.g. /opt/signal-cli/lib).")
        );
        return;
    }
    char *errormsg = purplesignal_init(libdir, signaljvm);
    if (errormsg) {
        purple_connection_error(pc, PURPLE_CONNECTION_ERROR_OTHER_ERROR, errormsg);
        g_free(errormsg);
        return;
    } else {
        purple_debug_info("signal", "JVM seems to have been initalized.\n");
    }

    // this protocol does not support anything special right now
    PurpleConnectionFlags pc_flags;
    pc_flags = purple_connection_get_flags(pc);
    pc_flags = (PurpleConnectionFlags)(pc_flags 
    | PURPLE_CONNECTION_NO_IMAGES
    | PURPLE_CONNECTION_NO_FONTSIZE
    | PURPLE_CONNECTION_NO_NEWLINES // TODO: find out how signal represents newlines, use them
    | PURPLE_CONNECTION_NO_BGCOLOR);
    purple_connection_set_flags(pc, pc_flags);

    SignalAccount *sa = g_new0(SignalAccount, 1);
    purple_connection_set_protocol_data(pc, sa);
    sa->account = account;
    sa->pc = pc;

    std::string settings_dir(purple_account_get_string(account, SIGNAL_OPTION_SETTINGS_DIR, SIGNAL_DEFAULT_SETTINGS_DIR));
    if (settings_dir == "") {
        settings_dir = std::string(purple_user_dir()) + "/signal";
    }
    purple_connection_set_state(pc, PURPLE_CONNECTION_CONNECTING);
    try {
        purplesignal_login(
            signaljvm, &sa->ps, (uintptr_t)pc, 
            purple_account_get_username(account),
            settings_dir
        );
        purple_connection_set_state(pc, PURPLE_CONNECTION_CONNECTED);
    } catch (std::exception & e) {
        purple_connection_error(pc, PURPLE_CONNECTION_ERROR_OTHER_ERROR, e.what());
    }
}

static void
signal_close(PurpleConnection *pc)
{
    SignalAccount *sa = (SignalAccount *)purple_connection_get_protocol_data(pc);
    purplesignal_close(sa->ps);
    g_free(sa);
}

static GList *
signal_status_types(PurpleAccount *account)
{
    GList *types = NULL;
    PurpleStatusType *status;

    status = purple_status_type_new_full(PURPLE_STATUS_AVAILABLE, SIGNAL_STATUS_STR_ONLINE, _("Online"), TRUE, TRUE, FALSE);
    types = g_list_append(types, status);

    status = purple_status_type_new_full(PURPLE_STATUS_OFFLINE, SIGNAL_STATUS_STR_OFFLINE, _("Offline"), TRUE, TRUE, FALSE);
    types = g_list_append(types, status);

    status = purple_status_type_new_full(PURPLE_STATUS_MOBILE, SIGNAL_STATUS_STR_MOBILE, NULL, FALSE, FALSE, TRUE);
    types = g_list_prepend(types, status);

    return types;
}

static int
signal_send_im(PurpleConnection *pc, const gchar *who, const gchar *message, PurpleMessageFlags flags)
{
    SignalAccount *sa = (SignalAccount *)purple_connection_get_protocol_data(pc);
    try {
        return purplesignal_send(signaljvm, sa->ps, who, message);
    } catch (std::exception & e) {
        purple_connection_error(pc, PURPLE_CONNECTION_ERROR_OTHER_ERROR, e.what());
        return -1;
    }
}

static void
signal_add_buddy(PurpleConnection *pc, PurpleBuddy *buddy, PurpleGroup *group)
{
    // does not actually do anything. buddy is added to pidgin's local list and is usable from there.
}

static GList *
signal_add_account_options(GList *account_options)
{
    PurpleAccountOption *option;
    
    option = purple_account_option_string_new(
                _("signal-cli's lib directory (containing .jar files)"),
                SIGNAL_OPTION_LIBDIR,
                SIGNAL_DEFAULT_LIBDIR
                );
    account_options = g_list_append(account_options, option);
    
    option = purple_account_option_string_new(
                _("signal-cli's settings directory (leave empty for purple user dir)"),
                SIGNAL_OPTION_SETTINGS_DIR,
                SIGNAL_DEFAULT_SETTINGS_DIR
                );
    account_options = g_list_append(account_options, option);
    
    return account_options;
}

static GList *
signal_actions(PurplePlugin *plugin, gpointer context)
{
    GList *m = NULL;
    return m;
}

static gboolean
plugin_load(PurplePlugin *plugin, GError **error)
{
    return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin, GError **error)
{
    purple_signals_disconnect_by_handle(plugin);
    purplesignal_destroy(signaljvm);
    return TRUE;
}

static gboolean
libpurple2_plugin_load(PurplePlugin *plugin)
{
    return plugin_load(plugin, NULL);
}

static gboolean
libpurple2_plugin_unload(PurplePlugin *plugin)
{
    return plugin_unload(plugin, NULL);
}

static void
plugin_init(PurplePlugin *plugin)
{
    PurplePluginInfo *info;
    PurplePluginProtocolInfo *prpl_info = g_new0(PurplePluginProtocolInfo, 1); // TODO: this leaks

    info = plugin->info;

    if (info == NULL) {
        plugin->info = info = g_new0(PurplePluginInfo, 1);
    }

    info->name = "Signal (Java)";
    info->extra_info = prpl_info;

    prpl_info->options = OPT_PROTO_NO_PASSWORD;
    prpl_info->protocol_options = signal_add_account_options(prpl_info->protocol_options);
    prpl_info->list_icon = signal_list_icon;
    prpl_info->status_types = signal_status_types; // this actually needs to exist, else the protocol cannot be set to "online"
    prpl_info->login = signal_login;
    prpl_info->close = signal_close;
    prpl_info->send_im = signal_send_im;
    prpl_info->add_buddy = signal_add_buddy;
}

static PurplePluginInfo info = {
    PURPLE_PLUGIN_MAGIC,
    2, 1,
    PURPLE_PLUGIN_PROTOCOL,            /* type */
    NULL,                            /* ui_requirement */
    0,                                /* flags */
    NULL,                            /* dependencies */
    PURPLE_PRIORITY_DEFAULT,        /* priority */
    SIGNAL_PLUGIN_ID,                /* id */
    (char*)"signal",                        /* name */
    SIGNAL_PLUGIN_VERSION,            /* version */
    (char*)"",                                /* summary */
    (char*)"",                                /* description */
    (char*)"Hermann Hoehne <hoehermann@gmx.de>", /* author */
    SIGNAL_PLUGIN_WEBSITE,            /* homepage */
    libpurple2_plugin_load,            /* load */
    libpurple2_plugin_unload,        /* unload */
    NULL,                            /* destroy */
    NULL,                            /* ui_info */
    NULL,                            /* extra_info */
    NULL,                            /* prefs_info */
    signal_actions,                /* actions */
    NULL,                            /* padding */
    NULL,
    NULL,
    NULL
};

PURPLE_INIT_PLUGIN(signal, plugin_init, info);
}

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
            "signal", "Not handling message for not-existant connection %p.\n", pc
        );
    } else {
        (*psm->function)(pc);
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
