/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#define LOG_TAG "sns_fastrpc_util"

#include "sns_fastrpc_util.h"

#include <dlfcn.h>
#include <log/log.h>
#include <pthread.h>
#include <string.h>
#include <sys/stat.h>


#define SNS_SLPI_BOOT_NODE "/sys/kernel/boot_slpi"
#define SNS_SLPI_STUB "libsdsprpc.so"
#define SNS_ADSP_STUB "libadsprpc.so"


#define SNS_SENSORSPD_NAME "'\":;./\\createstaticpd:sensorspd"

static pthread_mutex_t sns_rpc_lock = PTHREAD_MUTEX_INITIALIZER;
static void *sns_rpc_lib;
static sns_fastrpc_util_t sns_rpc_funcs;
static uint32_t sns_rpc_handle;

const char *get_lib(void)
{
    struct stat st;

    memset(&st, 0, sizeof(st));

    if (stat(SNS_SLPI_BOOT_NODE, &st) == 0) {
        return SNS_SLPI_STUB;
    }
    return SNS_ADSP_STUB;
}

int sns_rpc_get_handle(sns_fastrpc_util_t *util)
{
    int ret;
    void *lib;

    pthread_mutex_lock(&sns_rpc_lock);

    if (sns_rpc_lib != NULL) {
        *util = sns_rpc_funcs;
        ALOGI("fastrpc resources are already initialized");
        ret = 0;
        goto out;
    }

    memset(&sns_rpc_funcs, 0, sizeof(sns_rpc_funcs));

    lib = dlopen(get_lib(), RTLD_NOW);
    if (lib == NULL) {
        ALOGE("%s: load stub(%s) failed !", "sns_rpc_get_handle", get_lib());
        ret = -1;
        goto out;
    }
    ALOGI("%s: dlopen(%s) done !", "sns_rpc_get_handle", get_lib());

    sns_rpc_funcs.remote_handle_open = dlsym(lib, "remote_handle_open");
    sns_rpc_funcs.remote_handle_close = dlsym(lib, "remote_handle_close");
    sns_rpc_funcs.remote_handle_invoke = dlsym(lib, "remote_handle_invoke");
    sns_rpc_funcs.remote_register_buf = dlsym(lib, "remote_register_buf");
    sns_rpc_funcs.remote_register_buf_attr = dlsym(lib, "remote_register_buf_attr");
    sns_rpc_funcs.rpcmem_alloc = dlsym(lib, "rpcmem_alloc");
    sns_rpc_funcs.rpcmem_free = dlsym(lib, "rpcmem_free");
    sns_rpc_funcs.rpcmem_to_fd = dlsym(lib, "rpcmem_to_fd");

    if (sns_rpc_funcs.rpcmem_to_fd == NULL ||
        sns_rpc_funcs.remote_handle_open == NULL ||
        sns_rpc_funcs.remote_handle_close == NULL ||
        sns_rpc_funcs.remote_handle_invoke == NULL ||
        sns_rpc_funcs.rpcmem_alloc == NULL ||
        sns_rpc_funcs.rpcmem_free == NULL) {
        ALOGE("%s: dlsym failed !", "sns_rpc_get_handle");
        dlclose(lib);
        ret = -1;
        goto out;
    }

    if (sns_rpc_handle == 0) {
        if (sns_rpc_funcs.remote_handle_open(SNS_SENSORSPD_NAME, &sns_rpc_handle) != 0) {
            ALOGE("failed to open remote handle for sensorspd");
            dlclose(lib);
            ret = -1;
            goto out;
        }
        ALOGI("opened remote handle for sensorspd");

        sns_rpc_handle = 1;
    } else {
        ALOGI("using opened remote handle for sensorspd");
    }

    sns_rpc_lib = lib;
    *util = sns_rpc_funcs;
    ALOGI("fastrpc resources are initialized");
    ret = 0;

out:
    pthread_mutex_unlock(&sns_rpc_lock);
    return ret;
}

void sns_rpc_reset(void)
{
    pthread_mutex_lock(&sns_rpc_lock);

    ALOGI("reset fastrpc resources");

    if (sns_rpc_lib == NULL) {
        ALOGE("fastrpc resources are not initialized before. nothing to reset");
    } else {
        if (sns_rpc_handle != 0) {
            ALOGI("close remote handle for sensor pd");
            sns_rpc_funcs.remote_handle_close(sns_rpc_handle);
            sns_rpc_handle = 0;
        }
        ALOGI("unload fastrpc lib");
        dlclose(sns_rpc_lib);
        sns_rpc_lib = NULL;
        ALOGI("all fastrpc resources are reset successfully");
    }

    pthread_mutex_unlock(&sns_rpc_lock);
}
