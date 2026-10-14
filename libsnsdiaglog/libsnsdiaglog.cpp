/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#include "libsnsdiaglog.h"





extern "C" {

void *memcpy(void *dst, const void *src, unsigned long n);
void *malloc(unsigned long n);
void free(void *p);
void abort(void) __attribute__((noreturn));

int fprintf(void *stream, const char *fmt, ...);
extern void *stderr;

int __android_log_print(int prio, const char *tag, const char *fmt, ...);

int pthread_mutex_init(sensors_diag_log_mutex *m, const void *attr);
int pthread_mutex_destroy(sensors_diag_log_mutex *m);
int pthread_mutex_lock(sensors_diag_log_mutex *m);
int pthread_mutex_unlock(sensors_diag_log_mutex *m);


int Diag_LSM_Init(void *pInit);
void Diag_LSM_DeInit(void);
int log_status(unsigned short log_code);
void *log_alloc(unsigned int log_code, unsigned int length);
void log_commit(void *pkt);

}


namespace android {
long long elapsedRealtimeNano();
}


namespace sensors_log {

int get_level();
const char *get_tag();
bool get_stderr_logging();

}





static inline unsigned long long read_cntvct(void) {
#ifdef __aarch64__
    unsigned long long v;
    __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(v));
    return v;
#else
    return 0;
#endif
}

static inline unsigned long long read_cntfrq(void) {
#ifdef __aarch64__
    unsigned long long v;
    __asm__ __volatile__("mrs %0, cntfrq_el0" : "=r"(v));
    return v;
#else
    return 1000000000ull;
#endif
}

static unsigned long long elapsed_realtime_nano(void) {
    return (unsigned long long)android::elapsedRealtimeNano();
}






extern "C" {
__attribute__((weak, visibility("default")))
unsigned char g_sns_timeutil_inst[0x38] asm("_ZZN16sensors_timeutil12get_instanceEvE4inst");
__attribute__((weak, visibility("default")))
unsigned char g_sns_timeutil_guard[8] asm("_ZGVZN16sensors_timeutil12get_instanceEvE4inst");

extern int __cxa_guard_acquire(void *guard);
extern void __cxa_guard_release(void *guard);
}

namespace sensors_timeutil {

struct timeutil {
    unsigned long long cntfrq;
    unsigned long long ns_per_sec;
    unsigned long long realtime;
    unsigned int word18;
    unsigned int word1c;
    unsigned int word20;
    unsigned int word24;
    unsigned int word28;
    unsigned int pad2c;
    long long boot_offset;
};

static const unsigned int kInitWords[4] = {0xf8475800u, 13u, 10000u, 0u};

timeutil &get_instance() {
    if (g_sns_timeutil_guard[0] == 0) {
        if (__cxa_guard_acquire(g_sns_timeutil_guard)) {
            timeutil *t = (timeutil *)g_sns_timeutil_inst;
            unsigned long long freq = read_cntfrq();
            t->cntfrq = freq;
            t->ns_per_sec = 1000000000ull;
            t->realtime = elapsed_realtime_nano();
            t->word18 = kInitWords[0];
            t->word1c = kInitWords[1];
            t->word20 = kInitWords[2];
            t->word24 = kInitWords[3];
            t->word28 = 20;

            double ns_per_tick = (double)t->ns_per_sec / (double)t->cntfrq;
            double ticks_ns = ns_per_tick * (double)read_cntvct();
            unsigned long long ticks_ns_u = (unsigned long long)ticks_ns;
            long long a = (long long)t->realtime;
            long long b = (long long)ticks_ns_u;
            long long delta = (long long)((unsigned long long)a - (unsigned long long)b);
            if (((a ^ b) & (a ^ delta)) < 0)
                abort();
            t->boot_offset = delta;
            __cxa_guard_release(g_sns_timeutil_guard);
        }
    }
    return *(timeutil *)g_sns_timeutil_inst;
}

}






enum {
    PB_VARINT = 0,
    PB_FIXED64 = 1,
    PB_LEN = 2,
    PB_GROUP = 3,
    PB_ENDGROUP = 4,
    PB_FIXED32 = 5,
};

struct pb_buf {
    unsigned char *p;
    unsigned long len;
    unsigned long cap;
};

static void pb_init(pb_buf *b) {
    b->p = 0;
    b->len = 0;
    b->cap = 0;
}

