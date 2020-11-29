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
    PurpleAccount *account, 
    PurpleConnection *pc, 
    const std::string & signal_lib_directory, const std::string & settings_directory, const std::string & username
) :
    account(account), 
    connection(pc),
    ps(uintptr_t(pc), uintptr_t(account), PurpleSignalEnvironment::get(signal_lib_directory), settings_directory, username) {
};
    
PurpleSignalConnection::~PurpleSignalConnection() {
    ps.close();
}