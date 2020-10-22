#include <sstream>
#include "QR-Code-generator/cpp/QrCode.hpp"

#include "libsignal-account.h"
#include "libsignal.h"


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
