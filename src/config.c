// Copyright (c) 2026
#include "config.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void trim_whitespace(char *text) {
    char *start = text;
    char *end = NULL;
    size_t len;

    while (*start && isspace((unsigned char)*start)) {
        start++;
    }

    if (start != text) {
        memmove(text, start, strlen(start) + 1);
    }

    len = strlen(text);
    if (len == 0) {
        return;
    }

    end = text + len - 1;
    while (end >= text && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
}

static int is_comment_or_empty(const char *text) {
    if (text == NULL || *text == '\0') {
        return 1;
    }
    if (*text == '#' || *text == ';') {
        return 1;
    }
    return 0;
}

static void safe_copy(char *dst, size_t dst_size, const char *src) {
    if (dst_size == 0) {
        return;
    }
    if (src == NULL) {
        dst[0] = '\0';
        return;
    }
    strncpy(dst, src, dst_size - 1);
    dst[dst_size - 1] = '\0';
}

void config_set_defaults(softphone_config *cfg) {
    if (!cfg) {
        return;
    }
    memset(cfg, 0, sizeof(*cfg));
    safe_copy(cfg->transport, sizeof(cfg->transport), "udp");
    safe_copy(cfg->display_name, sizeof(cfg->display_name), "Softphone");
    safe_copy(cfg->url_template, sizeof(cfg->url_template),
              "https://example.local/incoming?cname={CNAME}&caller={CALLER}");
    safe_copy(cfg->record_dir, sizeof(cfg->record_dir), "recordings");
    safe_copy(cfg->codec_order, sizeof(cfg->codec_order), "G729,PCMU,PCMA");
    cfg->local_port = 5060;
    cfg->auto_answer = 0;
    cfg->log_level = 4;
    cfg->console_level = 4;
}

static void apply_kv(softphone_config *cfg, const char *key, const char *value) {
    if (strcmp(key, "sip_domain") == 0) {
        safe_copy(cfg->sip_domain, sizeof(cfg->sip_domain), value);
    } else if (strcmp(key, "sip_user") == 0) {
        safe_copy(cfg->sip_user, sizeof(cfg->sip_user), value);
    } else if (strcmp(key, "sip_password") == 0) {
        safe_copy(cfg->sip_password, sizeof(cfg->sip_password), value);
    } else if (strcmp(key, "sip_proxy") == 0) {
        safe_copy(cfg->sip_proxy, sizeof(cfg->sip_proxy), value);
    } else if (strcmp(key, "outbound_proxy") == 0) {
        safe_copy(cfg->outbound_proxy, sizeof(cfg->outbound_proxy), value);
    } else if (strcmp(key, "transport") == 0) {
        safe_copy(cfg->transport, sizeof(cfg->transport), value);
    } else if (strcmp(key, "local_bind") == 0) {
        safe_copy(cfg->local_bind, sizeof(cfg->local_bind), value);
    } else if (strcmp(key, "local_port") == 0) {
        cfg->local_port = atoi(value);
    } else if (strcmp(key, "display_name") == 0) {
        safe_copy(cfg->display_name, sizeof(cfg->display_name), value);
    } else if (strcmp(key, "url_template") == 0) {
        safe_copy(cfg->url_template, sizeof(cfg->url_template), value);
    } else if (strcmp(key, "record_dir") == 0) {
        safe_copy(cfg->record_dir, sizeof(cfg->record_dir), value);
    } else if (strcmp(key, "auto_answer") == 0) {
        cfg->auto_answer = atoi(value) ? 1 : 0;
    } else if (strcmp(key, "log_level") == 0) {
        cfg->log_level = atoi(value);
    } else if (strcmp(key, "console_level") == 0) {
        cfg->console_level = atoi(value);
    } else if (strcmp(key, "codec_order") == 0) {
        safe_copy(cfg->codec_order, sizeof(cfg->codec_order), value);
    }
}

int config_load(const char *path, softphone_config *cfg) {
    char line[512];
    FILE *fp;

    if (!cfg || !path) {
        return -1;
    }

    config_set_defaults(cfg);

    fp = fopen(path, "r");
    if (!fp) {
        return -1;
    }

    while (fgets(line, sizeof(line), fp)) {
        char *equals = NULL;
        trim_whitespace(line);
        if (is_comment_or_empty(line)) {
            continue;
        }
        equals = strchr(line, '=');
        if (!equals) {
            continue;
        }
        *equals = '\0';
        trim_whitespace(line);
        trim_whitespace(equals + 1);
        if (line[0] == '\0') {
            continue;
        }
        apply_kv(cfg, line, equals + 1);
    }

    fclose(fp);

    if (cfg->sip_domain[0] == '\0' || cfg->sip_user[0] == '\0' ||
        cfg->sip_password[0] == '\0') {
        return -2;
    }

    return 0;
}
