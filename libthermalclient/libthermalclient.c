/*
 * Copyright (C) 2026 The Android Open Source Project
 */



#define LOG_TAG "Thermal-Lib"

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include <log/log.h>





#define SOCK_DATA_SIZE      15584
#define ANAME_LEN           20
#define CFG_DESC_LEN        32
#define MAX_GROUPS          12
#define MAX_SLOTS           32
#define SLOT_NAME_LEN       32
#define MAX_INSTANCES       0x7F
#define STEPWISE_MAX        10
#define LIMIT_MAX           30

#define OFF_TYPE            0x000
#define OFF_NAME            0x004
#define OFF_DESC            0x018
#define OFF_MASK            0x038
#define OFF_PAYLOAD         0x044
#define OFF_COUNT           0x018
#define OFF_POLLING_DELAY   0x3CD8
#define OFF_PASSIVE_DELAY   0x3CDC


#define MON_GROUP0          0x050
#define MON_STRIDE          0x48C
#define MON_SLOT            0x24
#define MON_SLOT_VAL        0x20
#define MON_GROUP_COUNT     0x480
#define MON_GROUP_R1        0x484
#define MON_GROUP_R2        0x488


#define DEV_GROUP0          0x050
#define DEV_STRIDE          0x50C
#define DEV_SLOT            0x28
#define DEV_SLOT_UPPER      0x20
#define DEV_SLOT_LOWER      0x24
#define DEV_GROUP_COUNT     0x500
#define DEV_GROUP_R1        0x504
#define DEV_GROUP_R2        0x508


#define MON_SLOT_NAME       0x00C
#define MON_SLOT_INFO       0x02C





enum {
    CLIENT_SPKR = 0,
    CLIENT_OVERRIDE,
    CLIENT_CONFIG_SET,
    CLIENT_CONFIG_QUERY,
    CLIENT_CAMERA_BW,
    CLIENT_DISPLAY_BW,
    CLIENT_NUM,
};

static const struct client_socket {
    const char *name;
    const char *socket_path;
} client_sockets[CLIENT_NUM] = {
    { "spkr",         "/dev/socket/thermal-recv-passive-client" },
    { "override",     "/dev/socket/thermal-recv-client" },
    { "config_set",   "/dev/socket/thermal-recv-client" },
    { "config_query", "/dev/socket/thermal-recv-client" },
    { "camera_bw",    "/dev/socket/thermal-recv-passive-client" },
    { "display_bw",   "/dev/socket/thermal-recv-passive-client" },
};

#define SOCKET_SEND_CLIENT "/dev/socket/thermal-send-client"
#define SOCKET_SEND_RULE   "/dev/socket/thermal-send-rule"


#define ALGO_MONITOR    "monitor"
#define ALGO_PID        "pid"
#define ALGO_SS         "ss"
#define ALGO_TB         "tb"
#define ALGO_STEP_WISE  "step_wise"
#define ALGO_ALL        "all"
#define ALGO_REPORT     "report"

#define CLIENT_NAME_CONFIG_SET   "config_set"
#define CLIENT_NAME_CONFIG_QUERY "config_query"


#define ALGO_ID_VALID_MASK  0x18F


enum {
    FIELD_INT = 0,
    FIELD_STRING,
    FIELD_THERM_ARRAY,
    FIELD_DEVICE_ARRAY,
    FIELD_LIMITS,
};





struct target_client_field {
    char *field_name;
    int field_type;
    int num_vals;
    void *vals;
};

struct config_instance {
    char *cfg_desc;
    char *algo_type;
    unsigned int fields_mask;
    int num_fields;
    struct target_client_field *fields;
};


struct callback_node {
    int handle;
    int pad;
    const char *name;
    int (*cb)(long, void *, void *);
    void *data;
    void *arg;
    struct callback_node *next;
};





static int add_int_to_field(const void *msg, struct target_client_field *field);
static int add_string_to_field(const void *msg, struct target_client_field *field);
static int add_thresholds_to_field(const void *msg, struct target_client_field *field);
static int add_thresholds_clr_to_field(const void *msg, struct target_client_field *field);
static int add_actions_to_field(const void *msg, struct target_client_field *field);
static int add_action_info_to_field(const void *msg, struct target_client_field *field);
static int add_set_points_to_field(const void *msg, struct target_client_field *field);
static int add_set_points_clr_to_field(const void *msg, struct target_client_field *field);
static int add_cdev_array_to_field(const void *msg, struct target_client_field *field);
static int add_upper_limits_to_field(const void *msg, struct target_client_field *field);
static int add_lower_limits_to_field(const void *msg, struct target_client_field *field);

struct field_desc {
    const char *name;
    int type;
    int flag;
    int off;
    int extra;
    int (*add)(const void *msg, struct target_client_field *field);
};


static const struct field_desc monitor_fields[] = {
    { "disable",        FIELD_INT,          0x01, 0x3C, 0, add_int_to_field },
    { "sampling",       FIELD_INT,          0x02, 0x40, 0, add_int_to_field },
    { "thresholds",     FIELD_THERM_ARRAY,  0x04, 0x44, 0, add_thresholds_to_field },
    { "thresholds_clr", FIELD_THERM_ARRAY,  0x08, 0x44, 4, add_thresholds_clr_to_field },
    { "actions",        FIELD_DEVICE_ARRAY, 0x00, 0x44, 8, add_actions_to_field },
    { "action_info",    FIELD_LIMITS,       0x10, 0x44, 8, add_action_info_to_field },
};


static const struct field_desc pid_fields[] = {
    { "disable",       FIELD_INT,          0x01, 0x3C, 0, add_int_to_field },
    { "sampling",      FIELD_INT,          0x02, 0x40, 0, add_int_to_field },

    { "set_point",     FIELD_INT,          0x04, 0x44, 0, add_int_to_field },
    { "set_point_clr", FIELD_INT,          0x08, 0x48, 0, add_int_to_field },
    { "device",        FIELD_STRING,       0x00, 0x4C, 0, add_string_to_field },
};


static const struct field_desc override_fields[] = {
    { "disable",       FIELD_INT,          0x01, 0x3C,   0, add_int_to_field },
    { "polling_delay", FIELD_INT,          0x02, 0x3CD8, 0, add_int_to_field },
    { "passive_delay", FIELD_INT,          0x40, 0x3CDC, 0, add_int_to_field },
    { "set_temp",      FIELD_THERM_ARRAY,  0x04, 0x44,   0, add_set_points_to_field },
    { "clr_temp",      FIELD_THERM_ARRAY,  0x08, 0x44,   4, add_set_points_clr_to_field },
    { "device",        FIELD_DEVICE_ARRAY, 0x00, 0x44,   8, add_cdev_array_to_field },
    { "upper_limit",   FIELD_LIMITS,       0x10, 0x44,   8, add_upper_limits_to_field },
    { "lower_limit",   FIELD_LIMITS,       0x20, 0x44,   8, add_lower_limits_to_field },
};

#define MONITOR_NFIELDS  ((int)(sizeof(monitor_fields) / sizeof(monitor_fields[0])))
#define PID_NFIELDS      ((int)(sizeof(pid_fields) / sizeof(pid_fields[0])))
#define OVERRIDE_NFIELDS ((int)(sizeof(override_fields) / sizeof(override_fields[0])))





void *list_head;
static unsigned char listen_stop;
static pthread_mutex_t callback_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;
static unsigned char listen_running;
static pthread_t listen_thread;
static struct sockaddr_un listen_addr;
static int listen_pipe[2] = { -1, -1 };
static int registered_handle_mask;

