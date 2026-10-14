/*
 * Copyright (C) 2026 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sensors_log.h"

namespace sensors_log {

namespace {


log_level g_level = LOG_LEVEL_INFO;
const char* g_tag = "sensors";
bool g_stderr_logging = false;

}

log_level get_level() {
    return g_level;
}

void set_level(log_level level) {
    g_level = level;
}

const char* get_tag() {
    return g_tag;
}

void set_tag(const char* tag) {
    g_tag = tag;
}

bool get_stderr_logging() {
    return g_stderr_logging;
}

void set_stderr_logging(bool enabled) {

    g_stderr_logging = enabled;
}

}
