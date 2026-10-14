/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mdm_detect.h"

#define LOG_TAG "libmdmdetect"


extern int __android_log_print(int prio, const char *tag, const char *fmt, ...);

#define LOGE(...) __android_log_print(6, LOG_TAG, __VA_ARGS__)


#define SNPRINTF(dst, ...) snprintf((dst), 255, __VA_ARGS__)

#define ESOC_ROOT "/sys/bus/esoc/devices"
#define SSR_ROOT "/sys/bus/msm_subsys/devices"
#define RAMDUMP_ROOT "/data/vendor/tombstones"
#define ESOC_NAME_ATTR "esoc_name"
#define ESOC_LINK_ATTR "esoc_link"
#define ESOC_LINK_INFO_ATTR "esoc_link_info"
#define ESOC_PORT_DIR "esoc-dev"
#define SSR_NAME_ATTR "name"
#define SSR_STATE_ATTR "state"
#define NA "N/A"

__attribute__((visibility("default")))
const char *supported_modems[9] = {
    "QSC",
    "MDM9x15",
    "MDM9x25",
    "MDM9x35",
    "MDM9x45",
    "MDM9x55",
    "SDX50M",
    "SDX55M",
    "SDXPRAIRIE",
};


static int read_attribute(const char *path, char *buf, int size, int log_errors)
{
    int fd;
    ssize_t n;
    size_t len;

    if (path == NULL) {
        if (log_errors)
            LOGE("Invalid path string");
        return 1;
    }
    if (buf == NULL) {
        if (log_errors)
            LOGE("Invalid buf/buf size passed");
        return 1;
    }

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        if (log_errors)
            LOGE("Failed to open %s: %s", path, strerror(errno));
        return 1;
    }

    memset(buf, 0, size);
    n = read(fd, buf, size - 1);
    if (n < 0) {
        if (log_errors)
            LOGE("Failed to read %s: %s", path, strerror(errno));
        close(fd);
        return 1;
    }

    len = strlen(buf);
    if (len == 0) {
        if (log_errors)
            LOGE("Read 0 length string from : %s", path);
        close(fd);
        return 1;
    }
    if (buf[len - 1] == '\n')
        buf[len - 1] = '\0';

    close(fd);
    return 0;
}


static int read_soc_attribute(const char *name, const char *attr, char *buf,
                              int size, int log_errors)
{
    char path[255];

    memset(buf, 0, size);
    SNPRINTF(path, "%s/%s/%s", ESOC_ROOT, name, attr);
    return read_attribute(path, buf, size, log_errors);
}



char *get_soc_name(const char *name)
{
    char *soc = malloc(30);

    if (soc == NULL) {
        LOGE("Failed to allocate memory for soc name");
        return NULL;
    }
    if (read_soc_attribute(name, ESOC_NAME_ATTR, soc, 30, 1) != 0) {
        free(soc);
        return NULL;
    }
    return soc;
}

char *get_soc_link(const char *name)
{
    char *link = malloc(30);

    if (link == NULL) {
        LOGE("Failed to allocate memory for soc link");
        return NULL;
    }
    if (read_soc_attribute(name, ESOC_LINK_ATTR, link, 30, 1) != 0) {
        free(link);
        return NULL;
    }
    return link;
}

char *get_soc_extra_link_info(const char *name)
{
    char *info = malloc(60);

    if (info == NULL) {
        LOGE("Failed to allocate memory for soc link");
        return NULL;
    }

    if (read_soc_attribute(name, ESOC_LINK_INFO_ATTR, info, 60, 0) != 0) {
        free(info);
        return NULL;
    }
    return info;
}

char *get_soc_port(const char *name)
{
    char path[255];
    char *port = malloc(255);
    DIR *dir;
    struct dirent *entry;

    if (port == NULL) {
        LOGE("Failed to allocate memory for soc port name");
        return NULL;
    }

    SNPRINTF(path, "%s/%s/%s", ESOC_ROOT, name, ESOC_PORT_DIR);
    dir = opendir(path);
    if (dir == NULL) {
        LOGE("Failed to open port node directory: %s", strerror(errno));
        free(port);
        return NULL;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;
        SNPRINTF(port, "/dev/%s", entry->d_name);
        closedir(dir);
        return port;
    }

    LOGE("Failed to get the name of the soc port: %s", strerror(errno));
    closedir(dir);
    free(port);
    return NULL;
}

