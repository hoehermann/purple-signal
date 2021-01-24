#include "buddies.hpp"

void signal_assume_buddy_online(PurpleAccount *account, PurpleBuddy *buddy) {
    purple_prpl_got_user_status(account, buddy->name, STATUS_STR_ONLINE, NULL);
    purple_prpl_got_user_status(account, buddy->name, STATUS_STR_MOBILE, NULL);
}

void signal_assume_all_buddies_online(PurpleAccount *account) {
    GSList *buddies = purple_find_buddies(account, NULL);
    while (buddies != NULL) {
        signal_assume_buddy_online(account, static_cast<PurpleBuddy *>(buddies->data));
        buddies = g_slist_delete_link(buddies, buddies);
    }
}
