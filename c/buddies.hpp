#pragma once

#include <purple.h>

#define STATUS_STR_ONLINE  "online"
#define STATUS_STR_OFFLINE "offline"
#define STATUS_STR_MOBILE  "mobile"

void signal_assume_buddy_online(PurpleAccount *account, PurpleBuddy *buddy);
void signal_assume_all_buddies_online(PurpleAccount *account);
