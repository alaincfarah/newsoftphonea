// Copyright (c) 2026
#ifndef SOFTPHONE_H
#define SOFTPHONE_H

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct softphone softphone_t;

int softphone_create(softphone_t **out_phone, const softphone_config *cfg);
int softphone_start(softphone_t *phone);
void softphone_destroy(softphone_t *phone);

int softphone_make_call(softphone_t *phone, const char *dest_uri, int *out_call_id);
int softphone_answer_call(softphone_t *phone, int call_id, int code);
int softphone_hangup_call(softphone_t *phone, int call_id, int code);
int softphone_hold_call(softphone_t *phone, int call_id);
int softphone_unhold_call(softphone_t *phone, int call_id);
int softphone_blind_transfer(softphone_t *phone, int call_id, const char *dest_uri);
int softphone_start_warm_transfer(softphone_t *phone, int call_id, const char *dest_uri,
                                 int *out_consult_call_id);
int softphone_complete_warm_transfer(softphone_t *phone, int call_id,
                                    int consult_call_id);
int softphone_start_recording(softphone_t *phone, int call_id);
int softphone_stop_recording(softphone_t *phone, int call_id);

int softphone_get_last_incoming_call(softphone_t *phone);
void softphone_dump_calls(softphone_t *phone);

#ifdef __cplusplus
}
#endif

#endif
