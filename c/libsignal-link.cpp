#include "libsignal-link.h"
#include "libsignal.h"

void signal_show_qr_code(PurpleConnection *pc, const std::string & qr_raw_data, const std::string & qr_code_ppm) {
    SignalAccount *sa = static_cast<SignalAccount *>(purple_connection_get_protocol_data(pc));

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
        sa->pc, /*handle*/
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

void signal_generate_and_show_qr_code(PurpleConnection *pc, const std::string & device_link_uri) {
    signal_show_qr_code(pc, device_link_uri, "");
}
