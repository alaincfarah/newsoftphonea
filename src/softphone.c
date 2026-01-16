// Copyright (c) 2026
#include "softphone.h"

#include <pjsua-lib/pjsua.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

typedef struct call_ctx {
    pj_bool_t active;
    pj_bool_t recording;
    pjsua_recorder_id recorder_id;
    char record_path[CONFIG_MAX_STR];
} call_ctx;

struct softphone {
    softphone_config cfg;
    pjsua_acc_id acc_id;
    pjsua_transport_id transport_id;
    int last_incoming_call;
    call_ctx calls[PJSUA_MAX_CALLS];
};

static softphone_t *g_phone = NULL;

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

static int starts_with_icase(const char *text, const char *prefix) {
    while (*prefix && *text) {
        char a = (char)tolower((unsigned char)*text);
        char b = (char)tolower((unsigned char)*prefix);
        if (a != b) {
            return 0;
        }
        text++;
        prefix++;
    }
    return *prefix == '\0';
}

static void trim_copy_token(const char *src, char *dst, size_t dst_size) {
    const char *start = src;
    const char *end = NULL;
    size_t len;

    while (*start == ' ' || *start == '\t') {
        start++;
    }
    end = start + strlen(start);
    while (end > start && (end[-1] == ' ' || end[-1] == '\t')) {
        end--;
    }
    len = (size_t)(end - start);
    if (len >= dst_size) {
        len = dst_size - 1;
    }
    memcpy(dst, start, len);
    dst[len] = '\0';
}

static void url_encode(const char *input, char *output, size_t out_size) {
    static const char *hex = "0123456789ABCDEF";
    size_t out_len = 0;

    if (!input || out_size == 0) {
        if (out_size > 0) {
            output[0] = '\0';
        }
        return;
    }

    while (*input && out_len + 1 < out_size) {
        unsigned char ch = (unsigned char)*input;
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
            (ch >= '0' && ch <= '9') || ch == '-' || ch == '_' ||
            ch == '.' || ch == '~') {
            output[out_len++] = (char)ch;
        } else {
            if (out_len + 3 >= out_size) {
                break;
            }
            output[out_len++] = '%';
            output[out_len++] = hex[ch >> 4];
            output[out_len++] = hex[ch & 0x0F];
        }
        input++;
    }
    output[out_len] = '\0';
}

static void build_url_from_template(const char *template_str, const char *cname,
                                    const char *caller, char *output,
                                    size_t out_size) {
    size_t out_len = 0;
    const char *cursor = template_str;
    char encoded_name[CONFIG_MAX_STR];
    char encoded_caller[CONFIG_MAX_STR];

    url_encode(cname, encoded_name, sizeof(encoded_name));
    url_encode(caller, encoded_caller, sizeof(encoded_caller));

    while (*cursor && out_len + 1 < out_size) {
        if (strncmp(cursor, "{CNAME}", 7) == 0) {
            size_t len = strlen(encoded_name);
            if (out_len + len >= out_size) {
                break;
            }
            memcpy(output + out_len, encoded_name, len);
            out_len += len;
            cursor += 7;
            continue;
        }
        if (strncmp(cursor, "{CALLER}", 8) == 0) {
            size_t len = strlen(encoded_caller);
            if (out_len + len >= out_size) {
                break;
            }
            memcpy(output + out_len, encoded_caller, len);
            out_len += len;
            cursor += 8;
            continue;
        }
        output[out_len++] = *cursor++;
    }

    output[out_len] = '\0';
}

static void extract_display_name(const char *remote_info, char *display_name,
                                 size_t display_size) {
    const char *start = remote_info;
    const char *end = NULL;

    if (!remote_info || display_size == 0) {
        return;
    }
    display_name[0] = '\0';

    if (*start == '"') {
        start++;
        end = strchr(start, '"');
        if (end && end > start) {
            size_t len = (size_t)(end - start);
            if (len >= display_size) {
                len = display_size - 1;
            }
            memcpy(display_name, start, len);
            display_name[len] = '\0';
            return;
        }
    }

    end = strchr(start, '<');
    if (end && end > start) {
        size_t len = (size_t)(end - start);
        while (len > 0 && (start[len - 1] == ' ' || start[len - 1] == '\t')) {
            len--;
        }
        if (len >= display_size) {
            len = display_size - 1;
        }
        memcpy(display_name, start, len);
        display_name[len] = '\0';
        return;
    }
}

