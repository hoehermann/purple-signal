/*
 * Implementation of handlers for incoming attachments (Java → C).
 * This is all to be executed on the main purple thread.
 */

#include <iostream>
#include "attachment.hpp"
#include "../connection.hpp"

class PurpleSignalXferData {
    public:
    PurpleSignalConnection *psc;
    const std::shared_ptr<_jobject> attachment;
    const std::string sender;
    std::function<int(guchar **buffer)> read_fnc;
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
        purple_xfer_start(xfer, -1, NULL, 0); // invokes start_fnc
    });
    
    purple_xfer_set_start_fnc(xfer, [](PurpleXfer * xfer){
        const std::string local_file_name = purple_xfer_get_local_filename(xfer);
        PurpleSignalXferData * psxd = reinterpret_cast<PurpleSignalXferData *>(xfer->data);
        try {
            psxd->read_fnc = psxd->psc->ps.acceptAttachment(psxd->attachment.get(), local_file_name);
            purple_xfer_prpl_ready(xfer); // invokes do_transfer which invokes read_fnc
        } catch (std::exception & e) {
            purple_xfer_error(purple_xfer_get_type(xfer), purple_xfer_get_account(xfer), purple_xfer_get_remote_user(xfer), e.what());
            purple_xfer_cancel_local(xfer);
        }
    });

    purple_xfer_set_read_fnc(xfer, [](guchar **buffer, PurpleXfer * xfer) -> gssize {
        PurpleSignalXferData * psxd = reinterpret_cast<PurpleSignalXferData *>(xfer->data);
        return psxd->read_fnc(buffer);
    });
    
    purple_xfer_set_ack_fnc(xfer, [](PurpleXfer * xfer, const guchar *, size_t size){
        if (size > 0 && xfer->data != nullptr) { // TODO: better check this in callback?
            // this feels odd, but somehow I need to append the ready signal to the event queue
            purple_timeout_add(
                0, 
                reinterpret_cast<GSourceFunc>(purple_xfer_prpl_ready), // G_SOURCE_FUNC(…) is preferred, but not available on win32
                (void*)xfer
            );
        }
    });
    
    purple_xfer_set_end_fnc(xfer, [](PurpleXfer *xfer){
        std::cerr << "purple_xfer_set_end_fnc callback" << std::endl;
        if (xfer->data != nullptr) {
            delete reinterpret_cast<PurpleSignalXferData *>(xfer->data);
            xfer->data = nullptr;
        }
    });
    purple_xfer_set_request_denied_fnc(xfer, [](PurpleXfer *xfer){
        std::cerr << "purple_xfer_set_request_denied_fnc callback" << std::endl;
        if (xfer->data != nullptr) {
            delete reinterpret_cast<PurpleSignalXferData *>(xfer->data);
            xfer->data = nullptr;
        }
    });
    purple_xfer_set_cancel_recv_fnc(xfer, [](PurpleXfer *xfer){
        std::cerr << "purple_xfer_set_cancel_recv_fnc callback" << std::endl;
        // this is called after user cancels dialogue as well as after purple_xfer_cancel_local
        if (xfer->data != nullptr) {
            delete reinterpret_cast<PurpleSignalXferData *>(xfer->data);
            xfer->data = nullptr;
        }
    });
    
    purple_xfer_request(xfer);
}