static void pb_free(pb_buf *b) {
    if (b->p)
        free(b->p);
    pb_init(b);
}

static bool pb_reserve(pb_buf *b, unsigned long extra) {
    if (b->len + extra <= b->cap)
        return true;
    unsigned long cap = b->cap ? b->cap : 128;
    while (cap < b->len + extra) {
        if (cap > (1ul << 30))
            return false;
        cap *= 2;
    }

    unsigned char *p = (unsigned char *)malloc(cap);
    if (!p)
        return false;
    if (b->len)
        memcpy(p, b->p, b->len);
    if (b->p)
        free(b->p);
    b->p = p;
    b->cap = cap;
    return true;
}

static void pb_bytes(pb_buf *b, const void *src, unsigned long n) {
    if (!pb_reserve(b, n))
        return;
    memcpy(b->p + b->len, src, n);
    b->len += n;
}

static void pb_varint(pb_buf *b, unsigned long long v) {
    unsigned char tmp[10];
    unsigned n = 0;
    do {
        unsigned char byte = (unsigned char)(v & 0x7f);
        v >>= 7;
        if (v)
            byte |= 0x80;
        tmp[n++] = byte;
    } while (v);
    pb_bytes(b, tmp, n);
}

static void pb_tag(pb_buf *b, unsigned field, unsigned wire_type) {
    pb_varint(b, ((unsigned long long)field << 3) | wire_type);
}

static void pb_fixed32(pb_buf *b, unsigned field, unsigned v) {
    pb_tag(b, field, PB_FIXED32);
    unsigned char tmp[4];
    tmp[0] = (unsigned char)v;
    tmp[1] = (unsigned char)(v >> 8);
    tmp[2] = (unsigned char)(v >> 16);
    tmp[3] = (unsigned char)(v >> 24);
    pb_bytes(b, tmp, 4);
}

static void pb_fixed64(pb_buf *b, unsigned field, unsigned long long v) {
    pb_tag(b, field, PB_FIXED64);
    unsigned char tmp[8];
    for (unsigned i = 0; i < 8; i++)
        tmp[i] = (unsigned char)(v >> (8 * i));
    pb_bytes(b, tmp, 8);
}

static void pb_fixed_bytes(pb_buf *b, unsigned field, unsigned wire_type, const void *src, unsigned long n) {
    pb_tag(b, field, wire_type);
    pb_bytes(b, src, n);
}

static void pb_length_delimited(pb_buf *b, unsigned field, const void *src, unsigned long n) {
    pb_tag(b, field, PB_LEN);
    pb_varint(b, n);
    pb_bytes(b, src, n);
}

static void pb_string(pb_buf *b, unsigned field, const char *s) {
    unsigned long n = 0;
    while (s[n])
        n++;
    pb_length_delimited(b, field, s, n);
}






struct string_storage {
    unsigned char raw[24];
};

static void make_string(string_storage *s, const void *data, unsigned long n) {
    if (n <= 22) {
        s->raw[0] = (unsigned char)(n << 1);
        if (n)
            memcpy(s->raw + 1, data, n);
        s->raw[1 + n] = 0;
        return;
    }
    unsigned long cap = ((n + 16) & ~15ul) - 1;
    unsigned char *p = (unsigned char *)malloc(cap + 1);
    if (!p)
        abort();
    if (n)
        memcpy(p, data, n);
    p[n] = 0;
    for (unsigned i = 0; i < 8; i++) {
        s->raw[i] = (unsigned char)((cap | 1) >> (8 * i));
        s->raw[8 + i] = (unsigned char)(n >> (8 * i));
    }
    unsigned char *pp = p;
    for (unsigned i = 0; i < 8; i++)
        s->raw[16 + i] = (unsigned char)((unsigned long long)pp >> (8 * i));
}

static void release_string(string_storage *s) {
    if (s->raw[0] & 1) {
        unsigned long long ptr = 0;
        for (unsigned i = 0; i < 8; i++)
            ptr |= (unsigned long long)s->raw[16 + i] << (8 * i);
        free((void *)ptr);
    }
}






struct pb_desc {
    unsigned num;
    unsigned wire_type;
    bool repeated;
};

static const pb_desc kRequestFields[] = {
    {1, PB_LEN, false},
    {2, PB_FIXED32, false},
    {3, PB_LEN, false},
    {4, PB_LEN, false},
};
static const pb_desc kEventFields[] = {
    {1, PB_LEN, false},
    {2, PB_LEN, true},
};

