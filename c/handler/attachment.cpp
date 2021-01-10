/*
 * Implementation of handlers for incoming attachments (Java → C).
 * This is all to be executed on the main purple thread.
 */

#include <iostream>
#include "attachment.hpp"
#include "../connection.hpp"

bool signal_purple_xfer_exists(PurpleXfer * xfer) {
    int xfer_exists = false;
    for (GList *iter = purple_xfers_get_all(); iter != nullptr && xfer_exists == false; iter = iter->next) {
        const PurpleXfer *transfer = (PurpleXfer *)iter->data;
        if (transfer == xfer) {
            xfer_exists = true;
        };
    }
    return xfer_exists;
}

class PurpleSignalXferData {
    public:
    PurpleSignalConnection *psc;
    const std::shared_ptr<_jobject> attachment;
    const std::string sender;
    PurpleSignalXferData(
        PurpleSignalConnection *psc, 
        const std::shared_ptr<_jobject> attachment, 
        const std::string sender
    ) : psc(psc), attachment(attachment), sender(sender) {}
};

void
signal_process_attachment(
    PurpleConnection *pc, 
    const std::string & chat, 
    const std::string & sender, 
    const std::shared_ptr<_jobject> & attachment, 
    const std::string & file_name, 
    const int file_size
) {
    std::cerr << "signal_process_attachment:" << std::endl;
    std::cerr << "attachment " << attachment.get() << std::endl;
    PurpleSignalConnection *psc = static_cast<PurpleSignalConnection *>(purple_connection_get_protocol_data(pc));
    PurpleXfer * xfer = purple_xfer_new(psc->account, PURPLE_XFER_RECEIVE, sender.c_str());
    xfer->data = new PurpleSignalXferData(psc, attachment, sender); // I would really like to use a lambda capture instead of this cludge
    purple_xfer_ref(xfer);
    purple_xfer_set_size(xfer, file_size);
    purple_xfer_set_filename(xfer, file_name.c_str());
    std::cerr << "xfer is " << xfer << std::endl;
    
    purple_xfer_set_init_fnc(xfer, [](PurpleXfer *xfer){
        std::cerr << "purple_xfer_set_init_fnc callback!" << std::endl;
        std::cerr << "xfer is " << xfer << std::endl;
        purple_xfer_start(xfer, -1, NULL, 0);
        const std::string local_file_name = purple_xfer_get_local_filename(xfer);
        //const int file_size = purple_xfer_get_size(xfer);
        PurpleSignalXferData * psxd = reinterpret_cast<PurpleSignalXferData *>(xfer->data);
        try {
            psxd->psc->ps.receiveAttachment(psxd->attachment.get(), local_file_name, reinterpret_cast<uintptr_t>(xfer));
        } catch (std::exception & e) {
            purple_xfer_error(purple_xfer_get_type(xfer), purple_xfer_get_account(xfer), purple_xfer_get_remote_user(xfer), e.what());
            purple_xfer_cancel_local(xfer);
        }
    });
    
    purple_xfer_set_end_fnc(xfer, [](PurpleXfer *xfer){
        std::cerr << "purple_xfer_set_end_fnc callback" << std::endl;
        if (signal_purple_xfer_exists(xfer)) {
            delete reinterpret_cast<PurpleSignalXferData *>(xfer->data);
        }
    });
    purple_xfer_set_request_denied_fnc(xfer, [](PurpleXfer *xfer){
        std::cerr << "purple_xfer_set_request_denied_fnc callback" << std::endl;
        if (signal_purple_xfer_exists(xfer)) {
            delete reinterpret_cast<PurpleSignalXferData *>(xfer->data);
        }
    });
    purple_xfer_set_cancel_recv_fnc(xfer, [](PurpleXfer *xfer){
        std::cerr << "purple_xfer_set_cancel_recv_fnc callback" << std::endl;
        // this is called after purple_xfer_cancel_local
        // TODO: this should stop the backgroundReceiver in Java, too
        if (signal_purple_xfer_exists(xfer)) {
            delete reinterpret_cast<PurpleSignalXferData *>(xfer->data);
        }
    });
    
    purple_xfer_request(xfer);
}
