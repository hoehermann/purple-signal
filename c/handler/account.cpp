/*
 * Implementation of handlers for account management (Java → C → Java).
 * This is all to be executed on the main purple thread.
 */

#include <sstream>
#include "../purple_compat.h"
#include "../submodules/qrcode/cpp/QrCode.hpp"
#include "../connection.hpp"
#include "account.hpp"

void signal_show_qr_code(PurpleConnection *pc, const std::string & qr_code_ppm, const std::string & qr_raw_data) {
    PurpleSignalConnection *sa = static_cast<PurpleSignalConnection *>(purple_connection_get_protocol_data(pc));

    PurpleRequestFields *fields = purple_request_fields_new();
    PurpleRequestFieldGroup *group = purple_request_field_group_new(NULL);
    purple_request_fields_add_group(fields, group);

    PurpleRequestField *string_field = purple_request_field_string_new("qr_string", "QR Code Data", g_strdup(qr_raw_data.c_str()), FALSE);
    purple_request_field_group_add_field(group, string_field);
    PurpleRequestField *image_field = purple_request_field_image_new("qr_image", "QR Code Image", g_strdup(qr_code_ppm.c_str()), qr_code_ppm.size());
    purple_request_field_group_add_field(group, image_field);

    const char *username = g_strdup(purple_account_get_username(sa->account));
    const char *secondary = g_strdup_printf("Signal account %s", username);

    purple_request_fields(
        sa->connection, /*handle*/
        "Logon QR Code", /*title*/
        "Please scan this QR code with your phone", /*primary*/
        secondary, /*secondary*/
        fields, /*fields*/
        "OK", [](){}, /*OK*/
        "Dismiss", [](){}, /*Cancel*/
        sa->account, /*account*/
        username, /*username*/
        NULL, /*conversation*/
        NULL /*data*/
    );
}

std::string signal_generate_qr_code(const std::string & device_link_uri, int zoom_factor) {
    qrcodegen::QrCode qr1 = qrcodegen::QrCode::encodeText(device_link_uri.c_str(), qrcodegen::QrCode::Ecc::MEDIUM);
    std::ostringstream oss;
    oss << "P1\n" << qr1.getSize()*zoom_factor << " " << qr1.getSize()*zoom_factor << "\n";
    for (int y = 0; y < qr1.getSize(); y++) {
        for (int yz = 0; yz < zoom_factor; yz++) {
            for (int x = 0; x < qr1.getSize(); x++) {
                for (int xz = 0; xz < zoom_factor; xz++) {
                    oss << qr1.getModule(x, y) << " ";
                }
            }
            oss << "\n";
        }
    }
    return oss.str();
}

void
signal_ask_verification_code_ok_cb(PurpleConnection *pc, const char *code) {
    PurpleSignalConnection *sa = static_cast<PurpleSignalConnection *>(purple_connection_get_protocol_data(pc));
    sa->ps.verify_account(code, "");
}

void
signal_ask_verification_code_cancel_cb(PurpleConnection *pc, const char *code) {
    purple_connection_error(pc, PURPLE_CONNECTION_ERROR_INVALID_USERNAME, "Cannot continue without verification.");
}

void
signal_ask_verification_code(PurpleConnection *pc) {
    PurpleSignalConnection *sa = static_cast<PurpleSignalConnection *>(purple_connection_get_protocol_data(pc));
    purple_request_input(
        pc /*handle*/, 
        "Verification" /*title*/, 
        "Please enter verification code" /*primary*/,
        NULL /*secondary*/, 
        NULL /*default_value*/, 
        false /*multiline*/,
        false /*masked*/, 
        NULL /*hint*/,
        "OK" /*ok_text*/, 
        G_CALLBACK(signal_ask_verification_code_ok_cb),
        "Cancel" /*cancel_text*/, 
        G_CALLBACK(signal_ask_verification_code_cancel_cb),
        sa->account /*account*/, 
        NULL /*who*/, 
        NULL /*conv*/,
        pc /*user_data*/
    );
}

const int SIGNAL_ACCOUNT_LINK = 0;
const int SIGNAL_ACCOUNT_REGISTER = 1;
const int SIGNAL_ACCOUNT_VERIFY = 2;

void
signal_ask_register_or_link_ok_cb(PurpleConnection *pc, int choice) {
    PurpleSignalConnection *sa = static_cast<PurpleSignalConnection *>(purple_connection_get_protocol_data(pc));
    try {
        switch (choice) {
            case SIGNAL_ACCOUNT_LINK: sa->ps.link_account(); break;
            case SIGNAL_ACCOUNT_REGISTER: sa->ps.register_account(false, ""); break;
            case SIGNAL_ACCOUNT_VERIFY: signal_ask_verification_code(pc); break;
            default: purple_debug(PURPLE_DEBUG_ERROR, "signal", "%s\n", "User dialogue returned with invalid choice.");
        }
    } catch (std::exception & e) {
        purple_connection_error(pc, PURPLE_CONNECTION_ERROR_OTHER_ERROR,e.what());
    }   
}

void
signal_ask_register_or_link_cancel_cb(PurpleConnection *pc, int choice) {
    purple_connection_error(pc, PURPLE_CONNECTION_ERROR_INVALID_USERNAME, "Cannot continue without account.");
}

void
signal_ask_register_or_link(PurpleConnection *pc) {
    PurpleSignalConnection *sa = static_cast<PurpleSignalConnection *>(purple_connection_get_protocol_data(pc));
    // TODO: offer input for captcha
    purple_request_choice(
        pc, 
        "Unknown account", "Please choose",
        NULL, 0,
        "OK", G_CALLBACK(signal_ask_register_or_link_ok_cb),
        "Cancel", G_CALLBACK(signal_ask_register_or_link_cancel_cb),
        sa->account, NULL, NULL, 
        pc, 
        "Link to existing account", SIGNAL_ACCOUNT_LINK, 
        "Register new", SIGNAL_ACCOUNT_REGISTER, 
        NULL
    );
}
