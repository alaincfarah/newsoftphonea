// Copyright (c) 2026
#include "config.h"
#include "softphone.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_help(void) {
    printf("\nCommands:\n");
    printf("  help\n");
    printf("  list\n");
    printf("  call <sip-uri|extension>\n");
    printf("  answer [call-id]\n");
    printf("  hangup [call-id]\n");
    printf("  hold <call-id>\n");
    printf("  unhold <call-id>\n");
    printf("  blindxfer <call-id> <sip-uri>\n");
    printf("  warmxfer <call-id> <sip-uri>\n");
    printf("  warmcomplete <call-id> <consult-call-id>\n");
    printf("  record start <call-id>\n");
    printf("  record stop <call-id>\n");
    printf("  quit\n\n");
}

static int parse_optional_call_id(softphone_t *phone, const char *arg,
                                  int *out_id) {
    int call_id = -1;

    if (!out_id) {
        return -1;
    }

    if (arg && arg[0]) {
        call_id = atoi(arg);
    } else {
        call_id = softphone_get_last_incoming_call(phone);
    }

    if (call_id < 0) {
        return -1;
    }

    *out_id = call_id;
    return 0;
}

int main(int argc, char *argv[]) {
    const char *config_path = "config/softphone.ini";
    softphone_config cfg;
    softphone_t *phone = NULL;
    char line[512];

    if (argc > 1) {
        config_path = argv[1];
    }

    if (config_load(config_path, &cfg) != 0) {
        printf("Failed to load config: %s\n", config_path);
        return 1;
    }

    if (softphone_create(&phone, &cfg) != 0) {
        printf("Failed to initialize softphone.\n");
        return 1;
    }

    if (softphone_start(phone) != 0) {
        printf("Failed to start softphone.\n");
        softphone_destroy(phone);
        return 1;
    }

    printf("Softphone ready. Type 'help' for commands.\n");

    while (1) {
        char *cmd = NULL;
        char *arg1 = NULL;
        char *arg2 = NULL;
        char *arg3 = NULL;
        int call_id = -1;
        int consult_call_id = -1;

        printf("softphone> ");
        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }

        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '\0') {
            continue;
        }

        cmd = strtok(line, " ");
        if (!cmd) {
            continue;
        }

        if (strcmp(cmd, "help") == 0) {
            print_help();
            continue;
        }
        if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) {
            break;
        }
        if (strcmp(cmd, "list") == 0) {
            softphone_dump_calls(phone);
            continue;
        }

        if (strcmp(cmd, "call") == 0) {
            arg1 = strtok(NULL, " ");
            if (!arg1) {
                printf("Usage: call <sip-uri|extension>\n");
                continue;
            }
            if (softphone_make_call(phone, arg1, &call_id) == 0) {
                printf("Dialing... call id=%d\n", call_id);
            } else {
                printf("Failed to place call.\n");
            }
            continue;
        }

        if (strcmp(cmd, "answer") == 0) {
            arg1 = strtok(NULL, " ");
            if (parse_optional_call_id(phone, arg1, &call_id) != 0) {
                printf("Usage: answer [call-id]\n");
                continue;
            }
            softphone_answer_call(phone, call_id, 200);
            continue;
        }

        if (strcmp(cmd, "hangup") == 0) {
            arg1 = strtok(NULL, " ");
            if (parse_optional_call_id(phone, arg1, &call_id) != 0) {
                printf("Usage: hangup [call-id]\n");
                continue;
            }
            softphone_hangup_call(phone, call_id, 0);
            continue;
        }

        if (strcmp(cmd, "hold") == 0) {
            arg1 = strtok(NULL, " ");
            if (!arg1) {
                printf("Usage: hold <call-id>\n");
                continue;
            }
            softphone_hold_call(phone, atoi(arg1));
            continue;
        }

        if (strcmp(cmd, "unhold") == 0) {
            arg1 = strtok(NULL, " ");
            if (!arg1) {
                printf("Usage: unhold <call-id>\n");
                continue;
            }
            softphone_unhold_call(phone, atoi(arg1));
            continue;
        }

        if (strcmp(cmd, "blindxfer") == 0) {
            arg1 = strtok(NULL, " ");
            arg2 = strtok(NULL, " ");
            if (!arg1 || !arg2) {
                printf("Usage: blindxfer <call-id> <sip-uri>\n");
                continue;
            }
            if (softphone_blind_transfer(phone, atoi(arg1), arg2) != 0) {
                printf("Blind transfer failed.\n");
            }
            continue;
        }

        if (strcmp(cmd, "warmxfer") == 0) {
            arg1 = strtok(NULL, " ");
            arg2 = strtok(NULL, " ");
            if (!arg1 || !arg2) {
                printf("Usage: warmxfer <call-id> <sip-uri>\n");
                continue;
            }
            if (softphone_start_warm_transfer(phone, atoi(arg1), arg2,
                                              &consult_call_id) == 0) {
                printf("Warm transfer started. Consult call id=%d\n",
                       consult_call_id);
            } else {
                printf("Warm transfer failed.\n");
            }
            continue;
        }

        if (strcmp(cmd, "warmcomplete") == 0) {
            arg1 = strtok(NULL, " ");
            arg2 = strtok(NULL, " ");
            if (!arg1 || !arg2) {
                printf("Usage: warmcomplete <call-id> <consult-call-id>\n");
                continue;
            }
            call_id = atoi(arg1);
            consult_call_id = atoi(arg2);
            if (softphone_complete_warm_transfer(phone, call_id,
                                                 consult_call_id) != 0) {
                printf("Warm transfer complete failed.\n");
            }
            continue;
        }

        if (strcmp(cmd, "record") == 0) {
            arg1 = strtok(NULL, " ");
            arg2 = strtok(NULL, " ");
            arg3 = strtok(NULL, " ");
            if (!arg1 || !arg2 || arg3) {
                printf("Usage: record start <call-id>\n");
                printf("       record stop <call-id>\n");
                continue;
            }
            call_id = atoi(arg2);
            if (strcmp(arg1, "start") == 0) {
                softphone_start_recording(phone, call_id);
            } else if (strcmp(arg1, "stop") == 0) {
                softphone_stop_recording(phone, call_id);
            } else {
                printf("Usage: record start <call-id>\n");
                printf("       record stop <call-id>\n");
            }
            continue;
        }

        printf("Unknown command. Type 'help' for options.\n");
    }

    softphone_destroy(phone);
    return 0;
}
