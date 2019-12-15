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
#perror Purple3 not supported.
#endif

#include "libsignal-jni.h"

#define SIGNAL_PLUGIN_ID "prpl-hehoe-signal"
#ifndef SIGNAL_PLUGIN_VERSION
#error Must set SIGNAL_PLUGIN_VERSION in Makefile
#endif
#define SIGNAL_PLUGIN_WEBSITE "https://github.com/hoehermann/TODO"

#define SIGNAL_STATUS_STR_ONLINE   "online"
#define SIGNAL_STATUS_STR_OFFLINE  "offline"
#define SIGNAL_STATUS_STR_MOBILE   "mobile"

/*
 * Holds all information related to this account (connection) instance.
 */
typedef struct {
    PurpleAccount *account;
    PurpleConnection *pc;
    PurpleSignal ps;
    GList *used_images; // for inline images
} SignalAccount;

SignalJVM signaljvm; // only one Java VM over all connections

static const char *
signal_list_icon(PurpleAccount *account, PurpleBuddy *buddy)
{
    return "signal";
}

PurpleConversation *signal_find_conversation(char *username, PurpleAccount *account) {
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
signal_display_message(PurpleConnection *pc)
{
    //SignalAccount *sa = purple_connection_get_protocol_data(pc);
    PurpleMessageFlags flags = 0;
    char * content = NULL;
    if (0 /*fromMe*/) {
        // special handling of messages sent by self incoming from remote, addressing issue #32
        // copied from EionRobb/purple-discord/blob/master/libdiscord.c
        flags |= PURPLE_MESSAGE_SEND | PURPLE_MESSAGE_REMOTE_SEND | PURPLE_MESSAGE_DELAYED;
        //PurpleConversation *conv = signal_find_conversation(gwamsg->remoteJid, sa->account);
        //purple_conversation_write(conv, gwamsg->remoteJid, content, flags, gwamsg->timestamp);
    } else {
        flags |= PURPLE_MESSAGE_RECV;
        //purple_serv_got_im(pc, gwamsg->remoteJid, content, flags, gwamsg->timestamp);
    }
    g_free(content);
}

/*
 * Interprets a message received from go-whatsapp. Handles login sucess and failures. Forwards errors.
 */
void
signal_process_message(PurpleConnection *pc)
{
    //SignalAccount *sa = purple_connection_get_protocol_data(pc);
    //PurpleAccount *account = purple_connection_get_account(pc);
    
    purple_debug_info(
        "signal", "Recieved message.\n"
    );
    //if (!gwamsg->timestamp) {
    //    gwamsg->timestamp = time(NULL);
    //}
    signal_display_message(pc);
}

void
signal_login(PurpleAccount *account)
{
    PurpleConnection *pc = purple_account_get_connection(account);

    // this protocol does not support anything special right now
    PurpleConnectionFlags pc_flags;
    pc_flags = purple_connection_get_flags(pc);
    pc_flags |= PURPLE_CONNECTION_NO_IMAGES;
    pc_flags |= PURPLE_CONNECTION_NO_FONTSIZE;
    pc_flags |= PURPLE_CONNECTION_NO_NEWLINES; // TODO: find out how signal represents newlines, use them
    pc_flags |= PURPLE_CONNECTION_NO_BGCOLOR;
    purple_connection_set_flags(pc, pc_flags);

    SignalAccount *sa = g_new0(SignalAccount, 1);
    purple_connection_set_protocol_data(pc, sa);
    sa->account = account;
    sa->pc = pc;

    purplesignal_login(signaljvm, &sa->ps, purple_account_get_username(account));
}

static void
signal_close(PurpleConnection *pc)
{
    SignalAccount *sa = purple_connection_get_protocol_data(pc);
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
    return 0;
}

static void
signal_add_buddy(PurpleConnection *pc, PurpleBuddy *buddy, PurpleGroup *group)
{
    // does not actually do anything. buddy is added to pidgin's local list and is usable from there.
    //SignalAccount *sa = purple_connection_get_protocol_data(pc);
}

static GList *
signal_add_account_options(GList *account_options)
{
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
    gboolean jvm_ok = purplesignal_init(&signaljvm);
    if (jvm_ok) {
        purple_debug_info(
            "signal", "JVM seems to have been initalized!\n"
        );
    }
    return jvm_ok;
}

static gboolean
plugin_unload(PurplePlugin *plugin, GError **error)
{
    purple_signals_disconnect_by_handle(plugin);
    purplesignal_deinit(&signaljvm);
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
    "signal",                        /* name */
    SIGNAL_PLUGIN_VERSION,            /* version */
    "",                                /* summary */
    "",                                /* description */
    "Hermann Hoehne <hoehermann@gmx.de>", /* author */
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

PURPLE_INIT_PLUGIN(gowhatsapp, plugin_init, info);