static void extract_user_from_uri(const char *remote_info, char *caller,
                                  size_t caller_size) {
    const char *sip = NULL;
    const char *cursor = NULL;
    size_t len = 0;

    if (!remote_info || caller_size == 0) {
        return;
    }
    caller[0] = '\0';

    sip = strstr(remote_info, "sip:");
    if (!sip) {
        sip = strstr(remote_info, "sips:");
        if (sip) {
            sip += 5;
        }
    } else {
        sip += 4;
    }

    if (!sip) {
        safe_copy(caller, caller_size, remote_info);
        return;
    }

    cursor = sip;
    while (*cursor && *cursor != '@' && *cursor != '>' && *cursor != ';' &&
           *cursor != '?') {
        cursor++;
    }

    len = (size_t)(cursor - sip);
    if (len >= caller_size) {
        len = caller_size - 1;
    }
    memcpy(caller, sip, len);
    caller[len] = '\0';
}

static void open_url_for_call(const softphone_config *cfg, const char *cname,
                              const char *caller) {
    char url[CONFIG_MAX_STR * 2];

    if (!cfg || cfg->url_template[0] == '\0') {
        return;
    }

    build_url_from_template(cfg->url_template, cname, caller, url,
                            sizeof(url));

#ifdef _WIN32
    ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
#else
    if (fork() == 0) {
        execlp("xdg-open", "xdg-open", url, (char *)NULL);
        _exit(0);
    }
#endif
}

static void ensure_record_dir(const char *dir_path) {
    if (!dir_path || dir_path[0] == '\0') {
        return;
    }
#ifdef _WIN32
    CreateDirectoryA(dir_path, NULL);
#else
    mkdir(dir_path, 0755);
#endif
}

static void build_record_path(const char *dir, int call_id, char *out_path,
                              size_t out_size) {
    time_t now = time(NULL);
    struct tm tm_now;
    char time_buf[32];

#ifdef _WIN32
    localtime_s(&tm_now, &now);
#else
    localtime_r(&now, &tm_now);
#endif

    strftime(time_buf, sizeof(time_buf), "%Y%m%d_%H%M%S", &tm_now);
    snprintf(out_path, out_size, "%s/call_%d_%s.wav", dir, call_id, time_buf);
}

static void on_incoming_call(pjsua_acc_id acc_id, pjsua_call_id call_id,
                             pjsip_rx_data *rdata) {
    pjsua_call_info ci;
    char display_name[CONFIG_MAX_STR];
    char caller[CONFIG_MAX_STR];

    PJ_UNUSED_ARG(acc_id);
    PJ_UNUSED_ARG(rdata);

    if (!g_phone) {
        return;
    }

    pjsua_call_get_info(call_id, &ci);
    printf("Incoming call %d from %.*s\n", call_id, (int)ci.remote_info.slen,
           ci.remote_info.ptr);

    extract_display_name(ci.remote_info.ptr, display_name,
                         sizeof(display_name));
    extract_user_from_uri(ci.remote_info.ptr, caller, sizeof(caller));
    open_url_for_call(&g_phone->cfg,
                      display_name[0] ? display_name : caller,
                      caller[0] ? caller : display_name);

    g_phone->last_incoming_call = call_id;
    g_phone->calls[call_id].active = PJ_TRUE;

    if (g_phone->cfg.auto_answer) {
        pjsua_call_answer(call_id, 200, NULL, NULL);
    }
}

static void on_call_state(pjsua_call_id call_id, pjsip_event *e) {
    pjsua_call_info ci;

    PJ_UNUSED_ARG(e);

    if (!g_phone) {
        return;
    }

    pjsua_call_get_info(call_id, &ci);
    printf("Call %d state=%.*s\n", call_id, (int)ci.state_text.slen,
           ci.state_text.ptr);

    if (ci.state == PJSIP_INV_STATE_DISCONNECTED) {
        if (g_phone->calls[call_id].recording) {
            pjsua_recorder_destroy(g_phone->calls[call_id].recorder_id);
        }
        memset(&g_phone->calls[call_id], 0, sizeof(call_ctx));
    }
}

static void on_call_media_state(pjsua_call_id call_id) {
    pjsua_call_info ci;

    if (!g_phone) {
        return;
    }

    pjsua_call_get_info(call_id, &ci);
    if (ci.media_status == PJSUA_CALL_MEDIA_ACTIVE) {
        pjsua_conf_connect(ci.conf_slot, 0);
        pjsua_conf_connect(0, ci.conf_slot);
    }
}

