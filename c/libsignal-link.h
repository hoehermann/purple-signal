#pragma once

#include <purple.h>
#include <string>

void signal_generate_and_show_qr_code(PurpleConnection *pc, const std::string & device_link_uri);
