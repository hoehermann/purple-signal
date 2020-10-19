#include <purple.h>
#include <memory>
#include <functional>

typedef std::function<void(PurpleConnection *pc)> PurpleSignalConnectionFunction;

typedef struct {
    uintptr_t pc;
    std::unique_ptr<PurpleSignalConnectionFunction> function;
} PurpleSignalMessage;

void signal_process_error(PurpleConnection *pc, const PurpleDebugLevel level, const std::string & message);
void signal_process_message(PurpleConnection *pc, const std::string & chat, const std::string & sender, const std::string & message, const long timestamp, const PurpleMessageFlags flags);
void signal_ask_register_or_link(PurpleConnection *pc);