struct pb_occ {
    unsigned num;
    unsigned wire_type;
    const unsigned char *raw;
    unsigned long raw_len;
    const unsigned char *val;
    unsigned long val_len;
    unsigned long long varint;
};


static unsigned long pb_walk(const unsigned char *p, unsigned long n,
                             pb_occ *out, unsigned long cap) {
    unsigned long i = 0;
    unsigned long count = 0;
    while (i < n) {
        const unsigned char *start = p + i;

        unsigned long long tag = 0;
        unsigned shift = 0;
        bool bad = false;
        for (;;) {
            if (i >= n || shift > 63) {
                bad = true;
                break;
            }
            unsigned char byte = p[i++];
            tag |= (unsigned long long)(byte & 0x7f) << shift;
            if (!(byte & 0x80))
                break;
            shift += 7;
        }
        if (bad)
            break;
        unsigned field = (unsigned)(tag >> 3);
        unsigned wt = (unsigned)(tag & 7);
        if (field == 0)
            break;
        unsigned long long varint = 0;
        const unsigned char *val = 0;
        unsigned long val_len = 0;
        switch (wt) {
            case PB_VARINT: {
                unsigned shift2 = 0;
                for (;;) {
                    if (i >= n || shift2 > 63) {
                        bad = true;
                        break;
                    }
                    unsigned char byte = p[i++];
                    varint |= (unsigned long long)(byte & 0x7f) << shift2;
                    if (!(byte & 0x80))
                        break;
                    shift2 += 7;
                }
                break;
            }
            case PB_FIXED64:
                if (i + 8 > n) {
                    bad = true;
                } else {
                    val = p + i;
                    i += 8;
                }
                break;
            case PB_LEN: {
                unsigned long long len = 0;
                unsigned shift2 = 0;
                for (;;) {
                    if (i >= n || shift2 > 63) {
                        bad = true;
                        break;
                    }
                    unsigned char byte = p[i++];
                    len |= (unsigned long long)(byte & 0x7f) << shift2;
                    if (!(byte & 0x80))
                        break;
                    shift2 += 7;
                }
                if (bad)
                    break;
                if (len > n - i) {
                    bad = true;
                    break;
                }
                val = p + i;
                val_len = (unsigned long)len;
                i += val_len;
                break;
            }
            case PB_FIXED32:
                if (i + 4 > n) {
                    bad = true;
                } else {
                    val = p + i;
                    i += 4;
                }
                break;
            case PB_GROUP: {

                unsigned depth = 1;
                while (depth && i < n) {
                    unsigned long long t = 0;
                    unsigned shift2 = 0;
                    for (;;) {
                        if (i >= n || shift2 > 63) {
                            bad = true;
                            break;
                        }
                        unsigned char byte = p[i++];
                        t |= (unsigned long long)(byte & 0x7f) << shift2;
                        if (!(byte & 0x80))
                            break;
                        shift2 += 7;
                    }
                    if (bad)
                        break;
                    unsigned gwt = (unsigned)(t & 7);
                    if (gwt == PB_GROUP)
                        depth++;
                    else if (gwt == PB_ENDGROUP)
                        depth--;
                    else {

                        if (gwt == PB_VARINT) {
                            for (;;) {
                                if (i >= n) {
                                    bad = true;
                                    break;
                                }
                                if (!(p[i++] & 0x80))
                                    break;
                            }
                        } else if (gwt == PB_FIXED64) {
                            if (i + 8 > n)
                                bad = true;
                            else
                                i += 8;
                        } else if (gwt == PB_FIXED32) {
                            if (i + 4 > n)
                                bad = true;
                            else
                                i += 4;
                        } else if (gwt == PB_LEN) {
                            unsigned long long len = 0;
                            unsigned shift3 = 0;
                            for (;;) {
                                if (i >= n || shift3 > 63) {
                                    bad = true;
                                    break;
                                }
                                unsigned char byte = p[i++];
                                len |= (unsigned long long)(byte & 0x7f) << shift3;
                                if (!(byte & 0x80))
                                    break;
                                shift3 += 7;
                            }
                            if (bad || len > n - i) {
                                bad = true;
                                break;
                            }
                            i += (unsigned long)len;
                        } else {
                            bad = true;
                        }
                        if (bad)
                            break;
                    }
                }
                break;
            }
            case PB_ENDGROUP:
            default:
                bad = true;
                break;
        }
        if (bad)
            break;
        if (out) {
            if (count >= cap)
                break;
            pb_occ *o = &out[count];
            o->num = field;
            o->wire_type = wt;
            o->raw = start;
            o->raw_len = (unsigned long)((p + i) - start);
            o->val = val;
            o->val_len = val_len;
            o->varint = varint;
        }
        count++;
    }
    return count;
}

