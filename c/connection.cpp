/*
 * Implementation of libpurple connection data management.
 * 
 * Upon construction, a PurpleSignal instance is created.
 * If needed, PurpleSignalEnvironment::get creates a JVM.
 * Most work is defered to the PurpleSignal (Java) object.
 */

#include "connection.hpp"
#include "environment.hpp"

PurpleSignalConnection::PurpleSignalConnection(
    const std::string & settings_dir, 
    PurpleAccount *account,
    PurpleConnection *pc, 
    const std::string & signal_lib_directory, 
    const std::string & username
) :
    account(account),
    connection(pc),
    ps(PurpleSignalEnvironment::get(signal_lib_directory), settings_dir, username) {
};
    
PurpleSignalConnection::~PurpleSignalConnection() {
    ps.close();
}
