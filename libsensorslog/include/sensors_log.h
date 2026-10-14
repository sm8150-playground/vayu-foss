/*
 * Copyright (C) 2026 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIB_SENSORSLOG_H
#define LIB_SENSORSLOG_H



namespace sensors_log {


enum log_level {
    LOG_LEVEL_NONE = 0,
    LOG_LEVEL_ERROR = 1,
    LOG_LEVEL_INFO = 2,
    LOG_LEVEL_DEBUG = 3,
    LOG_LEVEL_VERBOSE = 4,
};

__attribute__((visibility("default")))
log_level get_level();

__attribute__((visibility("default")))
void set_level(log_level level);

__attribute__((visibility("default")))
const char* get_tag();

__attribute__((visibility("default")))
void set_tag(const char* tag);

__attribute__((visibility("default")))
bool get_stderr_logging();

__attribute__((visibility("default")))
void set_stderr_logging(bool enabled);

}

#endif