static bool pb_known(const pb_desc *tbl, unsigned ntbl, unsigned field, unsigned wt, bool *repeated) {
    for (unsigned i = 0; i < ntbl; i++) {
        if (tbl[i].num == field) {
            if (tbl[i].wire_type != wt)
                return false;
            *repeated = tbl[i].repeated;
            return true;
        }
    }
    return false;
}


static void pb_canonicalize(const unsigned char *src, unsigned long n, const pb_desc *tbl, unsigned ntbl,
                            pb_buf *out) {
    unsigned long count = pb_walk(src, n, 0, 0);
    if (!count)
        return;
    pb_occ *occ = (pb_occ *)malloc(count * sizeof(pb_occ));
    if (!occ)
        abort();
    count = pb_walk(src, n, occ, count);


    for (unsigned t = 0; t < ntbl; t++) {
        const pb_desc *d = &tbl[t];
        unsigned long last = 0;
        bool have_last = false;
        for (unsigned long i = 0; i < count; i++) {
            if (occ[i].num != d->num || occ[i].wire_type != d->wire_type)
                continue;
            last = i;
            have_last = true;
        }
        if (!have_last)
            continue;
        for (unsigned long i = 0; i < count; i++) {
            if (occ[i].num != d->num || occ[i].wire_type != d->wire_type)
                continue;

            if (!d->repeated && i != last)
                continue;
            switch (d->wire_type) {
                case PB_VARINT:
                    pb_tag(out, d->num, PB_VARINT);
                    pb_varint(out, occ[i].varint);
                    break;
                case PB_FIXED64:
                    pb_fixed_bytes(out, d->num, PB_FIXED64, occ[i].val, 8);
                    break;
                case PB_FIXED32:
                    pb_fixed_bytes(out, d->num, PB_FIXED32, occ[i].val, 4);
                    break;
                case PB_LEN:
                    pb_length_delimited(out, d->num, occ[i].val, occ[i].val_len);
                    break;
                default:
                    pb_bytes(out, occ[i].raw, occ[i].raw_len);
                    break;
            }
        }
    }


    for (unsigned long i = 0; i < count; i++) {
        bool repeated = false;
        if (pb_known(tbl, ntbl, occ[i].num, occ[i].wire_type, &repeated))
            continue;
        switch (occ[i].wire_type) {
            case PB_VARINT:
                pb_tag(out, occ[i].num, PB_VARINT);
                pb_varint(out, occ[i].varint);
                break;
            case PB_FIXED64:
                pb_fixed_bytes(out, occ[i].num, PB_FIXED64, occ[i].val, 8);
                break;
            case PB_FIXED32:
                pb_fixed_bytes(out, occ[i].num, PB_FIXED32, occ[i].val, 4);
                break;
            case PB_LEN:
                pb_length_delimited(out, occ[i].num, occ[i].val, occ[i].val_len);
                break;
            default:
                pb_bytes(out, occ[i].raw, occ[i].raw_len);
                break;
        }
    }
    free(occ);
}






static const unsigned long long kSuidLow = 0x1628e8697d485ecdu;
static const unsigned long long kSuidHigh = 0xb44e6dda82ef9b2bu;

static void build_suid(pb_buf *out) {
    pb_fixed64(out, 1, kSuidLow);
    pb_fixed64(out, 2, kSuidHigh);
}

static void build_client_api_log(pb_buf *out, unsigned long long cntvct, const string_t &pretty,
                                 unsigned oneof_field, const pb_buf *payload) {
    pb_fixed64(out, 1, cntvct);
    pb_length_delimited(out, 2, pretty.data(), pretty.size());
    pb_length_delimited(out, oneof_field, payload->p, payload->len);
}

static void build_sensor_log(pb_buf *out, unsigned id, unsigned long long cntvct,
                             const char *logging_module_type, unsigned long long self, const pb_buf *client) {
    pb_fixed32(out, 1, id);
    pb_fixed64(out, 2, cntvct);
    pb_buf suid;
    pb_init(&suid);
    build_suid(&suid);
    pb_length_delimited(out, 3, suid.p, suid.len);
    pb_free(&suid);
    pb_string(out, 4, logging_module_type);
    pb_fixed64(out, 5, self);
    pb_length_delimited(out, 102, client->p, client->len);
}