static unsigned char bw_request[SOCK_DATA_SIZE];
static pthread_mutex_t bw_mutex = PTHREAD_MUTEX_INITIALIZER;
static unsigned char bw_cancel;
static pthread_t bw_thread;
static int bw_pipe[2] = { -1, -1 };
static unsigned char bw_running;
static pthread_mutex_t bw_request_mutex = PTHREAD_MUTEX_INITIALIZER;

static unsigned char client_request[SOCK_DATA_SIZE];





static void *at(void *base, size_t off)
{
    return (unsigned char *)base + off;
}

static const void *cat(const void *base, size_t off)
{
    return (const unsigned char *)base + off;
}

static void store_int(void *base, size_t off, int val)
{
    memcpy((unsigned char *)base + off, &val, sizeof(val));
}

static int load_int(const void *base, size_t off)
{
    int val;

    memcpy(&val, (const unsigned char *)base + off, sizeof(val));
    return val;
}





static int connect_to_socket(const char *name, struct sockaddr_un *addr)
{
    int idx;
    int sock;

    if (!name) {
        ALOGE("Thermal-Lib-Client:%s: unexpected NULL", "connect_to_socket");
        return -1;
    }
    for (idx = 0; idx < CLIENT_NUM; idx++) {
        if (!strcmp(name, client_sockets[idx].name))
            break;
    }
    if (idx == CLIENT_NUM) {
        ALOGE("Thermal-Lib-Client:%s is not in supported thermal client list", name);
        return -1;
    }

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0)
        return -1;

    memset(addr, 0, sizeof(struct sockaddr_un) - 2);
    strlcpy(addr->sun_path, client_sockets[idx].socket_path,
            sizeof(struct sockaddr_un) - 2);
    addr->sun_family = AF_UNIX;
    if (connect(sock, (struct sockaddr *)addr,
                strlen(client_sockets[idx].socket_path) + 2)) {
        close(sock);
        return -1;
    }
    return sock;
}


static int name_to_client_index(const char *name)
{
    int idx;

    for (idx = 0; idx < CLIENT_NUM; idx++) {
        if (!strncmp(name, client_sockets[idx].name, ANAME_LEN))
            return idx;
    }
    ALOGE("Thermal-Lib-Client:%s is not in supported thermal client list", name);
    return -1;
}


static int is_reply_client_name(const char *name)
{
    return !strncmp(name, "camera", ANAME_LEN) ||
           !strncmp(name, "camcorder", ANAME_LEN) ||
           !strncmp(name, "spkr", ANAME_LEN) ||
           !strncmp(name, CLIENT_NAME_CONFIG_QUERY, ANAME_LEN);
}

int send_to_fd(int fd, const void *buf, int len)
{
    int sent = 0;

    while (sent < len) {
        ssize_t ret = sendto(fd, (const char *)buf + sent, len - sent,
                             MSG_NOSIGNAL, NULL, 0);

        if (ret < 0)
            return -errno;
        sent += ret;
    }
    return len;
}

int recv_from_fd(int fd, void *buf, int len)
{
    int got = 0;

    while (got < len) {
        ssize_t ret = recvfrom(fd, (char *)buf + got, len - got, 0, NULL, NULL);

        if (ret <= 0)
            return -errno;
        got += ret;
    }
    return len;
}





static const struct {
    const char *name;
    int val;
} algo_names[] = {
    { ALGO_MONITOR,   0 },
    { ALGO_PID,       1 },
    { ALGO_SS,        2 },
    { ALGO_TB,        3 },
    { ALGO_STEP_WISE, 7 },
    { ALGO_ALL,       8 },
};

#define ALGO_NAME_COUNT ((int)(sizeof(algo_names) / sizeof(algo_names[0])))


static const unsigned int algo_id_to_index[9] = { 0, 1, 2, 3, 0, 0, 0, 4, 5 };


static int check_for_algo_type(int algo, char *name)
{
    int i;

    if (algo >= 0) {
        if (algo > 8)
            return -1;
        if (!((ALGO_ID_VALID_MASK >> algo) & 1))
            return -1;
        strlcpy(name, algo_names[algo_id_to_index[algo]].name, 10);
        return algo_names[algo_id_to_index[algo]].val;
    }
    if (!*name)
        return -1;
    for (i = 0; i < ALGO_NAME_COUNT; i++) {
        if (!strcmp(name, algo_names[i].name))
            return algo_names[i].val;
    }
    return -1;
}





struct callback_node *get_callback_node_from_list(struct callback_node *node,
                                                  const char *name)
{
    while (node && strncmp(node->name, name, ANAME_LEN))
        node = node->next;
    return node;
}

int remove_from_list(int handle)
{
    struct callback_node *node;
    struct callback_node *prev;

    if ((unsigned)(handle - 1) > 30u ||
        !(registered_handle_mask & (1u << handle)))
        return -EINVAL;

    node = (struct callback_node *)list_head;
    if (!node) {
        registered_handle_mask &= ~(1u << handle);
        return -EINVAL;
    }
    for (prev = node; node && node->handle != handle; node = node->next)
        prev = node;
    if (!node) {
        registered_handle_mask &= ~(1u << handle);
        return -EINVAL;
    }
    if (node == (struct callback_node *)list_head)
        list_head = node->next;
    else
        prev->next = node->next;
    registered_handle_mask &= ~(1u << handle);
    free(node);
    return 0;
}

int add_to_list(const char *name, int (*cb)(long, void *, void *), void *data)
{
    struct callback_node *node;
    int handle;

    if (!name || !cb)
        return 0;
    for (handle = 1; handle < 32; handle++) {
        if (!(registered_handle_mask & (1u << handle)))
            break;
    }
    if (handle == 32)
        return 0;
    registered_handle_mask |= 1u << handle;

    node = (struct callback_node *)malloc(sizeof(*node));
    if (!node)
        return 0;
    node->handle = handle;
    node->pad = 0;
    node->name = name;
    node->cb = cb;
    node->data = data;
    node->arg = NULL;
    node->next = (struct callback_node *)list_head;
    list_head = node;
    return handle;
}





static int add_int_to_field(const void *msg, struct target_client_field *field)
{
    int *p;

    if (!msg || !field)
        return -1;
    p = (int *)malloc(sizeof(int));
    if (!p) {
        ALOGE("%s: malloc failed", "add_int_to_field");
        return -1;
    }
    *p = load_int(msg, 0);
    field->vals = p;
    field->num_vals = 1;
    return 0;
}

static int add_string_to_field(const void *msg, struct target_client_field *field)
{
    char *p;

    if (!msg || !field)
        return -1;
    p = (char *)malloc(CFG_DESC_LEN);
    if (!p) {
        ALOGE("%s: malloc failed", "add_string_to_field");
        return -1;
    }
    strlcpy(p, (const char *)msg, CFG_DESC_LEN);
    field->vals = p;
    field->num_vals = 1;
    return 0;
}

static int add_thresholds_to_field(const void *msg, struct target_client_field *field)
{
    int count;
    int i;
    int *p;

    if (!msg || !field)
        return -1;
    count = load_int(msg, 0);
    field->num_vals = count;
    p = (int *)malloc(sizeof(int) * count);
    if (!p) {
        ALOGE("%s: malloc failed", "add_thresholds_to_field");
        return -1;
    }
    for (i = 0; i < count; i++)
        p[i] = load_int(msg, 4 + (size_t)i * MON_STRIDE);
    field->vals = p;
    return 0;
}

static int add_thresholds_clr_to_field(const void *msg, struct target_client_field *field)
{
    int count;
    int i;
    int *p;

    if (!msg || !field)
        return -1;
    count = load_int(msg, 0);
    field->num_vals = count;
    p = (int *)malloc(sizeof(int) * count);
    if (!p) {
        ALOGE("%s: malloc failed", "add_thresholds_clr_to_field");
        return -1;
    }
    for (i = 0; i < count; i++)
        p[i] = load_int(msg, 8 + (size_t)i * MON_STRIDE);
    field->vals = p;
    return 0;
}


