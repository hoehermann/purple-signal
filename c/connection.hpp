#pragma once

#include <purple.h>
#include "purplesignal/purplesignal.hpp"

/*
 * Holds all information related to this connection instance.
 * 
 * Methods defer their work to the appropriate implementation (C → Java).
 */
class PurpleSignalConnection {
    public:
    PurpleAccount *account;
    PurpleConnection *connection;
    PurpleSignal ps;
    
    PurpleSignalConnection(
        PurpleAccount *account, 
        PurpleConnection *pc, 
        const std::string & signal_lib_directory, const std::string & settings_directory, const std::string & username
    );
    virtual ~PurpleSignalConnection();
};
