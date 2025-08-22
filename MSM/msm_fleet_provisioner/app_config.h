#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stddef.h>

typedef struct 
{
    char endpoint[128];
    int endpoint_len;     // added length field
    int port;
    char root_ca[256];
    char claim_cert[256];
    char claim_key[256];
    char template_name[64];
    char serial[64];
    int serial_len
}app_config_t;

// Declare gConfig as extern so other files can use it
extern app_config_t gConfig;

#endif // APP_CONFIG_H