static int add_actions_to_field(const void *msg, struct target_client_field *field)
{
    int count;
    int i;
    int j;
    int total = 0;
    char **p;

    if (!msg || !field)
        return -1;
    count = load_int(msg, 0);
    field->num_vals = count;
    p = (char **)malloc(sizeof(char *) * count);
    if (!p) {
        ALOGE("%s: malloc failed", "add_actions_to_field");
        return -1;
    }
    for (i = 0; i < count; i++) {
        int nacts = load_int(msg, 4 + MON_GROUP_R2 + (size_t)i * MON_STRIDE);
        int size = (nacts > 1 ? nacts : 1) * (SLOT_NAME_LEN + 1);

        if (!nacts)
            size = 0;
        total += size;
        p[i] = (char *)malloc((size_t)total);
        if (!p[i]) {
            ALOGE("%s: malloc failed", "add_actions_to_field");
            while (i-- > 0)
                free(p[i]);
            free(p);
            return -1;
        }
        memset(p[i], 0, (size_t)total);
        for (j = 0; j < nacts; j++) {
            if (j && strlcat(p[i], "+", (size_t)total) >= (size_t)total)
                break;
            if (strlcat(p[i],
                        (const char *)cat(msg, MON_SLOT_NAME + (size_t)i * MON_STRIDE +
                                          (size_t)j * MON_SLOT),
                        (size_t)total) >= (size_t)total)
                break;
        }
    }
    field->vals = p;
    return 0;
}


static int add_action_info_to_field(const void *msg, struct target_client_field *field)
{
    int count;
    int i;
    int j;
    int *p;

    if (!msg || !field)
        return -1;
    count = load_int(msg, 0);
    field->num_vals = count;
    p = (int *)malloc(sizeof(int) * 33 * count);
    if (!p) {
        ALOGE("%s: malloc failed", "add_action_info_to_field");
        return -1;
    }
    for (i = 0; i < count; i++) {
        int n = load_int(msg, 4 + MON_GROUP_R2 + (size_t)i * MON_STRIDE);

        p[i * 33 + 32] = n;
        for (j = 0; j < n; j++)
            p[i * 33 + j] = load_int(msg, MON_SLOT_INFO + (size_t)i * MON_STRIDE +
                                     (size_t)j * MON_SLOT);
    }
    field->vals = p;
    return 0;
}

static int add_set_points_to_field(const void *msg, struct target_client_field *field)
{
    int count;
    int i;
    int *p;

    if (!msg || !field)
        return -1;
    count = load_int(msg, 0);
    field->num_vals = count;
    p = (int *)calloc(count ? count : 1, sizeof(int));
    if (!p) {
        ALOGE("%s:calloc failed", "add_common_stepwise_entry_to_field");
        return -1;
    }
    for (i = 0; i < count; i++)
        p[i] = load_int(msg, 4 + (size_t)i * DEV_STRIDE);
    field->vals = p;
    return 0;
}

static int add_set_points_clr_to_field(const void *msg, struct target_client_field *field)
{
    int count;
    int i;
    int *p;

    if (!msg || !field)
        return -1;
    count = load_int(msg, 0);
    field->num_vals = count;
    p = (int *)calloc(count ? count : 1, sizeof(int));
    if (!p) {
        ALOGE("%s:calloc failed", "add_common_stepwise_entry_to_field");
        return -1;
    }
    for (i = 0; i < count; i++)
        p[i] = load_int(msg, 8 + (size_t)i * DEV_STRIDE);
    field->vals = p;
    return 0;
}


static int add_cdev_array_to_field(const void *msg, struct target_client_field *field)
{
    int count;
    int i;
    int j;
    char **p;

    if (!msg || !field)
        return -1;
    count = load_int(msg, 0);
    field->num_vals = count;
    p = (char **)calloc(count ? count : 1, sizeof(char *));
    if (!p) {
        ALOGE("%s: calloc failed", "add_cdev_array_to_field");
        return -1;
    }
    for (i = 0; i < count; i++) {
        int ndev = load_int(msg, 4 + DEV_GROUP_R2 + (size_t)i * DEV_STRIDE);
        int size = ndev * (SLOT_NAME_LEN + 1);

        p[i] = (char *)calloc(size ? size : 1, 1);
        if (!p[i]) {
            ALOGE("%s: calloc failed", "add_cdev_array_to_field");
            while (i-- > 0)
                free(p[i]);
            free(p);
            return -1;
        }
        for (j = 0; j < ndev; j++) {
            if (j && strlcat(p[i], "+", (size_t)size) >= (size_t)size)
                break;
            if (strlcat(p[i],
                        (const char *)cat(msg, MON_SLOT_NAME + (size_t)i * DEV_STRIDE +
                                          (size_t)j * DEV_SLOT),
                        (size_t)size) >= (size_t)size)
                break;
        }
    }
    field->vals = p;
    return 0;
}

static int add_upper_limits_to_field(const void *msg, struct target_client_field *field)
{
    int count;
    int i;
    int j;
    int *p;

    if (!msg || !field)
        return -1;
    count = load_int(msg, 0);
    field->num_vals = count;
    p = (int *)calloc(count ? count : 1, 33 * sizeof(int));
    if (!p) {
        ALOGE("%s: calloc failed", "add_common_stepwise_entry_to_field");
        return -1;
    }
    for (i = 0; i < count; i++) {
        int n = load_int(msg, DEV_STRIDE + (size_t)i * DEV_STRIDE);

        p[i * 33 + 32] = n;
        for (j = 0; j < n; j++) {
            p[i * 33 + j] = load_int(msg, MON_SLOT_INFO + (size_t)i * DEV_STRIDE +
                                     (size_t)j * DEV_SLOT);
            if (j > LIMIT_MAX)
                break;
        }
    }
    field->vals = p;
    return 0;
}

static int add_lower_limits_to_field(const void *msg, struct target_client_field *field)
{
    int count;
    int i;
    int j;
    int *p;

    if (!msg || !field)
        return -1;
    count = load_int(msg, 0);
    field->num_vals = count;
    p = (int *)calloc(count ? count : 1, 33 * sizeof(int));
    if (!p) {
        ALOGE("%s:calloc failed", "add_common_stepwise_entry_to_field");
        return -1;
    }
    for (i = 0; i < count; i++) {
        int n = load_int(msg, DEV_STRIDE + (size_t)i * DEV_STRIDE);

        p[i * 33 + 32] = n;
        for (j = 0; j < n; j++) {
            p[i * 33 + j] = load_int(msg, MON_SLOT_INFO + 4 + (size_t)i * DEV_STRIDE +
                                     (size_t)j * DEV_SLOT);
            if (j > LIMIT_MAX)
                break;
        }
    }
    field->vals = p;
    return 0;
}





static int add_settings_to_client_config(struct config_instance *inst,
                                         const void *msg,
                                         const struct field_desc *fields,
                                         int num_fields)
{
    struct target_client_field *entries;
    int i;

    if (!inst || !msg || !fields)
        return -1;
    inst->num_fields = num_fields;
    entries = (struct target_client_field *)malloc(sizeof(*entries) * num_fields);
    if (!entries) {
        ALOGE("%s: malloc failed for fields", "add_settings_to_client_config");
        return -1;
    }
    inst->fields = entries;
    for (i = 0; i < num_fields; i++) {
        struct target_client_field *e = &entries[i];

        e->field_name = (char *)malloc(ANAME_LEN);
        if (!e->field_name) {
            ALOGE("%s: malloc failed for field_name", "add_settings_to_client_config");
            while (i-- > 0)
                free(entries[i].field_name);
            free(entries);
            return -1;
        }
        strlcpy(e->field_name, fields[i].name, ANAME_LEN);
        e->field_type = fields[i].type;
        if (fields[i].add(cat(msg, fields[i].off), e)) {
            free(e->field_name);
            while (i-- > 0)
                free(entries[i].field_name);
            free(entries);
            return -1;
        }
    }
    return 0;
}





