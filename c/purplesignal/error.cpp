#include "error.hpp"

PurpleSignalError::PurpleSignalError(const std::string& what_arg, bool is_fatal) : std::runtime_error(what_arg), is_fatal(is_fatal) {}
