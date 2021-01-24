#include "contact.hpp"
#include "../buddies.hpp" // for assume_buddy_online
// TODO: merge unit contact with buddies?

const char * const SIGNAL_DEFAULT_GROUP = "Signal";

/*
 * Processes contact information provided by Signal.
 * Adapted from https://github.com/hoehermann/libpurple-signald/blob/master/contacts.c
 * Original by Brett Kosinski.
 */
void signal_process_contact(PurpleConnection *pc, const std::string & number, const std::string & alias) {
    PurpleAccount * account = purple_connection_get_account(pc);
    GSList * buddies = purple_find_buddies(account, number.c_str());
    if (buddies) {
        // contact already in buddy list
        g_slist_free(buddies);
    } else {
        // contact not in buddy list
        PurpleGroup * g = purple_find_group(SIGNAL_DEFAULT_GROUP);
        if (!g) {
            g = purple_group_new(SIGNAL_DEFAULT_GROUP);
            purple_blist_add_group(g, NULL);
        }
        PurpleBuddy * b = purple_buddy_new(account, number.c_str(), NULL);
        purple_blist_add_buddy(b, NULL, g, NULL);
        signal_assume_buddy_online(account, b);
    }
    if (!alias.empty()) {
        serv_got_alias(pc, number.c_str(), alias.c_str());
    }
}