static void store_values(void *msg, const struct field_desc *f,
                         const struct target_client_field *e, int stride, int max_index)
{
    const int *vals = (const int *)e->vals;
    int i;

    store_int(msg, f->off, e->num_vals);
    for (i = 0; i < e->num_vals; i++) {
        store_int(msg, f->off + 4 + f->extra + (size_t)i * stride, vals[i]);
        if (max_index >= 0 && i > max_index)
            break;
    }
}


static void store_limits(void *msg, const struct field_desc *f,
                         const struct target_client_field *e, int base,
                         int count_extra, int stride, int slot)
{
    const int *vals = (const int *)e->vals;
    int i;
    int j;

    store_int(msg, f->off, e->num_vals);
    for (i = 0; i < e->num_vals; i++) {
        int n = vals[i * 33 + 32];

        store_int(msg, f->off + 4 + count_extra + (size_t)i * stride, n);
        for (j = 0; j < n; j++)
            store_int(msg, base + f->off + f->extra + (size_t)i * stride +
                      (size_t)j * slot, vals[i * 33 + j]);
    }
}


static int add_algo_specific_data_to_sock_data(struct config_instance *inst,
                                               void *msg,
                                               const struct field_desc *fields,
                                               int num_fields)
{
    int not_applied = 0;
    int i;
    int j;

    if (!inst || !msg || !fields)
        return -1;
    if (!inst->fields_mask)
        return -1;
    if (!inst->num_fields || !num_fields)
        return -1;

    for (i = 0; i < inst->num_fields; i++) {
        struct target_client_field *e = &inst->fields[i];

        for (j = 0; j < num_fields; j++) {
            const struct field_desc *f = &fields[j];

            if (strcmp(e->field_name, f->name))
                continue;
            if (e->field_type != f->type)
                continue;
            if (!f->flag || !(inst->fields_mask & f->flag)) {
                not_applied++;
                break;
            }
            switch (e->field_type) {
            case FIELD_INT:
                store_int(msg, f->off, *(const int *)e->vals);
                break;
            case FIELD_STRING:
                strlcpy((char *)at(msg, f->off), (const char *)e->vals, CFG_DESC_LEN);
                break;
            case FIELD_THERM_ARRAY:
                if (!strcmp(e->field_name, "set_temp") ||
                    !strcmp(e->field_name, "clr_temp"))
                    store_values(msg, f, e, DEV_STRIDE, STEPWISE_MAX);
                else
                    store_values(msg, f, e, MON_STRIDE, -1);
                break;
            case FIELD_DEVICE_ARRAY:

                break;
            case FIELD_LIMITS:
                if (!strcmp(e->field_name, "upper_limit"))
                    store_limits(msg, f, e, MON_SLOT_VAL + 4, DEV_GROUP_R2,
                                 DEV_STRIDE, DEV_SLOT);
                else if (!strcmp(e->field_name, "lower_limit"))
                    store_limits(msg, f, e, MON_SLOT_VAL + 8, DEV_GROUP_R2,
                                 DEV_STRIDE, DEV_SLOT);
                else
                    store_limits(msg, f, e, MON_SLOT_VAL + 4, MON_GROUP_R2,
                                 MON_STRIDE, MON_SLOT);
                break;
            default:
                break;
            }
            break;
        }
        if (j == num_fields)
            return -1;
    }
    return (inst->num_fields == (int)(uint8_t)not_applied) ? -1 : 0;
}





static void init_monitor_record(void *msg)
{
    int g;
    int k;

    store_int(msg, OFF_PAYLOAD, 0);
    store_int(msg, OFF_PAYLOAD + 4, INT32_MIN);
    store_int(msg, OFF_PAYLOAD + 8, INT32_MIN);

    for (g = 0; g < MAX_GROUPS; g++) {
        unsigned char *group = (unsigned char *)at(msg, MON_GROUP0 + (size_t)g * MON_STRIDE);

        for (k = 0; k < MAX_SLOTS; k++) {
            group[k * MON_SLOT] = '\0';
            store_int(group, k * MON_SLOT + MON_SLOT_VAL, INT32_MIN);
        }
        store_int(group, MON_GROUP_COUNT, 0);
        if (g < MAX_GROUPS - 1) {

            store_int(group, MON_GROUP_R1, INT32_MIN);
            store_int(group, MON_GROUP_R2, INT32_MIN);
        }
    }
}

static void init_override_record(void *msg)
{
    int g;
    int k;

    store_int(msg, OFF_PAYLOAD, 0);
    store_int(msg, OFF_PAYLOAD + 4, INT32_MIN);
    store_int(msg, OFF_PAYLOAD + 8, INT32_MIN);
    store_int(msg, OFF_POLLING_DELAY, 0);
    store_int(msg, OFF_PASSIVE_DELAY, 0);

    for (g = 0; g < MAX_GROUPS; g++) {
        unsigned char *group = (unsigned char *)at(msg, DEV_GROUP0 + (size_t)g * DEV_STRIDE);

        for (k = 0; k < MAX_SLOTS; k++) {
            group[k * DEV_SLOT] = '\0';
            store_int(group, k * DEV_SLOT + DEV_SLOT_UPPER, 0);
            store_int(group, k * DEV_SLOT + DEV_SLOT_LOWER, 0);
        }
        store_int(group, DEV_GROUP_COUNT, 0);
        store_int(group, DEV_GROUP_R1, INT32_MIN);
        store_int(group, DEV_GROUP_R2, INT32_MIN);
    }
}

static void init_pid_record(void *msg)
{
    store_int(msg, OFF_PAYLOAD, INT32_MIN);
    store_int(msg, OFF_PAYLOAD + 4, INT32_MIN);
    *(unsigned char *)at(msg, OFF_PAYLOAD + 8) = '\0';
}





static const struct field_desc *algo_field_table(int algo, int *nfields)
{
    switch (algo) {
    case 0:
        *nfields = MONITOR_NFIELDS;
        return monitor_fields;
    case 1:
    case 2:
    case 3:
        *nfields = PID_NFIELDS;
        return pid_fields;
    case 7:
        *nfields = OVERRIDE_NFIELDS;
        return override_fields;
    default:
        *nfields = 0;
        return NULL;
    }
}

