#include <purple.h>
#include <memory>
#include <functional>

typedef std::function<void(PurpleConnection *pc)> PurpleSignalConnectionFunction;

typedef struct {
    uintptr_t pc;
    const char *chat;
    const char *sender;
    const char *message;
    /*signed*/ long timestamp;
    PurpleMessageFlags flags;
    PurpleDebugLevel error;
    std::unique_ptr<PurpleSignalConnectionFunction> function;
} PurpleSignalMessage;

void signal_process_message(PurpleConnection *pc, PurpleSignalMessage *psm);