static void disable_all_codecs(void) {
    pjsua_codec_info codecs[64];
    unsigned count = PJ_ARRAY_SIZE(codecs);
    unsigned i;

    if (pjsua_enum_codecs(codecs, &count) != PJ_SUCCESS) {
        return;
    }

    for (i = 0; i < count; ++i) {
        pjsua_codec_set_priority(&codecs[i].codec_id,
                                 PJMEDIA_CODEC_PRIO_DISABLED);
    }
}

static void apply_codec_priority(const char *token, pj_uint8_t priority) {
    pjsua_codec_info codecs[64];
    unsigned count = PJ_ARRAY_SIZE(codecs);
    unsigned i;
    char normalized[32];

    if (!token || token[0] == '\0') {
        return;
    }

    trim_copy_token(token, normalized, sizeof(normalized));
    if (normalized[0] == '\0') {
        return;
    }

    if (starts_with_icase(normalized, "ULAW")) {
        safe_copy(normalized, sizeof(normalized), "PCMU");
    } else if (starts_with_icase(normalized, "ALAW")) {
        safe_copy(normalized, sizeof(normalized), "PCMA");
    }

    if (pjsua_enum_codecs(codecs, &count) != PJ_SUCCESS) {
        return;
    }

    for (i = 0; i < count; ++i) {
        const char *id = codecs[i].codec_id.ptr;
        if (starts_with_icase(id, normalized)) {
            pjsua_codec_set_priority(&codecs[i].codec_id, priority);
            printf("Codec enabled: %.*s (prio %u)\n",
                   (int)codecs[i].codec_id.slen, codecs[i].codec_id.ptr,
                   (unsigned)priority);
        }
    }
}

static void set_codec_priorities(const softphone_config *cfg) {
    char list_copy[CONFIG_MAX_STR];
    char *token = NULL;
    pj_uint8_t priority = PJMEDIA_CODEC_PRIO_HIGHEST;

    disable_all_codecs();
    safe_copy(list_copy, sizeof(list_copy), cfg->codec_order);

    token = strtok(list_copy, ",");
    while (token && priority > PJMEDIA_CODEC_PRIO_LOW) {
        apply_codec_priority(token, priority);
        priority--;
        token = strtok(NULL, ",");
    }
}

