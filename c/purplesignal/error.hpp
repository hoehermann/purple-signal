#pragma once

#include <stdexcept>

class PurpleSignalError : public std::runtime_error
{
    public:
    const bool is_fatal;
    PurpleSignalError(const std::string& what_arg, bool is_fatal);
};
