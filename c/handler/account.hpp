#pragma once

#include <string>
#include <purple.h>

std::string signal_generate_qr_code(const std::string & device_link_uri, int zoom_factor);
void signal_show_qr_code(PurpleConnection *pc, const std::string & qr_code_ppm, const std::string & qr_raw_data);

void signal_ask_register_or_link(PurpleConnection *pc);
void signal_ask_verification_code(PurpleConnection *pc);