#define SNS_LOG(fmt, fmt_nl, func, line, ...)                                          \
    do {                                                                               \
        if (sensors_log::get_level() >= 4) {                                           \
            __android_log_print(2, sensors_log::get_tag(), fmt, func, line,            \
                                ##__VA_ARGS__);                                        \
            if (sensors_log::get_stderr_logging())                                     \
                fprintf(stderr, fmt_nl, func, line, ##__VA_ARGS__);                    \
        }                                                                              \
    } while (0)

#define SNS_LOG_DIAG_FLAG(func, line)                                                  \
    SNS_LOG("%s:%d, diag_flag is false", "%s:%d, diag_flag is false\n", func, line)
#define SNS_LOG_MODULE_TYPE(func, line)                                                \
    SNS_LOG("%s:%d, logging_module_type is not valid",                                 \
            "%s:%d, logging_module_type is not valid\n", func, line)
#define SNS_LOG_LOG_ID_DISABLED(func, line)                                            \
    SNS_LOG("%s:%d, diag_log_id is not enabled", "%s:%d, diag_log_id is not enabled\n", \
            func, line)
#define SNS_LOG_ENDED(func, line)                                                      \
    SNS_LOG("%s:%d, %s Ended", "%s:%d, %s Ended\n", func, line, func)

#define SNS_LOG_LEVEL 4





sensors_diag_log::sensors_diag_log() {
    sensors_timeutil::get_instance();
    timestamp = read_cntvct();
    unsigned char init = (unsigned char)(Diag_LSM_Init(0) & 0xff);
    diag_flag = init != 0;
    if (!init)
        SNS_LOG("%s:%d, diag_init_flag is failed ", "%s:%d, diag_init_flag is failed \n",
                "sensors_diag_log", 76);
    pthread_mutex_init(&mutex, 0);
}

sensors_diag_log::~sensors_diag_log() {
    Diag_LSM_DeInit();
    pthread_mutex_destroy(&mutex);
}

int sensors_diag_log::sensors_diag_log_get_diag_log_id(sensors_diag_logid id) {
    if (id == SENSORS_DIAG_LOG_REQUEST)
        return 6617;
    if (id == SENSORS_DIAG_LOG_EVENT)
        return 6618;
    if (id == SENSORS_DIAG_LOG_RESPONSE)
        return 6655;
    return 0;
}

int sensors_diag_log::submit_log_packet(sensors_diag_logid id, string_t &serialized_log) {
    int log_id;
    if (id == SENSORS_DIAG_LOG_REQUEST)
        log_id = 6617;
    else if (id == SENSORS_DIAG_LOG_EVENT)
        log_id = 6618;
    else if (id == SENSORS_DIAG_LOG_RESPONSE)
        log_id = 6655;
    else
        log_id = 0;


    unsigned char enabled = (unsigned char)(log_status((unsigned short)log_id) & 0xff);
    if (enabled)
        SNS_LOG("%s:%d, %d is enabled for logging", "%s:%d, %d is enabled for logging\n",
                "submit_log_packet", 274, log_id);
    else
        SNS_LOG("%s:%d, %d is disabled for logging", "%s:%d, %d is disabled for logging\n",
                "submit_log_packet", 278, log_id);

    unsigned long len = serialized_log.size();

    unsigned long chunk = ((len >> 5) < 375) ? len : 12000;

    SNS_LOG("%s:%d, max_buffer_size %ld , bytes_written %d",
            "%s:%d, max_buffer_size %ld , bytes_written %d\n", "submit_log_packet", 292,
            12000, (int)chunk);

    void *pkt = log_alloc((unsigned int)log_id, (unsigned int)(chunk + 12));
    if (!pkt) {
        SNS_LOG("%s:%d, failed to alloc log pck for diag",
                "%s:%d, failed to alloc log pck for diag\n", "submit_log_packet", 299);
        return 2;
    }

    if (chunk)
        memcpy((unsigned char *)pkt + 12, serialized_log.data(), chunk);
    log_commit(pkt);

    SNS_LOG("%s:%d, Log pkt submitted to diag", "%s:%d, Log pkt submitted to diag\n",
            "submit_log_packet", 307);
    return 0;
}


static int finish_sensors_diag_log(sensors_diag_log *self, sensors_diag_logid id,
                                   unsigned long long cntvct, const char *logging_module_type,
                                   const pb_buf *client) {
    pb_buf sensor;
    pb_init(&sensor);
    build_sensor_log(&sensor, (unsigned)id, cntvct, logging_module_type,
                     (unsigned long long)(unsigned long)self, client);

    string_storage serialized;
    make_string(&serialized, sensor.p ? sensor.p : (const void *)"", sensor.len);
    pb_free(&sensor);

    int ret = self->submit_log_packet(id, *(string_t *)serialized.raw);
    release_string(&serialized);
    return ret;
}

int sensors_diag_log::log_request_msg(const string_t &request, const string_t &pretty_request,
                                      char *logging_module_type) {
    if (!diag_flag) {
        SNS_LOG_DIAG_FLAG("log_request_msg", 93);
        return 3;
    }
    if (!logging_module_type) {
        SNS_LOG_MODULE_TYPE("log_request_msg", 98);
        return 1;
    }
    if (!(log_status(6617) & 0xff)) {
        SNS_LOG_LOG_ID_DISABLED("log_request_msg", 103);
        return 2;
    }

    pthread_mutex_lock(&mutex);


    pb_buf payload;
    pb_init(&payload);
    pb_canonicalize((const unsigned char *)request.data(), request.size(), kRequestFields,
                    sizeof(kRequestFields) / sizeof(kRequestFields[0]), &payload);

    pb_buf client;
    pb_init(&client);
    build_client_api_log(&client, timestamp, pretty_request, 100, &payload);
    pb_free(&payload);

    int ret = finish_sensors_diag_log(this, SENSORS_DIAG_LOG_REQUEST, read_cntvct(),
                                      logging_module_type, &client);
    pb_free(&client);

    pthread_mutex_unlock(&mutex);

    SNS_LOG_ENDED("log_request_msg", 142);
    return ret;
}

int sensors_diag_log::log_event_msg(const string_t &event, const string_t &pretty_event,
                                    char *logging_module_type) {
    if (!diag_flag) {
        SNS_LOG_DIAG_FLAG("log_event_msg", 153);
        return 3;
    }
    if (!logging_module_type) {
        SNS_LOG_MODULE_TYPE("log_event_msg", 158);
        return 1;
    }
    if (!(log_status(6618) & 0xff)) {
        SNS_LOG_LOG_ID_DISABLED("log_event_msg", 163);
        return 2;
    }

    pthread_mutex_lock(&mutex);

    pb_buf payload;
    pb_init(&payload);
    pb_canonicalize((const unsigned char *)event.data(), event.size(), kEventFields,
                    sizeof(kEventFields) / sizeof(kEventFields[0]), &payload);

    pb_buf client;
    pb_init(&client);
    build_client_api_log(&client, timestamp, pretty_event, 101, &payload);
    pb_free(&payload);


    int ret = finish_sensors_diag_log(this, SENSORS_DIAG_LOG_EVENT, read_cntvct(),
                                      logging_module_type, &client);
    pb_free(&client);

    pthread_mutex_unlock(&mutex);
    return ret;
}

int sensors_diag_log::log_response_msg(unsigned int response_id, const string_t &response,
                                       char *logging_module_type) {
    if (!diag_flag) {
        SNS_LOG_DIAG_FLAG("log_response_msg", 213);
        return 3;
    }
    if (!logging_module_type) {
        SNS_LOG_MODULE_TYPE("log_response_msg", 218);
        return 1;
    }
    if (!(log_status(6655) & 0xff)) {
        SNS_LOG_LOG_ID_DISABLED("log_response_msg", 223);
        return 2;
    }

    pthread_mutex_lock(&mutex);


    pb_buf payload;
    pb_init(&payload);
    pb_tag(&payload, 1, PB_VARINT);
    pb_varint(&payload, (unsigned long long)(long long)(int)response_id);

    pb_buf client;
    pb_init(&client);
    build_client_api_log(&client, timestamp, response, 102, &payload);
    pb_free(&payload);

    int ret = finish_sensors_diag_log(this, SENSORS_DIAG_LOG_RESPONSE, read_cntvct(),
                                      logging_module_type, &client);
    pb_free(&client);

    pthread_mutex_unlock(&mutex);

    SNS_LOG_ENDED("log_response_msg", 261);
    return ret;
}
