#pragma once

#include <purple.h>

void signal_process_message(PurpleConnection *pc, const std::string & chat, const std::string & sender, const std::string & message, time_t timestamp, const PurpleMessageFlags flags);