int thermal_client_config_set(struct config_instance *insts, int num_insts)
{
    struct sockaddr_un addr;
    unsigned char message[SOCK_DATA_SIZE];
    int fd;
    int sent = 0;
    unsigned int idx;
    int i;

    if (!insts || !num_insts) {
        ALOGE("Thermal-Lib-Client:%s: unexpected NULL", "thermal_client_config_set");
        return -1;
    }


    for (idx = 0; (unsigned int)num_insts > (uint8_t)idx; idx++) {
        struct config_instance *inst = &insts[(uint8_t)idx];
        const struct field_desc *table;
        int nfields;
        int algo;

        if (!inst->cfg_desc || !inst->algo_type || !inst->num_fields || !inst->fields)
            goto invalid;
        if (!inst->fields_mask) {
            ALOGE("Thermal-Lib:%s No adjustment request\n", "validate_config");
            goto invalid;
        }
        algo = check_for_algo_type(-1, inst->algo_type);
        table = algo_field_table(algo, &nfields);
        if (!table)
            goto invalid;
        for (i = 0; i < inst->num_fields; i++) {
            struct target_client_field *e = &inst->fields[i];
            int j;

            if (!e->field_name)
                goto invalid;
            for (j = 0; j < nfields; j++) {
                if (!strcmp(e->field_name, table[j].name))
                    break;
            }
            if (j == nfields)
                goto invalid;
            if (!e->num_vals || e->field_type > FIELD_LIMITS || !e->vals)
                goto invalid;
        }
        continue;
invalid:
        ALOGE("Thermal-Lib-Client:%s: request config is not valid",
             "thermal_client_config_set");
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    fd = connect_to_socket(CLIENT_NAME_CONFIG_SET, &addr);
    if (fd < 0) {
        ALOGE("Thermal-Lib-Client:%s: Could not connect to socket\n",
             "thermal_client_config_set");
        return -1;
    }


    memset(message, 0, SOCK_DATA_SIZE);
    store_int(message, OFF_TYPE, 8);
    store_int(message, OFF_DESC, num_insts);
    strlcpy((char *)at(message, OFF_NAME), CLIENT_NAME_CONFIG_SET, ANAME_LEN);
    if (send_to_fd(fd, message, SOCK_DATA_SIZE) < 1) {
        ALOGE("Thermal-Lib-Client: Unable to send request data to fd %d", fd);
        close(fd);
        return -1;
    }

    for (idx = 0; (unsigned int)num_insts > (uint8_t)idx; idx++) {
        struct config_instance *inst = &insts[(uint8_t)idx];
        const struct field_desc *table;
        int nfields;
        int algo;

        memset(message, 0, SOCK_DATA_SIZE);
        store_int(message, OFF_TYPE, -1);

        algo = check_for_algo_type(-1, inst->algo_type);
        if (algo != -1) {
            store_int(message, OFF_TYPE, algo);
            if (algo >= 1 && algo <= 3)
                init_pid_record(message);
            else if (algo == 7)
                init_override_record(message);
            else if (algo == 0)
                init_monitor_record(message);
        }

        strlcpy((char *)at(message, OFF_NAME), CLIENT_NAME_CONFIG_SET, ANAME_LEN);
        if (!inst->fields_mask) {
            ALOGE("Thermal-Lib-Client: config %d data is not valid\n",
                 (int)(uint8_t)idx + 1);
            continue;
        }
        strlcpy((char *)at(message, OFF_DESC), inst->cfg_desc, CFG_DESC_LEN);
        store_int(message, OFF_MASK, (int)inst->fields_mask);

        table = algo_field_table(load_int(message, OFF_TYPE), &nfields);
        if (!table) {
            ALOGE("Thermal-Lib-Client: config %d data is not valid\n",
                 (int)(uint8_t)idx + 1);
            continue;
        }
        if (add_algo_specific_data_to_sock_data(inst, message, table, nfields)) {
            ALOGE("Thermal-Lib-Client: config %d data is not valid\n",
                 (int)(uint8_t)idx + 1);
            continue;
        }

        if (send_to_fd(fd, message, SOCK_DATA_SIZE) < 1) {
            ALOGE("Thermal-Lib-Client: Unable to send request data to fd %d", fd);
            continue;
        }
        ALOGI("Thermal-Lib-Client: Client config query request sent %s  %d\n",
             (char *)at(message, OFF_NAME), load_int(message, OFF_TYPE));
        sent++;
    }


    memset(message, 0, SOCK_DATA_SIZE);
    store_int(message, OFF_TYPE, 8);
    store_int(message, OFF_DESC, 0);
    strlcpy((char *)at(message, OFF_NAME), CLIENT_NAME_CONFIG_SET, ANAME_LEN);
    if (send_to_fd(fd, message, SOCK_DATA_SIZE) <= 0) {
        ALOGE("Thermal-Lib-Client: Unable to send request data to fd %d", fd);
        close(fd);
        return -1;
    }
    close(fd);
    return sent;
}





void thermal_client_config_cleanup(struct config_instance *insts, int num_insts)
{
    int i;

    if (!insts || !num_insts)
        return;
    for (i = 0; i < num_insts; i++) {
        struct config_instance *inst = &insts[i];
        int j;

        if (inst->cfg_desc)
            free(inst->cfg_desc);
        if (inst->algo_type)
            free(inst->algo_type);
        if (!inst->fields)
            continue;
        for (j = 0; j < inst->num_fields; j++) {
            struct target_client_field *e = &inst->fields[j];
            void **vals = (void **)e->vals;

            if (e->field_name)
                free(e->field_name);
            if (vals) {
                if (e->field_type == FIELD_DEVICE_ARRAY) {
                    int k;

                    for (k = 0; k < e->num_vals; k++)
                        free(vals[k]);
                }
                free(vals);
            }
        }
        free(inst->fields);
    }
    free(insts);
}





int thermal_client_config_query(const char *name, struct config_instance **insts)
{
    unsigned char message[SOCK_DATA_SIZE];
    unsigned char reply[SOCK_DATA_SIZE];
    struct sockaddr_un addr;
    struct timeval tv = { 1, 0 };
    struct config_instance *list = NULL;
    int announced = 0;
    int expected = 0;
    int received;
    int client_id;
    int algo;
    int num = 0;
    int have = 0;
    char algoname[16];
    int fd;
    int ret;
    int type;
    int count;

    memset(message, 0, SOCK_DATA_SIZE);
    if (!name) {
        ALOGE("Thermal-Lib-Client:%s: unexpected NULL", "thermal_client_config_query");
        return 0;
    }
    client_id = check_for_algo_type(-1, (char *)name);
    if (client_id == -1)
        return 0;

    memset(&addr, 0, sizeof(addr));
    memset(at(message, OFF_NAME), 0, SOCK_DATA_SIZE - OFF_NAME);
    strlcpy((char *)at(message, OFF_NAME), CLIENT_NAME_CONFIG_QUERY, ANAME_LEN);
    store_int(message, OFF_TYPE, client_id);
    store_int(message, OFF_DESC, 0);

    fd = connect_to_socket((char *)at(message, OFF_NAME), &addr);
    if (fd < 0) {
        ALOGE("Thermal-Lib-Client:%s: could not connect to socket\n",
             "thermal_client_config_query");
        return 0;
    }

    setsockopt(fd, SOL_SOCKET, 0x14, &tv, sizeof(tv));
    ret = send_to_fd(fd, message, SOCK_DATA_SIZE);
    if (ret <= 0) {
        ALOGE("Thermal-Lib-Client: Unable to send request data to fd %d", fd);
        close(fd);
        return 0;
    }
    ALOGI("Thermal-Lib-Client: Client config query request sent %s  %d\n",
         (char *)at(message, OFF_NAME), load_int(message, OFF_TYPE));

    for (;;) {
        if (!have) {
            memset(reply, 0, SOCK_DATA_SIZE);
            expected = announced;
            received = recv_from_fd(fd, reply, SOCK_DATA_SIZE);
            if (received < 1) {
                ALOGE("Thermal-Lib-Client:%s: recv failed, received byte %zd\n",
                     "recv_and_extract_packet", (ssize_t)received);
                if (received == -1)
                    ALOGE("errno %d:%s\n", errno, strerror(errno));
                goto check;
            }
        }
        have = 0;

        if (!list) {
            if (received != SOCK_DATA_SIZE) {
                ALOGE("Thermal-Lib-Client:%s: size of received message not matching\n",
                     "recv_and_extract_packet");
                goto check;
            }
            if (strnlen((const char *)at(reply, OFF_NAME), ANAME_LEN) > ANAME_LEN - 1) {
                ALOGE("Thermal-Lib-Client:%s: invalid client name %s\n",
                     "recv_and_extract_packet", (char *)at(reply, OFF_NAME));
                goto check;
            }
            ALOGI("Thermal-Lib-Client:%s: Client received msg %s %d\n",
                 "recv_and_extract_packet", (char *)at(reply, OFF_NAME),
                 load_int(reply, OFF_TYPE));
            if (!is_reply_client_name((char *)at(reply, OFF_NAME))) {
                ALOGE("Thermal-Lib-Client:%s is not in supported thermal client list",
                     (char *)at(reply, OFF_NAME));
                goto check;
            }
            if (load_int(reply, OFF_TYPE) != 8)
                goto bad_header;
            count = load_int(reply, OFF_COUNT);
            if (!count)
                goto check;
            if (count < 1)
                goto bad_header;
            if (count > MAX_INSTANCES) {
                ALOGE("Thermal-Lib-Client:%s Invalid number configs %d\n",
                     "recv_and_extract_packet", count);
                goto check;
            }
            list = (struct config_instance *)malloc(sizeof(*list) * count);
            if (!list) {
                ALOGE("%s: malloc failed for fields", "recv_and_extract_packet");
                goto check;
            }
            announced = count;
            continue;
        }

        if (received != SOCK_DATA_SIZE) {
            ALOGE("Thermal-Lib-Client:%s: size of received message not matching\n",
                 "recv_and_extract_packet");
            goto check;
        }
        if (strnlen((const char *)at(reply, OFF_NAME), ANAME_LEN) > ANAME_LEN - 1) {
            ALOGE("Thermal-Lib-Client:%s: invalid client name %s\n",
                 "recv_and_extract_packet", (char *)at(reply, OFF_NAME));
            goto check;
        }
        ALOGI("Thermal-Lib-Client:%s: Client received msg %s %d\n",
             "recv_and_extract_packet", (char *)at(reply, OFF_NAME),
             load_int(reply, OFF_TYPE));
        if (!is_reply_client_name((char *)at(reply, OFF_NAME))) {
            ALOGE("Thermal-Lib-Client:%s is not in supported thermal client list",
                 (char *)at(reply, OFF_NAME));
            goto check;
        }

        type = load_int(reply, OFF_TYPE);
        if (type == 8) {
            count = load_int(reply, OFF_COUNT);
            if (!count)
                goto check;
            if (count < 1)
                goto bad_header;
            if (count > MAX_INSTANCES) {
                ALOGE("Thermal-Lib-Client:%s Invalid number configs %d\n",
                     "recv_and_extract_packet", count);
                goto check;
            }
            list = (struct config_instance *)malloc(sizeof(*list) * count);
            if (!list) {
                ALOGE("%s: malloc failed for fields", "recv_and_extract_packet");
                goto check;
            }
            announced = count;
            continue;
        }

        if (strnlen((const char *)at(reply, OFF_DESC), CFG_DESC_LEN) > CFG_DESC_LEN - 1) {
            ALOGE("Thermal-Lib-Client:%s: invalid config description %s\n",
                 "recv_and_extract_packet", (char *)at(reply, OFF_DESC));
            goto check;
        }
        if (num >= expected)
            goto cleanup;
        if (client_id != 8 && type != client_id)
            goto cleanup;
        algo = check_for_algo_type(type, algoname);
        if (algo == -1)
            goto cleanup;

        list[num].cfg_desc = (char *)malloc(CFG_DESC_LEN);
        if (!list[num].cfg_desc) {
            ALOGE("%s: malloc failed for config_desc", "thermal_client_config_query");
            goto cleanup;
        }
        strlcpy(list[num].cfg_desc, (const char *)at(reply, OFF_DESC), CFG_DESC_LEN);
        list[num].algo_type = (char *)malloc(10);
        if (!list[num].algo_type) {
            ALOGE("%s: malloc failed for algo_type", "thermal_client_config_query");
            goto cleanup;
        }
        strlcpy(list[num].algo_type, algoname, 10);
        list[num].fields_mask = 0;

        switch (type) {
        case 1:
        case 2:
        case 3:
            ret = add_settings_to_client_config(&list[num], reply, pid_fields,
                                                PID_NFIELDS);
            break;
        case 7:
            ret = add_settings_to_client_config(&list[num], reply, override_fields,
                                                OVERRIDE_NFIELDS);
            break;
        case 0:
            ret = add_settings_to_client_config(&list[num], reply, monitor_fields,
                                                MONITOR_NFIELDS);
            break;
        default:
            ALOGE("Thermal-Lib-Client:Unknown algo type\n");
            goto cleanup;
        }
        if (ret) {
            ALOGE("Thermal-Lib-Client:%s: config data is invalid\n",
                 "thermal_client_config_query");
            goto cleanup;
        }

        memset(reply, 0, SOCK_DATA_SIZE);
        num++;
        received = recv_from_fd(fd, reply, SOCK_DATA_SIZE);
        if (received < 1) {
            ALOGE("Thermal-Lib-Client:%s: recv failed, received byte %zd\n",
                 "recv_and_extract_packet", (ssize_t)received);
            if (received == -1)
                ALOGE("errno %d:%s\n", errno, strerror(errno));
            goto check;
        }
        have = 1;
        continue;

bad_header:
        if (strnlen((const char *)at(reply, OFF_DESC), CFG_DESC_LEN) > CFG_DESC_LEN - 1) {
            ALOGE("Thermal-Lib-Client:%s: invalid config description %s\n",
                 "recv_and_extract_packet", (char *)at(reply, OFF_DESC));
            goto check;
        }
        goto cleanup;

cleanup:
        if (list) {

            if (list[num].cfg_desc)
                free(list[num].cfg_desc);
            if (list[num].algo_type)
                free(list[num].algo_type);
            thermal_client_config_cleanup(list, num);
        }
        *insts = NULL;
        close(fd);
        return 0;

check:
        if (num != expected)
            goto cleanup;
        *insts = list;
        close(fd);
        return num;
    }
}





static void *do_request_to_thermal(void *path)
{
    struct sockaddr_un addr;
    int sock;
    int ret;

    for (;;) {
        sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock < 0) {
            ALOGE("Thermal-Lib-Client: %s: failed setup client send sockfd",
                 "do_request_to_thermal");
            sleep(5);
            continue;
        }
        memset(&addr, 0, sizeof(struct sockaddr_un) - 2);
        strlcpy(addr.sun_path, (const char *)path, sizeof(struct sockaddr_un) - 2);
        addr.sun_family = AF_UNIX;
        ret = connect(sock, (struct sockaddr *)&addr, strlen((const char *)path) + 2);
        if (ret) {
            close(sock);
            sleep(5);
            continue;
        }
        ret = send_to_fd(sock, client_request, SOCK_DATA_SIZE);
        if (ret > 0)
            ALOGI("Thermal-Lib-Client: Client request sent");
        else
            ALOGE("Thermal-Lib-Client: Unable to send request data to fd %d", sock);
        close(sock);
        return NULL;
    }
}

