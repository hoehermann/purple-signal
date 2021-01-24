#pragma once

#include <string>
#include <purple.h>

extern const char * const SIGNAL_DEFAULT_GROUP;

void signal_process_contact(PurpleConnection *pc, const std::string & who, const std::string & alias);