char *get_soc_ramdump_path(const char *name)
{
    char *path = malloc(255);
    char *soc;

    if (path == NULL) {
        LOGE("Failed to allocate memory for ramdump path string");
        return NULL;
    }

    soc = get_soc_name(name);
    if (soc == NULL) {
        LOGE("Failed to get ramdump path");
        free(path);
        return NULL;
    }

    SNPRINTF(path, "%s/%s/", RAMDUMP_ROOT, soc);
    free(soc);
    return path;
}



int esoc_framework_supported(void)
{
    struct stat st;


    return stat(ESOC_ROOT, &st) != 0;
}

int esoc_supported(const char *name)
{
    char *soc;
    int i;

    if (name == NULL) {
        LOGE("soc_name passed as NULL");
        return 1;
    }

    soc = get_soc_name(name);
    if (soc == NULL) {
        LOGE("Failed to read soc_name");
        return 1;
    }

    for (i = 0; i < 9; i++) {
        if (strncmp(soc, supported_modems[i], 30) == 0) {
            free(soc);
            return 0;
        }
    }

    free(soc);
    return 1;
}



int get_esoc_details(struct esoc_details *out, const char *name)
{
    char *value;

    out->status = 0;
    strlcpy(out->esoc_name, name, sizeof(out->esoc_name));

    value = get_soc_name(name);
    if (value == NULL) {
        LOGE("Failed to get soc name for modem");
        return 1;
    }
    strlcpy(out->soc_name, value, sizeof(out->soc_name));
    free(value);

    value = get_soc_port(name);
    if (value == NULL) {
        LOGE("Failed to get soc port for modem");
        return 1;
    }
    strlcpy(out->port, value, sizeof(out->port));
    free(value);

    value = get_soc_link(name);
    if (value == NULL) {
        LOGE("Failed to get mdm link for modem");
        return 1;
    }
    strlcpy(out->link, value, sizeof(out->link));
    free(value);


    value = get_soc_extra_link_info(name);
    if (value != NULL) {
        strlcpy(out->link_info, value, sizeof(out->link_info));
        free(value);
    } else {
        strlcpy(out->link_info, NA, sizeof(out->link_info));
    }

    value = get_soc_ramdump_path(name);
    if (value == NULL) {
        LOGE("Failed to get ram dump path for modem");
        return 1;
    }
    strlcpy(out->ramdump, value, sizeof(out->ramdump));
    free(value);

    SNPRINTF(out->dev, "/dev/subsys_%s", name);
    return 0;
}

int get_subsystem_info(struct esoc_details *out, char *name)
{
    size_t len;

    if (out == NULL || name == NULL) {
        LOGE("Invalid argument passed to get_subsystem_info");
        return 1;
    }


    len = strlen(name);
    if (len != 0 && name[len - 1] == '\n')
        name[len - 1] = '\0';

    if (strncmp(name, "slpi", 32) == 0) {
        out->status = 2;
        strlcpy(out->link, "n/a", sizeof(out->link));
        SNPRINTF(out->ramdump, NA);
    } else if (strncmp(name, "modem", 32) == 0) {
        strlcpy(out->link, "SMD", sizeof(out->link));
        SNPRINTF(out->ramdump, "%s/%s", RAMDUMP_ROOT, name);
        out->status = 1;
    } else if (strncmp(name, "spss", 32) == 0) {
        strlcpy(out->link, "SMD", sizeof(out->link));
        SNPRINTF(out->ramdump, "%s/%s", RAMDUMP_ROOT, name);
        out->status = 3;
    } else {

        return 3;
    }

    strlcpy(out->esoc_name, NA, sizeof(out->esoc_name));
    strlcpy(out->port, NA, sizeof(out->port));
    strlcpy(out->soc_name, name, sizeof(out->soc_name));
    SNPRINTF(out->dev, "/dev/subsys_%s", name);
    return 0;
}