int thermal_client_request(const char *name, int data)
{
    pthread_t thread;
    int idx;

    if (!name) {
        ALOGE("Thermal-Lib-Client:%s: unexpected NULL", "thermal_client_request");
        return -EINVAL;
    }
    idx = name_to_client_index(name);
    if (idx < 0)
        return -EINVAL;

    memset(client_request, 0, SOCK_DATA_SIZE);
    strlcpy((char *)at(client_request, OFF_NAME), name, ANAME_LEN);
    store_int(client_request, OFF_DESC, data);

    if (pthread_create(&thread, NULL, do_request_to_thermal,
                       (void *)client_sockets[idx].socket_path)) {
        ALOGE("Thermal-Lib-Client: Unable to create pthread to send client request from %s",
             name);
        return -EINVAL;
    }
    return 0;
}





static void bw_release(void)
{
    if (bw_pipe[0] >= 0)
        close(bw_pipe[0]);
    bw_pipe[0] = -1;
    if (bw_pipe[1] >= 0)
        close(bw_pipe[1]);
    bw_pipe[1] = -1;
}

static void *do_bw_request_to_thermal(void *path)
{
    struct sockaddr_un addr;
    unsigned char message[SOCK_DATA_SIZE];
    fd_set master;
    fd_set readfds;
    char pipebuf[8];
    int sock = -1;
    int state = 0;
    int val;
    int ret;

    FD_ZERO(&master);
    if (bw_cancel & 1) {
        bw_release();
        return NULL;
    }

    for (;;) {
        if (bw_cancel == 1)
            break;
        memset(pipebuf, 0, 5);
        if (bw_pipe[0] < 0) {
            if (pipe(bw_pipe)) {
                ALOGE("Thermal-Lib:Pipe error. Re-trying after 5s\n");
                sleep(5);
                continue;
            }
            FD_SET(bw_pipe[0], &master);
        }
        if (sock < 0) {
            sock = socket(AF_UNIX, SOCK_STREAM, 0);
            if (sock < 0) {
                ALOGE("Thermal-Lib-Client: %s: failed setup client send sockfd",
                     "do_bw_request_to_thermal");
                sleep(5);
                continue;
            }
            FD_SET(sock, &master);
        }
        if (state == 1) {
            if (read(bw_pipe[0], pipebuf, 5) < 0)
                ALOGE("bw pipe read error:%d\n", errno);
            state = 0;
        } else if (state == 0) {
            memset(&addr, 0, sizeof(struct sockaddr_un) - 2);
            strlcpy(addr.sun_path, (const char *)path, sizeof(struct sockaddr_un) - 2);
            addr.sun_family = AF_UNIX;
            ret = connect(sock, (struct sockaddr *)&addr,
                          strlen((const char *)path) + 2);
            if (ret) {
                ALOGE("Thermal-Lib:%s:connect failure, err:%d\n",
                     "do_bw_request_to_thermal", ret);
                close(sock);
                sleep(1);
                state = 0;
                sock = -1;
                continue;
            }
        }

        memset(message, 0, SOCK_DATA_SIZE);
        pthread_mutex_lock(&bw_mutex);
        memset(message, 0, SOCK_DATA_SIZE);
        strlcpy((char *)at(message, OFF_NAME), (char *)at(bw_request, OFF_NAME),
                ANAME_LEN);
        val = load_int(bw_request, OFF_DESC);
        store_int(message, OFF_DESC, val);
        pthread_mutex_unlock(&bw_mutex);

        ret = send_to_fd(sock, message, SOCK_DATA_SIZE);
        if (ret <= 0) {
            ALOGE("Thermal-Lib-Client: Unable to send request data to fd %d", sock);
            close(sock);
            sock = -1;
            bw_release();
            return NULL;
        }
        ALOGI("Thermal-Lib: Client request sent for %s with val:0x%x\n",
             (char *)at(message, OFF_NAME), val);

        for (;;) {
            if (bw_cancel & 1)
                goto out;
            readfds = master;
            ret = select(1024, &readfds, NULL, NULL, NULL);
            if (ret <= 0)
                continue;
            if (FD_ISSET(bw_pipe[0], &readfds)) {
                state = 1;
                break;
            }
            if (FD_ISSET(sock, &readfds)) {
                ALOGI("Thermal-Lib: %s:Server is crashed,reconnect & send again\n",
                     "do_bw_request_to_thermal");
                FD_CLR(sock, &master);
                close(sock);
                sock = -1;
                state = 0;
                break;
            }
        }
    }

out:
    if (sock >= 0)
        close(sock);
    bw_release();
    return NULL;
}

