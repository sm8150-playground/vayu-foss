/*
 * Copyright (C) 2026 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <android/log.h>

#include "mdmimgload.h"

#define LOG_TAG "libmdmimgload"

#define KS_BINARY "/vendor/bin/ks"
#define PARTITION_DIR "/dev/block/bootdevice/by-name/"
#define MDM_DDR_IMAGE "9:mdmddr"


#define EXEC_ARGV_MAX 64

#define EXEC_ARGV_STORE_LIMIT 0x3d


#define PORT_PATH_SIZE 4096
#define CMDLINE_SIZE 1024


#define MAP_NAME_SIZE 0x20
#define MAP_PATH_SIZE 0x1000
#define MAP_ARGS_MAX 0x28


#define MAP_TERMINATOR "DEFAULT"

struct sahara_map_entry {
    char name[MAP_NAME_SIZE];
    char path[MAP_PATH_SIZE];
    const char *args[MAP_ARGS_MAX];
};


__attribute__((visibility("default"))) struct sahara_map_entry sahara_map_arr[] = {
    {
        .name = "MDM9x55",
        .path = "/firmware/image/mdm9x55",
        .args = {
            "-s", "21:/firmware/image/mdm9x55/sbl1.mbn",
            "-s", "25:/firmware/image/mdm9x55/tz.mbn",
            "-s", "23:/firmware/image/mdm9x55/rpm.mbn",
            "-s", "30:/firmware/image/mdm9x55/sdi.mbn",
            "-s", "29:/firmware/image/mdm9x55/acdb.mbn",
            "-s", "31:/firmware/image/mdm9x55/mba.mbn",
            "-s", "8:/firmware/image/mdm9x55/qdsp6sw.mbn",
            "-s", "28:/firmware/image/mdm9x55/dsp2.mbn",
            "-s", "6:/firmware/image/mdm9x55/apps.mbn",
            "-s", "16:/dev/block/bootdevice/by-name/mdm1m9kefs1",
            "-s", "17:/dev/block/bootdevice/by-name/mdm1m9kefs2",
            "-s", "20:/dev/block/bootdevice/by-name/mdm1m9kefs3",
            "-s", "34:/dev/block/bootdevice/by-name/mdmddr",
        },
    },
    {
        .name = "SDX50M",
        .path = "/vendor/firmware_mnt/image/sdx50m",
        .args = {
            "-s", "21:/vendor/firmware_mnt/image/sdx50m/sbl1.mbn",
            "-s", "25:/vendor/firmware_mnt/image/sdx50m/tz.mbn",
            "-s", "23:/vendor/firmware_mnt/image/sdx50m/rpm.mbn",
            "-s", "30:/vendor/firmware_mnt/image/sdx50m/sdi.mbn",
            "-s", "29:/vendor/firmware_mnt/image/sdx50m/acdb.mbn",
            "-s", "31:/vendor/firmware_mnt/image/sdx50m/mba.mbn",
            "-s", "8:/vendor/firmware_mnt/image/sdx50m/qdsp6sw.mbn",
            "-s", "28:/vendor/firmware_mnt/image/sdx50m/dsp2.mbn",
            "-s", "6:/vendor/firmware_mnt/image/sdx50m/apps.mbn",
            "-s", "16:/dev/block/bootdevice/by-name/mdm1m9kefs1",
            "-s", "17:/dev/block/bootdevice/by-name/mdm1m9kefs2",
            "-s", "20:/dev/block/bootdevice/by-name/mdm1m9kefs3",
            "-s", "34:/dev/block/bootdevice/by-name/mdmddr",
        },
    },
    {
        .name = "SDX55M",
        .path = "/vendor/firmware_mnt/image/sdx55m",
        .args = {
            "-s", "21:/vendor/firmware_mnt/image/sdx55m/sbl1.mbn",
            "-s", "37:/vendor/firmware_mnt/image/sdx55m/multi_image.mbn",
            "-s", "38:/vendor/firmware_mnt/image/sdx55m/xbl_config.elf",
            "-s", "36:/vendor/firmware_mnt/image/sdx55m/multi_image_qti.mbn",
            "-s", "41:/vendor/firmware_mnt/image/sdx55m/devcfg.mbn",
            "-s", "25:/vendor/firmware_mnt/image/sdx55m/tz.mbn",
            "-s", "23:/vendor/firmware_mnt/image/sdx55m/aop.mbn",
            "-s", "8:/vendor/firmware_mnt/image/sdx55m/qdsp6sw.mbn",
            "-s", "6:/vendor/firmware_mnt/image/sdx55m/apps.mbn",
            "-s", "16:/dev/block/bootdevice/by-name/mdm1m9kefs1",
            "-s", "17:/dev/block/bootdevice/by-name/mdm1m9kefs2",
            "-s", "20:/dev/block/bootdevice/by-name/mdm1m9kefs3",
            "-s", "29:/vendor/firmware_mnt/image/sdx55m/acdb.mbn",
            "-s", "34:/dev/block/bootdevice/by-name/mdmddr",
            "-s", "40:/vendor/firmware_mnt/image/sdx55m/apdp.mbn",
            "-s", "33:/vendor/firmware_mnt/image/sdx55m/hyp.mbn",
            "-s", "42:/vendor/firmware_mnt/image/sdx55m/sec.elf",
        },
    },
    {
        .name = "SDXPRAIRIE",
        .path = "/vendor/firmware_mnt/image/sdx55m",
        .args = {
            "-s", "21:/vendor/firmware_mnt/image/sdx55m/sbl1.mbn",
            "-s", "37:/vendor/firmware_mnt/image/sdx55m/multi_image.mbn",
            "-s", "38:/vendor/firmware_mnt/image/sdx55m/xbl_config.elf",
            "-s", "36:/vendor/firmware_mnt/image/sdx55m/multi_image_qti.mbn",
            "-s", "41:/vendor/firmware_mnt/image/sdx55m/devcfg.mbn",
            "-s", "25:/vendor/firmware_mnt/image/sdx55m/tz.mbn",
            "-s", "23:/vendor/firmware_mnt/image/sdx55m/aop.mbn",
            "-s", "8:/vendor/firmware_mnt/image/sdx55m/qdsp6sw.mbn",
            "-s", "6:/vendor/firmware_mnt/image/sdx55m/apps.mbn",
            "-s", "16:/dev/block/bootdevice/by-name/mdm1m9kefs1",
            "-s", "17:/dev/block/bootdevice/by-name/mdm1m9kefs2",
            "-s", "20:/dev/block/bootdevice/by-name/mdm1m9kefs3",
            "-s", "29:/vendor/firmware_mnt/image/sdx55m/acdb.mbn",
            "-s", "34:/dev/block/bootdevice/by-name/mdmddr",
            "-s", "40:/vendor/firmware_mnt/image/sdx55m/apdp.mbn",
            "-s", "33:/vendor/firmware_mnt/image/sdx55m/hyp.mbn",
            "-s", "42:/vendor/firmware_mnt/image/sdx55m/sec.elf",
        },
    },
    {
        .name = MAP_TERMINATOR,
        .path = "/firmware/image",
        .args = {
            "-s", "21:/firmware/image/sbl1.mbn",
            "-s", "25:/firmware/image/tz.mbn",
            "-s", "23:/firmware/image/rpm.mbn",
            "-s", "30:/firmware/image/sdi.mbn",
            "-s", "29:/firmware/image/acdb.mbn",
            "-s", "31:/firmware/image/mba.mbn",
            "-s", "8:/firmware/image/qdsp6sw.mbn",
            "-s", "28:/firmware/image/dsp2.mbn",
            "-s", "6:/firmware/image/apps.mbn",
            "-s", "16:/dev/block/bootdevice/by-name/mdm1m9kefs1",
            "-s", "17:/dev/block/bootdevice/by-name/mdm1m9kefs2",
            "-s", "20:/dev/block/bootdevice/by-name/mdm1m9kefs3",
            "-s", "34:/dev/block/bootdevice/by-name/mdmddr",
        },
    },
};

__attribute__((visibility("default"))) int wait_for_com_port(const char *port)
{
    struct stat st;
    int i;

    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Testing if file(port) \"%s\" exists", port);

    for (i = 0; i < 250; i++) {
        if (stat(port, &st) >= 0) {
            __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "File(port) :%s: located", port);
            return 1;
        }
        usleep(20000);
    }

    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG,
                        "'%s' was not found in first 5 seconds. Retrying..", port);

    for (i = 0; i < 950; i++) {
        if (stat(port, &st) >= 0) {
            __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "File(port) :%s: located", port);
            return 1;
        }
        usleep(100000);
    }

    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "'%s' was not found", port);
    return 0;
}

__attribute__((visibility("default"))) int mdm_img_transfer(const char *mdm_name,
                                                            const char *link_type,
                                                            const char *dev_name,
                                                            const char *w_arg,
                                                            const char *g_arg, int mode)
{
    const struct sahara_map_entry *map;
    char port[PORT_PATH_SIZE];
    char cmdline[CMDLINE_SIZE];
    char *argv[EXEC_ARGV_MAX];
    struct stat st;
    unsigned int index;
    unsigned int n;
    int status;
    pid_t pid;
    int i;
    int off;

    memset(port, 0, sizeof(port));

    if (mdm_name == NULL || link_type == NULL) {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Invalid arguments passed to %s",
                            "mdm_img_transfer");
        return 0;
    }

    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "MDM name '%s', Link type '%s'", mdm_name,
                        link_type);

    memset(cmdline, 0, sizeof(cmdline));
    memset(argv, 0, sizeof(argv));


    if (strncmp(sahara_map_arr[0].name, MAP_TERMINATOR, 7) == 0) {
        index = 0;
    } else {
        for (index = 0;; index++) {
            const struct sahara_map_entry *row = &sahara_map_arr[index];

            if (strncmp(row->name, mdm_name, strlen(row->name)) == 0) {
                if (stat(row->path, &st) == 0)
                    break;
                __android_log_print(ANDROID_LOG_INFO, LOG_TAG,
                                    "Unable to find %s.Using default mapping", strerror(errno));
            }

            if (strncmp(sahara_map_arr[index + 1].name, MAP_TERMINATOR, 7) == 0) {
                index++;
                break;
            }
        }
    }

    map = &sahara_map_arr[index];
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Using sahara mapping for %s", map->name);

    if (strncmp(link_type, "PCIe", sizeof("PCIe") - 1) != 0) {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Invalid MDM and link type");
        return 1;
    }

    if (dev_name != NULL)
        snprintf(port, PORT_PATH_SIZE - 1, "/dev/mhi_%s%s", dev_name, "_pipe_2");
    else
        snprintf(port, PORT_PATH_SIZE - 1, "%s", "/dev/mhi_pipe_2");

    status = 0;

    if (!wait_for_com_port(port))
        return 1;

    n = 0;
    argv[n++] = (char *)KS_BINARY;
    argv[n++] = (char *)"-o";
    argv[n++] = (char *)"-p";
    argv[n++] = port;
    argv[n++] = (char *)"-w";
    argv[n++] = (char *)w_arg;

    if (g_arg != NULL) {
        argv[n++] = (char *)"-g";
        argv[n++] = (char *)g_arg;
    }

    switch (mode) {
    case 3:
        argv[n++] = (char *)"-t";
        argv[n++] = (char *)"0";
        break;
    case 2:
        argv[n++] = (char *)"-i";
        break;
    case 1:
        argv[n++] = (char *)"-m";
        break;
    default:
        break;
    }

    argv[n] = (char *)"-r";
    argv[n + 1] = (char *)"-1";
    if (n + 2 > EXEC_ARGV_MAX - 1) {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "No more room for execve arguments");
        return 0;
    }

    argv[n + 2] = (char *)"--partition_path";
    if (n + 3 > EXEC_ARGV_MAX - 1) {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "No more room for execve arguments");
        return 0;
    }

    argv[n + 3] = (char *)PARTITION_DIR;
    n += 4;


    for (i = 0; map->args[i] != NULL && n <= EXEC_ARGV_MAX - 2; i++)
        argv[n++] = (char *)map->args[i];

    if (n > EXEC_ARGV_MAX - 2) {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "No more room for execve arguments");
        return 0;
    }

    if (n >= EXEC_ARGV_STORE_LIMIT) {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "No more room for execve arguments");
        return 0;
    }

    argv[n] = (char *)"-a";
    argv[n + 1] = (char *)MDM_DDR_IMAGE;

    argv[n + 2] = NULL;

    for (i = 0, off = 0; i < (int)(n + 2); i++) {
        off += snprintf(cmdline + off, sizeof(cmdline) - off, "%s ", argv[i]);
        if (off >= (int)sizeof(cmdline))
            break;
    }

    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Running '%s'", cmdline);

    pid = fork();
    if (pid < 0) {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Forking new process failed");
        return 1;
    }

    if (pid == 0) {
        if (execvp(argv[0], argv) == -1) {
            __android_log_print(ANDROID_LOG_ERROR, LOG_TAG,
                                "Spawning process using execvp failed");
            _exit(127);
        }
        return 1;
    }

    waitpid(pid, &status, 0);
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Process return value %d", status);

    if (((status >> 8) & 0xff) == 0) {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG,
                            "Sahara transfer completed successfully");
        return 0;
    }

    return ((status >> 8) & 0xff) == 11 ? 2 : 1;
}