int softphone_create(softphone_t **out_phone, const softphone_config *cfg) {
    pjsua_config ua_cfg;
    pjsua_logging_config log_cfg;
    pjsua_media_config media_cfg;
    pjsua_transport_config tcfg;
    pjsua_acc_config acc_cfg;
    pjsip_transport_type_e transport_type = PJSIP_TRANSPORT_UDP;
    char id_buf[CONFIG_MAX_STR * 2];
    char reg_uri[CONFIG_MAX_STR * 2];
    char from_uri[CONFIG_MAX_STR * 3];
    int status;
    unsigned proxy_count = 0;

    if (!out_phone || !cfg) {
        return -1;
    }

    *out_phone = NULL;
    g_phone = (softphone_t *)calloc(1, sizeof(softphone_t));
    if (!g_phone) {
        return -1;
    }

    g_phone->cfg = *cfg;
    g_phone->last_incoming_call = PJSUA_INVALID_ID;

    status = pjsua_create();
    if (status != PJ_SUCCESS) {
        softphone_destroy(g_phone);
        return -1;
    }

    pjsua_config_default(&ua_cfg);
    ua_cfg.cb.on_incoming_call = &on_incoming_call;
    ua_cfg.cb.on_call_state = &on_call_state;
    ua_cfg.cb.on_call_media_state = &on_call_media_state;
    ua_cfg.thread_cnt = 2;

    pjsua_logging_config_default(&log_cfg);
    log_cfg.level = cfg->log_level;
    log_cfg.console_level = cfg->console_level;

    pjsua_media_config_default(&media_cfg);

    status = pjsua_init(&ua_cfg, &log_cfg, &media_cfg);
    if (status != PJ_SUCCESS) {
        softphone_destroy(g_phone);
        return -1;
    }

    pjsua_transport_config_default(&tcfg);
    tcfg.port = (pj_uint16_t)cfg->local_port;
    if (cfg->local_bind[0]) {
        tcfg.bound_addr = pj_str((char *)cfg->local_bind);
    }

    if (starts_with_icase(cfg->transport, "tcp")) {
        transport_type = PJSIP_TRANSPORT_TCP;
    } else if (starts_with_icase(cfg->transport, "tls")) {
        transport_type = PJSIP_TRANSPORT_TLS;
    }

    status = pjsua_transport_create(transport_type, &tcfg,
                                    &g_phone->transport_id);
    if (status != PJ_SUCCESS) {
        softphone_destroy(g_phone);
        return -1;
    }

    pjsua_acc_config_default(&acc_cfg);
    snprintf(id_buf, sizeof(id_buf), "sip:%s@%s", cfg->sip_user,
             cfg->sip_domain);
    snprintf(reg_uri, sizeof(reg_uri), "sip:%s", cfg->sip_domain);

    if (cfg->display_name[0]) {
        snprintf(from_uri, sizeof(from_uri), "\"%s\" <%s>",
                 cfg->display_name, id_buf);
        acc_cfg.id = pj_str(from_uri);
    } else {
        acc_cfg.id = pj_str(id_buf);
    }
    acc_cfg.reg_uri = pj_str(reg_uri);

    if (cfg->sip_proxy[0]) {
        acc_cfg.proxy[proxy_count++] = pj_str((char *)cfg->sip_proxy);
    }
    if (cfg->outbound_proxy[0]) {
        acc_cfg.proxy[proxy_count++] = pj_str((char *)cfg->outbound_proxy);
    }
    acc_cfg.proxy_cnt = proxy_count;

    acc_cfg.cred_count = 1;
    acc_cfg.cred_info[0].realm = pj_str((char *)"*");
    acc_cfg.cred_info[0].scheme = pj_str((char *)"digest");
    acc_cfg.cred_info[0].username = pj_str((char *)cfg->sip_user);
    acc_cfg.cred_info[0].data_type = PJSIP_CRED_DATA_PLAIN_PASSWD;
    acc_cfg.cred_info[0].data = pj_str((char *)cfg->sip_password);

    status = pjsua_acc_add(&acc_cfg, PJ_TRUE, &g_phone->acc_id);
    if (status != PJ_SUCCESS) {
        softphone_destroy(g_phone);
        return -1;
    }

    *out_phone = g_phone;
    return 0;
}

int softphone_start(softphone_t *phone) {
    if (!phone) {
        return -1;
    }
    if (pjsua_start() != PJ_SUCCESS) {
        return -1;
    }
    set_codec_priorities(&phone->cfg);
    return 0;
}

void softphone_destroy(softphone_t *phone) {
    if (!phone) {
        return;
    }
    if (phone == g_phone) {
        g_phone = NULL;
    }
    pjsua_destroy();
    free(phone);
}

int softphone_make_call(softphone_t *phone, const char *dest_uri,
                        int *out_call_id) {
    char full_uri[CONFIG_MAX_STR * 2];
    pj_str_t uri;
    pjsua_call_id call_id;

    if (!phone || !dest_uri) {
        return -1;
    }

    if (starts_with_icase(dest_uri, "sip:") ||
        starts_with_icase(dest_uri, "sips:")) {
        safe_copy(full_uri, sizeof(full_uri), dest_uri);
    } else {
        snprintf(full_uri, sizeof(full_uri), "sip:%s@%s", dest_uri,
                 phone->cfg.sip_domain);
    }

    uri = pj_str(full_uri);
    if (pjsua_call_make_call(phone->acc_id, &uri, 0, NULL, NULL, &call_id) !=
        PJ_SUCCESS) {
        return -1;
    }

    if (out_call_id) {
        *out_call_id = call_id;
    }
    return 0;
}

int softphone_answer_call(softphone_t *phone, int call_id, int code) {
    PJ_UNUSED_ARG(phone);
    return (pjsua_call_answer(call_id, code, NULL, NULL) == PJ_SUCCESS) ? 0 : -1;
}

int softphone_hangup_call(softphone_t *phone, int call_id, int code) {
    PJ_UNUSED_ARG(phone);
    return (pjsua_call_hangup(call_id, code, NULL, NULL) == PJ_SUCCESS) ? 0
                                                                       : -1;
}

int softphone_hold_call(softphone_t *phone, int call_id) {
    PJ_UNUSED_ARG(phone);
    return (pjsua_call_set_hold(call_id, NULL) == PJ_SUCCESS) ? 0 : -1;
}

int softphone_unhold_call(softphone_t *phone, int call_id) {
    PJ_UNUSED_ARG(phone);
    return (pjsua_call_reinvite(call_id, PJ_TRUE, NULL) == PJ_SUCCESS) ? 0
                                                                       : -1;
}