int thermal_bandwidth_client_request(const char *name, int data)
{
    int idx;

    if (!name) {
        ALOGE("Thermal-Lib:%s: unexpected NULL", "thermal_bandwidth_client_request");
        return -EINVAL;
    }
    idx = name_to_client_index(name);
    if (idx < 0)
        return -EINVAL;
    if (data < 0) {
        ALOGE("%s: Invalid input data: < 0\n", "thermal_bandwidth_client_request");
        return -EINVAL;
    }
    if ((int16_t)data < 0) {
        ALOGE("%s: Invalid input data: > lim \n", "thermal_bandwidth_client_request");
        return -EINVAL;
    }

    pthread_mutex_lock(&bw_request_mutex);
    if (bw_cancel == 1)
        bw_cancel = 0;
    pthread_mutex_lock(&bw_mutex);
    memset(bw_request, 0, SOCK_DATA_SIZE);
    strlcpy((char *)at(bw_request, OFF_NAME), name, ANAME_LEN);
    store_int(bw_request, OFF_DESC, data);
    pthread_mutex_unlock(&bw_mutex);

    if (bw_running & 1) {
        if (bw_pipe[1] >= 0) {
            if (write(bw_pipe[1], "exit", 5) < 0)
                ALOGE("errno %d:%s\n", errno, strerror(errno));
        }
        pthread_mutex_unlock(&bw_request_mutex);
        return 0;
    }
    data = pthread_create(&bw_thread, NULL, do_bw_request_to_thermal,
                          (void *)client_sockets[idx].socket_path);
    if (data) {
        ALOGE("Thermal-Lib: Unable to create pthread to send client request from %s",
             name);
    } else {
        bw_running = 1;
    }
    pthread_mutex_unlock(&bw_request_mutex);
    return data;
}

int thermal_bandwidth_client_cancel_request(const char *name)
{
    pthread_mutex_lock(&bw_request_mutex);
    pthread_mutex_lock(&bw_mutex);
    store_int(bw_request, OFF_DESC, 0);
    pthread_mutex_unlock(&bw_mutex);

    bw_cancel = 1;
    if (bw_pipe[1] >= 0) {
        if (write(bw_pipe[1], "exit", 5) < 0)
            ALOGE("errno %d:%s\n", errno, strerror(errno));
    }
    if (bw_running == 1)
        pthread_join(bw_thread, NULL);
    ALOGI("%s: Removing all bw request for %s\n",
         "thermal_bandwidth_client_cancel_request", name);
    bw_running = 0;
    pthread_mutex_unlock(&bw_request_mutex);
    return 0;
}





static const struct {
    const char *name;
    int lo;
    int hi;
} report_limits[] = {
    { "camera",        0,  10 },
    { "camcorder",     0,  10 },
    { "spkr",        -30, 150 },
    { "config_query",  0,   3 },
};

