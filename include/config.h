// Copyright (c) 2026
// Simple INI-style config loader for the softphone.
#ifndef SOFTPHONE_CONFIG_H
#define SOFTPHONE_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_MAX_STR 256

typedef struct softphone_config {
    char sip_domain[CONFIG_MAX_STR];
    char sip_user[CONFIG_MAX_STR];
    char sip_password[CONFIG_MAX_STR];
    char sip_proxy[CONFIG_MAX_STR];
    char outbound_proxy[CONFIG_MAX_STR];
    char transport[16];
    char local_bind[64];
    int local_port;
    char display_name[CONFIG_MAX_STR];
    char url_template[CONFIG_MAX_STR];
    char record_dir[CONFIG_MAX_STR];
    int auto_answer;
    int log_level;
    int console_level;
    char codec_order[CONFIG_MAX_STR];
} softphone_config;

void config_set_defaults(softphone_config *cfg);
int config_load(const char *path, softphone_config *cfg);

#ifdef __cplusplus
}
#endif

#endif