int softphone_blind_transfer(softphone_t *phone, int call_id,
                             const char *dest_uri) {
    char full_uri[CONFIG_MAX_STR * 2];
    pj_str_t uri;

    if (!phone || !dest_uri) {
        return -1;
    }

    if (starts_with_icase(dest_uri, "sip:") ||
        starts_with_icase(dest_uri, "sips:")) {
        safe_copy(full_uri, sizeof(full_uri), dest_uri);
    } else {
        snprintf(full_uri, sizeof(full_uri), "sip:%s@%s", dest_uri,
                 phone->cfg.sip_domain);
    }

    uri = pj_str(full_uri);
    return (pjsua_call_xfer(call_id, &uri, NULL) == PJ_SUCCESS) ? 0 : -1;
}

int softphone_start_warm_transfer(softphone_t *phone, int call_id,
                                 const char *dest_uri,
                                 int *out_consult_call_id) {
    int consult_call_id = PJSUA_INVALID_ID;

    if (!phone || !dest_uri) {
        return -1;
    }

    if (softphone_hold_call(phone, call_id) != 0) {
        return -1;
    }

    if (softphone_make_call(phone, dest_uri, &consult_call_id) != 0) {
        return -1;
    }

    if (out_consult_call_id) {
        *out_consult_call_id = consult_call_id;
    }
    return 0;
}

int softphone_complete_warm_transfer(softphone_t *phone, int call_id,
                                    int consult_call_id) {
    PJ_UNUSED_ARG(phone);
    return (pjsua_call_xfer_replaces(call_id, consult_call_id, 0, NULL) ==
            PJ_SUCCESS)
               ? 0
               : -1;
}

int softphone_start_recording(softphone_t *phone, int call_id) {
    pjsua_recorder_id rec_id;
    pj_str_t file;
    call_ctx *ctx = NULL;

    if (!phone || call_id < 0 || call_id >= PJSUA_MAX_CALLS) {
        return -1;
    }

    ctx = &phone->calls[call_id];
    if (ctx->recording) {
        return 0;
    }

    ensure_record_dir(phone->cfg.record_dir);
    build_record_path(phone->cfg.record_dir, call_id, ctx->record_path,
                      sizeof(ctx->record_path));

    file = pj_str(ctx->record_path);
    if (pjsua_recorder_create(&file, 0, NULL, 0, 0, &rec_id) != PJ_SUCCESS) {
        return -1;
    }

    pjsua_conf_connect(pjsua_call_get_conf_port(call_id),
                       pjsua_recorder_get_conf_port(rec_id));

    ctx->recorder_id = rec_id;
    ctx->recording = PJ_TRUE;
    printf("Recording started: %s\n", ctx->record_path);
    return 0;
}

int softphone_stop_recording(softphone_t *phone, int call_id) {
    call_ctx *ctx = NULL;

    if (!phone || call_id < 0 || call_id >= PJSUA_MAX_CALLS) {
        return -1;
    }

    ctx = &phone->calls[call_id];
    if (!ctx->recording) {
        return 0;
    }

    pjsua_recorder_destroy(ctx->recorder_id);
    ctx->recording = PJ_FALSE;
    ctx->recorder_id = PJSUA_INVALID_ID;
    printf("Recording stopped for call %d\n", call_id);
    return 0;
}

int softphone_get_last_incoming_call(softphone_t *phone) {
    if (!phone) {
        return PJSUA_INVALID_ID;
    }
    return phone->last_incoming_call;
}

void softphone_dump_calls(softphone_t *phone) {
    pjsua_call_id call_ids[PJSUA_MAX_CALLS];
    unsigned count = PJ_ARRAY_SIZE(call_ids);
    unsigned i;

    if (!phone) {
        return;
    }

    if (pjsua_enum_calls(call_ids, &count) != PJ_SUCCESS) {
        printf("Unable to list calls.\n");
        return;
    }

    if (count == 0) {
        printf("No active calls.\n");
        return;
    }

    for (i = 0; i < count; ++i) {
        pjsua_call_info ci;
        pjsua_call_get_info(call_ids[i], &ci);
        printf("Call %d: state=%.*s remote=%.*s\n", call_ids[i],
               (int)ci.state_text.slen, ci.state_text.ptr,
               (int)ci.remote_info.slen, ci.remote_info.ptr);
    }
}