int get_system_info(struct system_info *out)
{
    struct stat st;
    char path[255];
    char state[32];
    DIR *dir;
    struct dirent *entry;
    int index;
    int rc;

    if (out == NULL) {
        LOGE("dev info paseed in as NULL");
        return 1;
    }


    out->n_mdm = 0;
    out->n_subsys = 0;

    if (stat(ESOC_ROOT, &st) == 0) {
        dir = opendir(ESOC_ROOT);
        if (dir == NULL) {
            LOGE("Failed to open ESOC dir");
            return 1;
        }

        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.')
                continue;
            if (esoc_supported(entry->d_name) != 0)
                continue;

            index = out->n_mdm;
            if (index >= 4) {
                LOGE("Too many mdm's detected");
                closedir(dir);
                return 1;
            }
            if (get_esoc_details(&out->mdm[index], entry->d_name) != 0) {
                LOGE("Failed to get ESOC info for %s", entry->d_name);
                closedir(dir);
                return 1;
            }
            out->n_mdm = index + 1;
        }
        closedir(dir);
    }

    dir = opendir(SSR_ROOT);
    if (dir == NULL) {
        LOGE("Failed to open SSR root dir: %s", strerror(errno));
        return 1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;


        SNPRINTF(path, "%s/%s/%s", SSR_ROOT, entry->d_name, SSR_NAME_ATTR);
        if (read_attribute(path, state, sizeof(state), 1) != 0)
            continue;

        if (strncmp(state, "esoc", 4) == 0)
            continue;

        if (strncmp(state, "modem", 5) == 0) {
            index = out->n_mdm;
            if (index >= 4) {
                LOGE("Too many mdm's detected");
                closedir(dir);
                return 1;
            }
            if (get_subsystem_info(&out->mdm[index], state) != 0) {
                LOGE("Failed to get internal modem info");
                closedir(dir);
                return 1;
            }
            out->n_mdm = index + 1;
            continue;
        }

        index = out->n_subsys;
        if (index >= 8) {
            LOGE("Too many additional subsystems..");
            closedir(dir);
            return 1;
        }
        rc = get_subsystem_info(&out->subsys[index], state);
        if (rc == 3)
            continue;
        if (rc == 1) {
            LOGE("Failed to get information on : %s", state);
            closedir(dir);
            return 1;
        }
        out->n_subsys = index + 1;
    }

    closedir(dir);
    return 0;
}


static int state_is_offline(const char *state)
{
    return memcmp(state, "OFFLINE", 7) == 0;
}

int get_peripheral_state(const char *name)
{
    struct system_info info;
    char name_path[255];
    char state_path[255];
    char node_name[32];
    char state[30];
    DIR *ssr = NULL;
    DIR *esoc = NULL;
    char *soc = NULL;
    const char *lookup = name;
    struct dirent *entry;
    int i;

    memset(&info, 0, sizeof(info));

    if (name == NULL)
        return 1;
    if (get_system_info(&info) != 0)
        return 1;

    for (i = 0; i < info.n_mdm; i++) {
        if (strncmp(info.mdm[i].soc_name, name, 32) == 0)
            break;
    }
    if (i == info.n_mdm) {
        LOGE("Unrecognised name passed to get_peripheral_state");
        return 1;
    }


    if (strncmp(info.mdm[i].esoc_name, "esoc", 4) == 0) {
        esoc = opendir(ESOC_ROOT);
        if (esoc == NULL) {
            LOGE("Failed to open ESOC dir");
            return 1;
        }

        for (entry = readdir(esoc); entry != NULL; entry = readdir(esoc)) {
            if (entry->d_name[0] == '.')
                continue;

            free(soc);
            soc = get_soc_name(entry->d_name);
            if (soc == NULL) {
                LOGE("Failed to get name for %s", entry->d_name);
                closedir(esoc);
                return 1;
            }
            if (strncmp(soc, name, 32) != 0)
                continue;

            lookup = entry->d_name;
            break;
        }

        if (entry == NULL) {
            LOGE("Failed to locate registered ssr name for %s", lookup);
            closedir(esoc);
            free(soc);
            return 1;
        }
    }

    ssr = opendir(SSR_ROOT);
    if (ssr == NULL) {
        LOGE("Failed to open ssr bus directory");
        if (esoc != NULL)
            closedir(esoc);
        free(soc);
        return 1;
    }

    for (entry = readdir(ssr); entry != NULL; entry = readdir(ssr)) {
        if (entry->d_name[0] == '.')
            continue;

        SNPRINTF(name_path, "%s/%s/%s", SSR_ROOT, entry->d_name,
                 SSR_NAME_ATTR);
        if (read_attribute(name_path, node_name, sizeof(node_name), 1) != 0) {
            closedir(ssr);
            if (esoc != NULL)
                closedir(esoc);
            free(soc);
            return 1;
        }
        if (strncmp(node_name, lookup, 32) != 0)
            continue;

        SNPRINTF(state_path, "%s/%s/%s", SSR_ROOT, entry->d_name,
                 SSR_STATE_ATTR);
        if (read_attribute(state_path, state, sizeof(state), 1) != 0) {
            closedir(ssr);
            if (esoc != NULL)
                closedir(esoc);
            free(soc);
            return 1;
        }

        closedir(ssr);
        if (esoc != NULL)
            closedir(esoc);
        free(soc);
        return state_is_offline(state) ? 4 : 3;
    }

    LOGE("Failed to find a ssr entry for %s", lookup);
    closedir(ssr);
    if (esoc != NULL)
        closedir(esoc);
    free(soc);
    return 1;
}
