#pragma once

#include <memory>
#include "../purplesignal/purplesignal.hpp"
#include "../purple_compat.h"

void signal_process_attachment(
    PurpleConnection *pc, 
    const std::string & chat, 
    const std::string & sender, 
    const std::shared_ptr<_jobject> & attachment, 
    const std::string & file_name, 
    const int file_size
);
