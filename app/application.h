#ifndef _APPLICATION_H
#define _APPLICATION_H

#ifndef VERSION
#define VERSION "vdev"
#endif

#include <bcl.h>

void qrcode_project(char *project_name);
void get_qr_data();
char get_orderId();
void bc_change_qr_value(uint64_t *id, const char *topic, void *value, void *param);

typedef enum
{
    PASSWD = 0,
    SSID = 1,
    ENCRYPTION = 2
}bc_wifi_command_t;



#endif // _APPLICATION_H
