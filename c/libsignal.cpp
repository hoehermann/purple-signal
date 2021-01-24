 /*
  * This file contains all exported C functions as required by libpurple.
  * 
  * All exceptions should be caught in these functions.
  * Exception: Exceptions thrown by asynchronous events are handled in async.cpp.
  */

#include "libsignal.hpp"
#include "environment.hpp"
#include "connection.hpp"
#include "buddies.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __GNUC__
#include <unistd.h>
#endif

#include "purple_compat.h"
#if PURPLE_VERSION_CHECK(3, 0, 0)
#error Purple3 not supported.
#endif

const char * const SIGNAL_PLUGIN_ID  = "prpl-hehoe-signal";
const char * const SIGNAL_PLUGIN_WEBSITE = "https://github.com/hoehermann/purple-signal";

const char * const SIGNAL_OPTION_LIBDIR = "signal-cli-lib-dir";
const char * const SIGNAL_DEFAULT_LIBDIR = "";
const char * const SIGNAL_OPTION_SHOW_SYSTEM = "show-system-messages";
const bool SIGNAL_DEFAULT_SHOW_SYSTEM = true;
                
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

    const std::string libdir(purple_account_get_string(account, SIGNAL_OPTION_LIBDIR, SIGNAL_DEFAULT_LIBDIR));
    if (libdir == "") {
        purple_connection_error(
            pc, 
            PURPLE_CONNECTION_ERROR_OTHER_ERROR, 
            "Path to signal-cli's lib directory is empty. Set it appropriately (e.g. /opt/signal-cli/lib)."
        );
        return;
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

    try {
        purple_connection_set_state(pc, PURPLE_CONNECTION_CONNECTING);
        PurpleSignalConnection *sa = new PurpleSignalConnection(account, pc, libdir, purple_account_get_username(account));
        purple_connection_set_protocol_data(pc, sa);
        purple_connection_set_state(pc, PURPLE_CONNECTION_CONNECTED);
        signal_assume_all_buddies_online(account);
    } catch (std::exception & e) {
        purple_connection_error(pc, PURPLE_CONNECTION_ERROR_OTHER_ERROR, e.what());
    }
}

static void
signal_close(PurpleConnection *pc)
{
    // TODO: disconnect all handles (important for xfers of dying connections)
    PurpleSignalConnection *sa = (PurpleSignalConnection *)purple_connection_get_protocol_data(pc);
    if (sa) {
        delete sa;
    }
}

static GList *
signal_status_types(PurpleAccount *account)
{
    GList *types = NULL;
    PurpleStatusType *status;

    status = purple_status_type_new_full(PURPLE_STATUS_AVAILABLE, STATUS_STR_ONLINE, "Online", TRUE, TRUE, FALSE);
    types = g_list_append(types, status);

    status = purple_status_type_new_full(PURPLE_STATUS_OFFLINE, STATUS_STR_OFFLINE, "Offline", TRUE, TRUE, FALSE);
    types = g_list_append(types, status);

    status = purple_status_type_new_full(PURPLE_STATUS_MOBILE, STATUS_STR_MOBILE, NULL, FALSE, FALSE, TRUE);
    types = g_list_prepend(types, status);

    return types;
}

static int
signal_send_im(PurpleConnection *pc, const gchar *who, const gchar *message, PurpleMessageFlags flags)
{
    PurpleSignalConnection *sa = (PurpleSignalConnection *)purple_connection_get_protocol_data(pc);
    char *m = purple_markup_strip_html(message); // related: https://github.com/majn/telegram-purple/issues/12 and https://github.com/majn/telegram-purple/commit/fffe7519d7269cf4e5029a65086897c77f5283ac
    try {
        return sa->ps.send_im(who, m);
    } catch (std::exception & e) {
        purple_connection_error(pc, PURPLE_CONNECTION_ERROR_OTHER_ERROR, e.what());
        return -1;
    }
}

static void
signal_add_buddy(PurpleConnection *pc, PurpleBuddy *buddy, PurpleGroup *group)
{
    // does not actually do anything. buddy is added to pidgin's local list and is usable from there.
    signal_assume_buddy_online(pc->account, buddy);
}

static GList *
signal_add_account_options(GList *account_options)
{
    PurpleAccountOption *option;
    
    option = purple_account_option_string_new(
                "signal-cli's lib directory (containing .jar files)",
                SIGNAL_OPTION_LIBDIR,
                SIGNAL_DEFAULT_LIBDIR
                );
    account_options = g_list_append(account_options, option);
    
    option = purple_account_option_string_new(
                "Name to show in profile (optional for linked accounts)",
                "profile-name",
                ""
                );
    account_options = g_list_append(account_options, option);
    
    option = purple_account_option_bool_new(
                "Show system messages in conversation for debug purposes.",
                SIGNAL_OPTION_SHOW_SYSTEM,
                SIGNAL_DEFAULT_SHOW_SYSTEM
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
    // TODO: move this out of here
    PurpleSignalEnvironment::destroy();
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
    const_cast<char *>(SIGNAL_PLUGIN_ID),                /* id */
    (char*)"signal",                        /* name */
    SIGNAL_PLUGIN_VERSION,            /* version */
    (char*)"",                                /* summary */
    (char*)"",                                /* description */
    (char*)"Hermann Hoehne <hoehermann@gmx.de>", /* author */
    const_cast<char *>(SIGNAL_PLUGIN_WEBSITE),            /* homepage */
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