static void *do_listen(void *arg)
{
    unsigned char message[SOCK_DATA_SIZE];
    char target[0x7C];
    const char *path = (const char *)arg;
    fd_set master;
    fd_set readfds;
    struct callback_node *node;
    int sock = -1;
    int idx;
    int ret;

    memset(message, 0, SOCK_DATA_SIZE);
    memset(target, 0, sizeof(target));
    if (!path) {
        ALOGE("Thermal-Client-Lib: Invalid input\n");
        return NULL;
    }
    if (listen_stop & 1) {
        close(listen_pipe[0]);
        close(listen_pipe[1]);
        listen_pipe[0] = -1;
        listen_pipe[1] = -1;
        return NULL;
    }
    FD_ZERO(&master);

    for (;;) {
        if (listen_stop & 1) {
            close(listen_pipe[0]);
            close(listen_pipe[1]);
            listen_pipe[0] = -1;
            listen_pipe[1] = -1;
            return NULL;
        }
        if (listen_pipe[0] < 0) {
            if (pipe(listen_pipe)) {
                ALOGE("Thermal-Lib:Pipe error. Re-trying after 5s\n");
                sleep(5);
                continue;
            }
        }
        sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock < 0) {
            sleep(5);
            continue;
        }
        memset(&listen_addr, 0, sizeof(struct sockaddr_un) - 2);
        strlcpy(listen_addr.sun_path, path, sizeof(struct sockaddr_un) - 2);
        listen_addr.sun_family = AF_UNIX;
        if (connect(sock, (struct sockaddr *)&listen_addr, strlen(path) + 2)) {
            close(sock);
            sleep(5);
            continue;
        }
        FD_SET(sock, &master);
        FD_SET(listen_pipe[0], &master);

        for (;;) {
            if (listen_stop & 1)
                goto relisten;
            readfds = master;
            if (select(1024, &readfds, NULL, NULL, NULL) < 1)
                continue;
            if (FD_ISSET(listen_pipe[0], &readfds))
                goto relisten;

            if (!strcmp(path, SOCKET_SEND_RULE)) {
                ssize_t n;

                memset(target, 0, sizeof(target));
                n = recvfrom(sock, target, sizeof(target), 0, NULL, NULL);
                if (n <= 0) {
                    ALOGE("Thermal-Lib-Client:%s: recv failed", "do_listen");
                    continue;
                }
                pthread_mutex_lock(&list_mutex);
                if (list_head) {
                    node = get_callback_node_from_list(
                        (struct callback_node *)list_head, ALGO_REPORT);
                    if (!node) {
                        ALOGE("No callback for report, stop listening\n");
                        listen_stop = 1;
                        pthread_mutex_unlock(&list_mutex);
                        continue;
                    }
                    if (node->cb)
                        node->cb((long)(intptr_t)target, node->data, node->arg);
                }
                pthread_mutex_unlock(&list_mutex);
                continue;
            }

            memset(message, 0, SOCK_DATA_SIZE);
            ret = recv_from_fd(sock, message, SOCK_DATA_SIZE);
            if (ret <= 0) {
                ALOGE("Thermal-Lib-Client:%s: recv failed", "do_listen");
                continue;
            }
            if (ret != SOCK_DATA_SIZE)
                continue;
            if (strnlen((const char *)at(message, OFF_NAME), ANAME_LEN) >= ANAME_LEN)
                message[OFF_NAME + ANAME_LEN - 1] = '\0';
            ALOGI("Thermal-Lib-Client: Client received msg %s %d",
                 (char *)at(message, OFF_NAME), load_int(message, OFF_DESC));

            if (!is_reply_client_name((char *)at(message, OFF_NAME))) {
                ALOGE("Thermal-Lib-Client:%s is not in supported thermal client list",
                     (char *)at(message, OFF_NAME));
                continue;
            }
            if (!strncmp((char *)at(message, OFF_NAME), "camera", ANAME_LEN))
                idx = 0;
            else if (!strncmp((char *)at(message, OFF_NAME), "camcorder", ANAME_LEN))
                idx = 1;
            else if (!strncmp((char *)at(message, OFF_NAME), "spkr", ANAME_LEN))
                idx = 2;
            else
                idx = 3;
            ret = load_int(message, OFF_DESC);
            if (ret < report_limits[idx].lo || ret > report_limits[idx].hi) {
                ALOGE("Thermal-Lib-Client:%s: invalid level %d unexpected\n",
                     "do_listen", ret);
                continue;
            }

            pthread_mutex_lock(&list_mutex);
            node = (struct callback_node *)list_head;
            if (node) {
                int count = 0;

                for (;;) {
                    node = get_callback_node_from_list(node,
                                                       (char *)at(message, OFF_NAME));
                    if (!node) {
                        if (!count)
                            ALOGE("Thermal-Lib-Client: No Callback registered for %s",
                                 (char *)at(message, OFF_NAME));
                        break;
                    }
                    count++;
                    if (node->cb)
                        node->cb((long)ret, node->data, node->arg);
                    node = node->next;
                    if (!node)
                        break;
                }
            }
            pthread_mutex_unlock(&list_mutex);
        }

relisten:
        close(sock);
        FD_CLR(sock, &master);
        FD_CLR(listen_pipe[0], &master);
    }
}





int thermal_client_register_callback(const char *name, int (*cb)(long, void *, void *),
                                     void *data)
{
    int handle;

    if (!name || !cb) {
        ALOGE("Thermal-Lib-Client:%s: unexpected NULL client registraion failed ",
             "thermal_client_register_callback");
        return 0;
    }
    if (strncmp(name, "camcorder", ANAME_LEN) &&
        strncmp(name, "spkr", ANAME_LEN) &&
        strncmp(name, CLIENT_NAME_CONFIG_SET, ANAME_LEN) &&
        strncmp(name, CLIENT_NAME_CONFIG_QUERY, ANAME_LEN)) {
        ALOGE("Thermal-Lib-Client:%s is not in supported thermal client list", name);
        return 0;
    }

    pthread_mutex_lock(&callback_mutex);
    if (listen_stop == 1)
        listen_stop = 0;
    pthread_mutex_lock(&list_mutex);
    handle = add_to_list(name, cb, data);
    if (!handle) {
        ALOGE("Thermal-Lib-Client: %s: Client Registration failed",
             "thermal_client_register_callback");
        pthread_mutex_unlock(&list_mutex);
        pthread_mutex_unlock(&callback_mutex);
        return 0;
    }
    pthread_mutex_unlock(&list_mutex);

    if (!listen_running) {
        if (pthread_create(&listen_thread, NULL, do_listen, (void *)SOCKET_SEND_CLIENT)) {
            ALOGE("Thermal-Lib: Unable to create pthread to send client request from %s",
                 name);
            pthread_mutex_lock(&list_mutex);
            remove_from_list(handle);
            pthread_mutex_unlock(&list_mutex);
            pthread_mutex_unlock(&callback_mutex);
            return 0;
        }
        listen_running = 1;
    }
    ALOGI("Thermal-Lib-Client: Registration successful for %s with handle:%d", name,
         handle);
    pthread_mutex_unlock(&callback_mutex);
    return handle;
}

int thermal_client_register_report_callback(int (*cb)(long, void *, void *), void *data)
{
    int handle;

    if (!cb) {
        ALOGE("Thermal-Lib-Client:%s: unexpected NULL client registration failed ",
             "thermal_client_register_report_callback");
        return 0;
    }
    pthread_mutex_lock(&list_mutex);
    handle = add_to_list(ALGO_REPORT, cb, data);
    if (!handle) {
        ALOGE("Thermal-Lib-Client: %s: Client Registration failed",
             "thermal_client_register_report_callback");
        pthread_mutex_unlock(&list_mutex);
        return 0;
    }
    pthread_mutex_unlock(&list_mutex);

    if (pthread_create(&listen_thread, NULL, do_listen, (void *)SOCKET_SEND_RULE)) {
        ALOGE("Thermal-Lib-Client: Unable to create pthread to listen thermal events for report events");
        return -1;
    }
    ALOGI("Thermal-Lib-Client: Registration successful for thermal report events:%d",
         handle);
    return handle;
}

int thermal_client_unregister_callback(int handle)
{
    int ret;

    pthread_mutex_lock(&callback_mutex);
    pthread_mutex_lock(&list_mutex);
    ret = remove_from_list(handle);
    pthread_mutex_unlock(&list_mutex);
    if (ret < 0) {
        ALOGE("Thermal-Lib-Client: thermal client unregister callback error");
        pthread_mutex_unlock(&callback_mutex);
        return 0;
    }
    if (!list_head) {
        listen_stop = 1;
        if (write(listen_pipe[1], "exit", 5) < 0)
            ALOGE("pipe write error:%d\n", errno);
        if (listen_running == 1)
            pthread_join(listen_thread, NULL);
        listen_running = 0;
    }
    ALOGI("Thermal-Lib-Client: Unregisteration is successfull for handle:%d", handle);
    pthread_mutex_unlock(&callback_mutex);
    return 0;
}
