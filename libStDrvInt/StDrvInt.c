/*
 * Copyright (C) 2026 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <android/log.h>

#define TAG "libStDrvInt: "

#define EXPORT __attribute__((visibility("default")))




static int g_ctrl_fd = -1;
static int g_event_fd = -1;


#define PATH_SIZE 100
static char g_enable_path[PATH_SIZE];
static char g_touch_path[PATH_SIZE];

#define CONTROL_ROOT "/data/vendor/ts_loopback"
#define TOUCH_ENTRY "secure_touch"
#define ENABLE_SUFFIX "_enable"




static int find_secure_touch(const char *path)
{
    char dir[PATH_SIZE];
    DIR *d;
    int result;

    if (path == NULL) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Invalid input");
        return -EINVAL;
    }

    __android_log_print(ANDROID_LOG_DEBUG, TAG, "Searching %s", path);

    d = opendir(path);
    if (d == NULL) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Failed to open %s: %s (%d)",
                            path, strerror(errno), errno);
        return -EINVAL;
    }

    memset(dir, 0, sizeof(dir));
    strlcpy(dir, path, sizeof(dir));
    dir[strnlen(dir, sizeof(dir))] = '/';

    for (;;) {
        struct dirent *entry = readdir(d);

        if (entry == NULL) {
            result = -ENOENT;
            break;
        }
        if (strncmp(entry->d_name, ".", 10) == 0)
            continue;
        if (strncmp(entry->d_name, "..", 10) == 0)
            continue;
        if (entry->d_type == DT_DIR) {
            char sub[PATH_SIZE];

            memset(sub, 0, sizeof(sub));
            strlcpy(sub, dir, sizeof(sub));
            strlcat(sub, entry->d_name, sizeof(sub));
            if (find_secure_touch(sub) == 0) {
                result = 0;
                break;
            }
            continue;
        }
        if (entry->d_type != DT_REG)
            continue;
        if (strcmp(entry->d_name, TOUCH_ENTRY) != 0)
            continue;

        strlcpy(g_touch_path, dir, sizeof(g_touch_path));
        strlcat(g_touch_path, entry->d_name, sizeof(g_touch_path));
        strlcpy(g_enable_path, g_touch_path, sizeof(g_enable_path));
        strlcat(g_enable_path, ENABLE_SUFFIX, sizeof(g_enable_path));
        __android_log_print(ANDROID_LOG_DEBUG, TAG, "Secure touch found @ %s: %s - %s",
                            dir, g_enable_path, g_touch_path);
        result = 0;
        break;
    }

    closedir(d);
    return result;
}




EXPORT int stStartSession(int *token)
{
    char buffer[PATH_SIZE];
    char dash[2];
    char slash[2];
    char *save;
    char *field;
    int value = 0;
    int rc;
    size_t i;

    if (g_ctrl_fd != -1) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Session already started");
        return EBUSY;
    }

    __android_log_print(ANDROID_LOG_DEBUG, TAG, "Searching %s", CONTROL_ROOT);
    rc = find_secure_touch(CONTROL_ROOT);
    if (rc != 0) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "No secure touch support detected");
        return rc;
    }


    memset(buffer, 0, sizeof(buffer));
    for (i = 0; i < strlen(g_enable_path); i++)
        buffer[i] = g_enable_path[i];

    dash[0] = '-';
    dash[1] = '\0';
    slash[0] = '/';
    slash[1] = '\0';
    save = NULL;

    field = strtok_r(buffer, dash, &save);
    if (field != NULL) {
        field = strtok_r(NULL, dash, &save);
        if (field != NULL) {
            field = strtok_r(NULL, slash, &save);
            if (field != NULL)
                value = (int)strtol(field, NULL, 16);
        }
    }
    if (token != NULL)
        *token = value;

    __android_log_print(ANDROID_LOG_DEBUG, TAG, "Opening control file %s", g_enable_path);

    g_ctrl_fd = open(g_enable_path, O_WRONLY);
    if (g_ctrl_fd == -1) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Error opening %s: %s (%d)",
                            g_enable_path, strerror(errno), errno);
        return errno;
    }

    if (pwrite(g_ctrl_fd, "1", 1, 0) <= 0) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Error writing (1) to %s: %s (%d)",
                            g_enable_path, strerror(errno), errno);
        close(g_ctrl_fd);
        g_ctrl_fd = -1;
        return errno;
    }

    return 0;
}


EXPORT int stTerminateSession(int force)
{
    if (force != 0 && g_ctrl_fd == -1) {
        int rc;

        __android_log_print(ANDROID_LOG_DEBUG, TAG, "Searching %s", CONTROL_ROOT);
        rc = find_secure_touch(CONTROL_ROOT);
        if (rc != 0) {
            __android_log_print(ANDROID_LOG_ERROR, TAG, "No secure touch support detected");
            return rc;
        }
        g_ctrl_fd = open(g_enable_path, O_WRONLY);
    }

    if (g_ctrl_fd == -1) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "No session active");
        return ENODEV;
    }

    if (pwrite(g_ctrl_fd, "0", 1, 0) <= 0) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Error writing (0) to %s: %s (%d)",
                            g_enable_path, strerror(errno), errno);
        return errno;
    }

    close(g_ctrl_fd);
    g_ctrl_fd = -1;
    if (g_event_fd != -1) {
        close(g_event_fd);
        g_event_fd = -1;
    }

    return 0;
}


EXPORT int stWaitForEvent(int cancel_fd, int timeout_ms)
{
    char flag = 0;
    struct pollfd *fds;
    nfds_t nfds;
    int result;
    int ret;

    if (g_event_fd == -1) {
        g_event_fd = open(g_touch_path, O_RDONLY);
        if (g_event_fd == -1) {
            __android_log_print(ANDROID_LOG_ERROR, TAG, "Error opening %s: %s (%d)",
                                g_touch_path, strerror(errno), errno);
            return errno;
        }
    }

    if (pread(g_event_fd, &flag, 1, 0) <= 0) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Error reading from %s: %s (%d)",
                            g_touch_path, strerror(errno), errno);
        return errno;
    }
    if (flag == '1')
        return 0;

    nfds = (cancel_fd != -1) ? 2 : 1;
    fds = calloc(nfds, sizeof(*fds));
    if (fds == NULL)
        return -ENOMEM;

    fds[0].fd = g_event_fd;
    fds[0].events = POLLIN | POLLPRI | POLLERR;
    if (cancel_fd != -1) {
        fds[1].fd = cancel_fd;
        fds[1].events = POLLIN;
    }

    for (;;) {
        ret = poll(fds, nfds, timeout_ms);
        if (ret < 0) {
            __android_log_print(ANDROID_LOG_ERROR, TAG,
                                "Error condition during polling: %s (%d)",
                                strerror(errno), errno);
            result = errno;
            break;
        }

        result = -ETIMEDOUT;
        if (timeout_ms == 0)
            break;
        if (ret == 0)
            break;
        if (cancel_fd != -1 && fds[1].revents != 0) {
            result = -ECONNABORTED;
            break;
        }
        if (fds[0].revents == 0) {
            result = ret;
            break;
        }

        if (pread(g_event_fd, &flag, 1, 0) <= 0) {
            __android_log_print(ANDROID_LOG_ERROR, TAG, "Error reading from %s: %s (%d)",
                                g_touch_path, strerror(errno), errno);
            result = errno;
            break;
        }
        if (flag == '1') {
            result = 0;
            break;
        }

    }

    free(fds);
    return result;
}





#define UBSAN_MAX_ERRORS 20

static uint32_t ubsan_report_count;
static uintptr_t ubsan_reported[UBSAN_MAX_ERRORS];

static void ubsan_write(const char *msg)
{
    write(STDERR_FILENO, msg, strlen(msg));
}


static bool ubsan_should_report(uintptr_t site)
{
    uint32_t count;

    if (site == 0)
        return false;

    count = __atomic_load_n(&ubsan_report_count, __ATOMIC_RELAXED);
    if (count > UBSAN_MAX_ERRORS)
        return false;

    for (;;) {
        if (count != 0 && count != UBSAN_MAX_ERRORS) {
            uint32_t remaining = count;
            const uintptr_t *slot = ubsan_reported;

            do {
                if (*slot == 0)
                    break;
                if (*slot == site)
                    return false;
                slot++;
            } while (--remaining != 0);
        }


        {
            uint32_t expected = count;

            if (__atomic_compare_exchange_n(&ubsan_report_count, &expected, count + 1,
                                            false, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED)) {
                if (count == UBSAN_MAX_ERRORS) {
                    ubsan_write("ubsan: too many errors\n");
                    return false;
                }
                ubsan_reported[count] = site;
                return true;
            }
            count = expected;
            if (count > UBSAN_MAX_ERRORS)
                return false;
        }
    }
}


extern void android_set_abort_message(const char *message) __attribute__((weak));


static void ubsan_abort_with(const char *msg)
{
    if (android_set_abort_message != NULL)
        android_set_abort_message(msg);
    abort();
}

#define UBSAN_HANDLER(name, message)                                        \
    EXPORT void __ubsan_handle_##name##_minimal(void)                       \
    {                                                                       \
        if (ubsan_should_report((uintptr_t)__builtin_return_address(0)))    \
            ubsan_write(message);                                           \
    }

#define UBSAN_ABORT_HANDLER(name, message, abort_message)                   \
    EXPORT void __ubsan_handle_##name##_minimal_abort(void)                 \
    {                                                                       \
        ubsan_write(message);                                               \
        ubsan_abort_with(abort_message);                                    \
    }

#define UBSAN_PAIR(name, message, abort_message)                            \
    UBSAN_HANDLER(name, message)                                            \
    UBSAN_ABORT_HANDLER(name, message, abort_message)

UBSAN_PAIR(type_mismatch, "ubsan: type-mismatch\n", "ubsan: type-mismatch")
UBSAN_PAIR(alignment_assumption, "ubsan: alignment-assumption\n", "ubsan: alignment-assumption")
UBSAN_PAIR(add_overflow, "ubsan: add-overflow\n", "ubsan: add-overflow")
UBSAN_PAIR(sub_overflow, "ubsan: sub-overflow\n", "ubsan: sub-overflow")
UBSAN_PAIR(mul_overflow, "ubsan: mul-overflow\n", "ubsan: mul-overflow")
UBSAN_PAIR(negate_overflow, "ubsan: negate-overflow\n", "ubsan: negate-overflow")
UBSAN_PAIR(divrem_overflow, "ubsan: divrem-overflow\n", "ubsan: divrem-overflow")
UBSAN_PAIR(shift_out_of_bounds, "ubsan: shift-out-of-bounds\n", "ubsan: shift-out-of-bounds")
UBSAN_PAIR(out_of_bounds, "ubsan: out-of-bounds\n", "ubsan: out-of-bounds")
UBSAN_PAIR(vla_bound_not_positive, "ubsan: vla-bound-not-positive\n", "ubsan: vla-bound-not-positive")
UBSAN_PAIR(float_cast_overflow, "ubsan: float-cast-overflow\n", "ubsan: float-cast-overflow")
UBSAN_PAIR(load_invalid_value, "ubsan: load-invalid-value\n", "ubsan: load-invalid-value")
UBSAN_PAIR(invalid_builtin, "ubsan: invalid-builtin\n", "ubsan: invalid-builtin")
UBSAN_PAIR(function_type_mismatch, "ubsan: function-type-mismatch\n", "ubsan: function-type-mismatch")
UBSAN_PAIR(implicit_conversion, "ubsan: implicit-conversion\n", "ubsan: implicit-conversion")
UBSAN_PAIR(nonnull_arg, "ubsan: nonnull-arg\n", "ubsan: nonnull-arg")
UBSAN_PAIR(nonnull_return, "ubsan: nonnull-return\n", "ubsan: nonnull-return")
UBSAN_PAIR(nullability_arg, "ubsan: nullability-arg\n", "ubsan: nullability-arg")
UBSAN_PAIR(nullability_return, "ubsan: nullability-return\n", "ubsan: nullability-return")
UBSAN_PAIR(pointer_overflow, "ubsan: pointer-overflow\n", "ubsan: pointer-overflow")
UBSAN_PAIR(cfi_check_fail, "ubsan: cfi-check-fail\n", "ubsan: cfi-check-fail")


UBSAN_HANDLER(builtin_unreachable, "ubsan: builtin-unreachable\n")
UBSAN_HANDLER(missing_return, "ubsan: missing-return\n")




static void *cfi_check_fail(void *diag)
{
    if (diag == NULL || *(const uint8_t *)diag < 5)
        abort();
    return diag;
}


EXPORT void *__cfi_check(uint64_t type_id, void *target, void *diag)
{
    void *expected;

    switch (type_id) {
    case 0xac3778598d51adbdULL:
        expected = (void *)&stTerminateSession;
        break;
    case 0xacf1de79d22706abULL:
        expected = (void *)&stStartSession;
        break;
    case 0x6cf58e448911dfd5ULL:
        expected = (void *)&stWaitForEvent;
        break;
    default:
        return cfi_check_fail(diag);
    }

    if (target == expected)
        return (void *)(uintptr_t)type_id;

    return cfi_check_fail(diag);
}
