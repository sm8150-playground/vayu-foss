/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MDM_DETECT_H
#define MDM_DETECT_H

#ifdef __cplusplus
extern "C" {
#endif


struct esoc_details {
    int status;
    char soc_name[32];
    char link[32];
    char dev[255];
    char port[255];
    char ramdump[255];
    char esoc_name[32];
    char link_info[32];
};

struct system_info {
    int n_mdm;
    int n_subsys;
    struct esoc_details mdm[4];
    struct esoc_details subsys[8];
};


__attribute__((visibility("default")))
extern const char *supported_modems[9];


__attribute__((visibility("default")))
int esoc_framework_supported(void);


__attribute__((visibility("default")))
int esoc_supported(const char *name);


__attribute__((visibility("default")))
char *get_soc_name(const char *name);
__attribute__((visibility("default")))
char *get_soc_link(const char *name);
__attribute__((visibility("default")))
char *get_soc_extra_link_info(const char *name);
__attribute__((visibility("default")))
char *get_soc_port(const char *name);
__attribute__((visibility("default")))
char *get_soc_ramdump_path(const char *name);


__attribute__((visibility("default")))
int get_esoc_details(struct esoc_details *out, const char *name);


__attribute__((visibility("default")))
int get_subsystem_info(struct esoc_details *out, char *name);


__attribute__((visibility("default")))
int get_system_info(struct system_info *out);


__attribute__((visibility("default")))
int get_peripheral_state(const char *name);

#ifdef __cplusplus
}
#endif

#endif
