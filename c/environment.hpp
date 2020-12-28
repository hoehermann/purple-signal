#pragma once

#include "submodules/typedjni/typedjni.hpp"
#include <string>

class PurpleSignalEnvironment {
    private:
    static TypedJNIEnv *jvm; // static member – only one Java VM over all connections
    public:
    static TypedJNIEnv * get(const std::string & signal_cli_path);
    static void destroy();
};
