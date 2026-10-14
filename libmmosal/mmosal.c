/*
 * Copyright (C) 2025 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdarg.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <time.h>
#include <unistd.h>

#include <android/log.h>
#include <cutils/properties.h>





#define MM_LOG_TAG "MM_OSAL"

#define MM_LOGE(...) \
    __android_log_print(ANDROID_LOG_ERROR, MM_LOG_TAG, __VA_ARGS__)


#define MM_CS_FILE "vendor/qcom/proprietary/commonsys/mm-osal/src/MMCriticalSection.c"
#define MM_TIMER_FILE "vendor/qcom/proprietary/commonsys/mm-osal/src/MMTimer.c"
#define MM_SIGNAL_FILE "vendor/qcom/proprietary/commonsys/mm-osal/src/MMSignal.c"
#define MM_FILE_FILE "vendor/qcom/proprietary/commonsys/mm-osal/src/MMFile.c"


#define MM_ID_THREAD 6013
#define MM_ID_FILE 6022
#define MM_ID_GLOBAL 6035
#define MM_ID_BASE 6000

#define MM_LOG_MASK_COUNT 37
#define MM_LOG_MASK_DEFAULT 0x18










extern void android_set_abort_message(const char *msg) __attribute__((weak));

static void mm_ubsan_report(const char *op, unsigned long long value)
{
    static const char hex[] = "0123456789abcdef";
    char msg[64];
    size_t n;
    int i;

    n = (size_t)snprintf(msg, sizeof(msg), "ubsan: %s by 0x", op);
    for (i = 15; i >= 0; i--)
        msg[n++] = hex[(value >> (i * 4)) & 0xf];
    msg[n++] = '\n';
    msg[n] = 0;

    (void)!write(2, msg, strlen(msg));
    if (android_set_abort_message != NULL)
        android_set_abort_message(msg);
    abort();
}

#define MM_UBSAN_SUB(v) mm_ubsan_report("sub-overflow", (unsigned long long)(v))
#define MM_UBSAN_MUL(v) mm_ubsan_report("mul-overflow", (unsigned long long)(v))
#define MM_UBSAN_NEG(v) mm_ubsan_report("negate-overflow", (unsigned long long)(v))
#define MM_UBSAN_OOB(v) mm_ubsan_report("out-of-bounds", (unsigned long long)(v))





void *MM_malloc(size_t size, const char *file, unsigned long line);
void *MM_calloc(size_t num, size_t size, const char *file, unsigned long line);
void *MM_realloc(void *ptr, size_t size, const char *file, unsigned long line);
void MM_free(void *ptr, const char *file, unsigned long line);
void *MM_new(void *arg);
void MM_delete(void *arg);
int MM_Memory_InitializeCheckPoint(void);
int MM_Memory_ReleaseCheckPoint(void);

void *MM_malloc(size_t size, const char *file, unsigned long line)
{
    (void)file;
    (void)line;
    return malloc(size);
}

void *MM_calloc(size_t num, size_t size, const char *file, unsigned long line)
{
    (void)file;
    (void)line;
    return calloc(num, size);
}

void *MM_realloc(void *ptr, size_t size, const char *file, unsigned long line)
{
    (void)file;
    (void)line;
    if (size == 0)
        return NULL;
    return realloc(ptr, size);
}

void MM_free(void *ptr, const char *file, unsigned long line)
{
    (void)file;
    (void)line;
    free(ptr);
}


void *MM_new(void *arg)
{
    return arg;
}

void MM_delete(void *arg)
{
    (void)arg;
}

int MM_Memory_InitializeCheckPoint(void)
{
    return 1;
}

int MM_Memory_ReleaseCheckPoint(void)
{
    return 1;
}









typedef struct {
    pthread_mutex_t mutex;
    pthread_mutexattr_t attr;
} mm_critical_section;

int MM_CriticalSection_Create(void **handle);
int MM_CriticalSection_Release(void *handle);
int MM_CriticalSection_Enter(void *handle);
int MM_CriticalSection_Leave(void *handle);
int MM_CriticalSection_TryEnter(void *handle);

int MM_CriticalSection_Create(void **handle)
{
    mm_critical_section *cs;
    int rc;

    cs = (mm_critical_section *)MM_malloc(sizeof(*cs), MM_CS_FILE, 63);
    if (cs == NULL)
        return 1;

    rc = pthread_mutexattr_init(&cs->attr);
    if (rc != 0) {
        MM_free(cs, MM_CS_FILE, 86);
        return rc;
    }

    (void)pthread_mutexattr_settype(&cs->attr, PTHREAD_MUTEX_RECURSIVE);
    rc = pthread_mutex_init(&cs->mutex, &cs->attr);
    if (rc != 0) {
        (void)pthread_mutexattr_destroy(&cs->attr);
        MM_free(cs, MM_CS_FILE, 81);
        return rc;
    }

    *handle = cs;
    return 0;
}

int MM_CriticalSection_Release(void *handle)
{
    mm_critical_section *cs = (mm_critical_section *)handle;
    int rc;

    if (cs == NULL)
        return 1;

    (void)pthread_mutexattr_destroy(&cs->attr);
    rc = pthread_mutex_destroy(&cs->mutex);
    MM_free(cs, MM_CS_FILE, 111);
    return rc;
}

int MM_CriticalSection_Enter(void *handle)
{
    mm_critical_section *cs = (mm_critical_section *)handle;

    if (cs == NULL)
        return 1;
    return pthread_mutex_lock(&cs->mutex);
}

int MM_CriticalSection_Leave(void *handle)
{
    mm_critical_section *cs = (mm_critical_section *)handle;

    if (cs == NULL)
        return 1;
    return pthread_mutex_unlock(&cs->mutex);
}

int MM_CriticalSection_TryEnter(void *handle)
{
    mm_critical_section *cs = (mm_critical_section *)handle;

    if (cs == NULL)
        return 1;
    return pthread_mutex_trylock(&cs->mutex);
}





typedef long MM_FILE;

int MM_File_Create(const char *path, int mode, MM_FILE *handle);
int MM_File_Release(MM_FILE handle);
int MM_File_Read(MM_FILE handle, void *buf, unsigned long size,
                 unsigned long *pRead);






pthread_mutex_t mOSALMutex = PTHREAD_MUTEX_INITIALIZER;

static int32_t g_logMask[MM_LOG_MASK_COUNT];
static int g_debugRefCount;

unsigned long std_strlen(const char *s);
char *std_strstr(const char *haystack, const char *needle);


static int mm_snprintf(char *dst, const char *fmt, ...)
{
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = vsnprintf(dst, 32, fmt, ap);
    va_end(ap);
    return n;
}

int MM_Debug_Initialize(void);
int MM_Debug_Initialize_Ex(void);
int MM_Debug_Deinitialize(void);
int GetLogMask(int id);
int GetContext(void);

static const char *const k_logmaskPaths[] = {
    "/data/mmosal_logmask.cfg",
    "/data/misc/media/mmosal_logmask.cfg",
};

static void mm_logmask_defaults(void)
{
    int i;

    for (i = 0; i < MM_LOG_MASK_COUNT; i++) {
        if ((g_logMask[i] & 0x40) == 0)
            g_logMask[i] = MM_LOG_MASK_DEFAULT;
    }
}

int MM_Debug_Initialize(void)
{
    char buf[2048];
    char key[32];
    MM_FILE file;
    unsigned long bytes = 0;
    const char *p;
    int rc;
    int i;

    rc = pthread_mutex_lock(&mOSALMutex);
    if (rc != 0)
        MM_LOGE("Mutex Lock failed in Init, %d", rc);

    if (g_debugRefCount != 0) {
        g_debugRefCount++;
        (void)pthread_mutex_unlock(&mOSALMutex);
        return 0;
    }

    memset(buf, 0, sizeof(buf));
    mm_logmask_defaults();

    for (i = 0; i < 2; i++) {
        file = -1;
        bytes = 0;
        if (MM_File_Create(k_logmaskPaths[i], 0, &file) == 0) {
            rc = MM_File_Read(file, buf, sizeof(buf), &bytes);
            MM_File_Release(file);
            if (rc == 0)
                break;
            bytes = 0;
        }
        if (i == 0)
            MM_LOGE("Open or read fail on /data/mmosal_logmask.cfg. Possible "
                    "permission denied issue. Looking for "
                    "/data/misc/media/mmosal_logmask.cfg");
    }

    if (bytes != 0) {
        if (bytes >= sizeof(buf) + 1)
            MM_UBSAN_OOB(bytes);
        buf[bytes] = 0;

        if (std_strlen(buf) != 0) {
            mm_snprintf(key, "LOGMASK = %d", MM_ID_GLOBAL);
            p = std_strstr(buf, key);
            if (p != NULL) {
                int value = atoi(p + std_strlen(key) + 1) | 0x40;
                for (i = 0; i < MM_LOG_MASK_COUNT; i++)
                    g_logMask[i] = value;
            }

            mm_snprintf(key, "LOGMASK = ");
            p = std_strstr(buf, key);
            while (p != NULL) {
                uint32_t idx;
                int value;

                p += std_strlen(key);
                idx = (uint32_t)atoi(p) - (uint32_t)MM_ID_BASE;
                if (idx <= 0x24u) {
                    value = atoi(p + 5);
                    g_logMask[idx] = value | 0x40;
                }
                p = std_strstr(p, key);
            }
        }
    }

    g_debugRefCount++;
    (void)pthread_mutex_unlock(&mOSALMutex);
    return 0;
}

int MM_Debug_Initialize_Ex(void)
{
    char buf[0x5c + 4];
    int len;
    int i;
    int rc;

    rc = pthread_mutex_lock(&mOSALMutex);
    if (rc != 0)
        MM_LOGE("Mutex Lock failed in Init, %d", rc);

    mm_logmask_defaults();

    memset(buf, 0, sizeof(buf));
    (void)property_get("vendor.debug.mmosal.config", buf, "");

    len = (int)std_strlen(buf);
    i = 0;

    if (len >= 6) {
        for (;;) {
            const char *dp;
            uint32_t idx;
            int value;

            if (i > 0x5b)
                MM_UBSAN_OOB(i);


            dp = buf + i;
            while (*dp >= '0' && *dp <= '9')
                dp++;
            if (*dp != ':') {
                MM_LOGE("Error parsing mmosal.debug.config at %d", i);
                break;
            }

            idx = (uint32_t)atoi(buf + i) - (uint32_t)MM_ID_BASE;
            value = atoi(dp + 1);

            i = (int)(dp - buf) + 1;
            while (buf[i] >= '0' && buf[i] <= '9')
                i++;

            if (idx <= 0x24u && value <= 0x3f) {
                if (idx == 0x23u) {
                    int j;
                    value |= 0x40;
                    for (j = 0; j < MM_LOG_MASK_COUNT; j++)
                        g_logMask[j] = value;
                } else {
                    g_logMask[idx] = value | 0x40;
                    MM_LOGE("%d %d", MM_ID_BASE + (int)idx, value);
                }
            }

            if (i >= len)
                break;
            if (buf[i] == 0)
                break;
            if (buf[i] != ',')
                break;
            i++;
        }
    }

    g_debugRefCount++;
    (void)pthread_mutex_unlock(&mOSALMutex);
    return 0;
}

int MM_Debug_Deinitialize(void)
{
    int rc;

    rc = pthread_mutex_lock(&mOSALMutex);
    if (rc != 0)
        MM_LOGE("Mutex Lock failed in Deinit, %d", rc);

    if (g_debugRefCount != 0)
        g_debugRefCount--;

    rc = pthread_mutex_unlock(&mOSALMutex);
    if (rc != 0)
        MM_LOGE("Mutex UnLock failed in Deinit, %d", rc);

    return 0;
}

int GetLogMask(int id)
{
    uint32_t idx;

    if (g_debugRefCount < 1)
        return 0x1c;

    idx = (uint32_t)id - (uint32_t)MM_ID_BASE;
#if defined(__LP64__)

    if (id < MM_ID_BASE)
        MM_UBSAN_SUB((uint32_t)MM_ID_BASE - (uint32_t)id);
    if (idx >= 0x25u) {
        MM_UBSAN_OOB(idx);
        return 0;
    }
#else
    (void)idx;
#endif

    return g_logMask[id - MM_ID_BASE];
}

int GetContext(void)
{
    return 0;
}





typedef struct {
    int32_t year;
    int32_t month;
    int32_t dayOfWeek;
    int32_t day;
    int32_t hour;
    int32_t minute;
    int32_t second;
    long milliSecs;
} mm_time;

int MM_Time_GetTime(long *pTime);
int MM_Time_GetTimeEx(long long *pTime);
int MM_Time_GetLocalTime(mm_time *pTime);
int MM_Time_GetUTCTime(mm_time *pTime);
int MM_Time_GetCurrentTimeInMilliSecsFromEpoch(unsigned long long *pTime);
void MM_Timer_Sleep(long ms);

int MM_Time_GetTime(long *pTime)
{
    struct timespec ts;

    if (pTime == NULL)
        return 1;

    ts.tv_sec = 0;
    ts.tv_nsec = 0;
    (void)clock_gettime(CLOCK_MONOTONIC, &ts);
    *pTime = (long)ts.tv_sec * 1000L + (long)ts.tv_nsec / 1000000L;
    return 0;
}

int MM_Time_GetTimeEx(long long *pTime)
{
    struct timespec ts;

    if (pTime == NULL)
        return 1;

    ts.tv_sec = 0;
    ts.tv_nsec = 0;
    (void)clock_gettime(CLOCK_MONOTONIC, &ts);
    *pTime = (long)ts.tv_sec * 1000L + (long)ts.tv_nsec / 1000000L;
    return 0;
}

static int mm_time_fill(mm_time *pTime, const struct tm *tm, long msecs)
{
    if (pTime == NULL)
        return 1;

    pTime->year = tm->tm_year + 1900;
    pTime->month = tm->tm_mon + 1;
    pTime->dayOfWeek = tm->tm_wday;
    pTime->day = tm->tm_mday;
    pTime->hour = tm->tm_hour;
    pTime->minute = tm->tm_min;
    pTime->second = tm->tm_sec;
    pTime->milliSecs = msecs;
    return 0;
}

int MM_Time_GetLocalTime(mm_time *pTime)
{
    struct timeval tv;
    struct timezone tz;
    struct tm tm;

    if (pTime == NULL)
        return 1;

    memset(&tm, 0, sizeof(tm));
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    tz.tz_minuteswest = 0;
    tz.tz_dsttime = 0;
    (void)gettimeofday(&tv, &tz);
    (void)localtime_r(&tv.tv_sec, &tm);
    return mm_time_fill(pTime, &tm, (long)(tv.tv_usec / 1000));
}

int MM_Time_GetUTCTime(mm_time *pTime)
{
    struct timeval tv;
    struct timezone tz;
    struct tm tm;

    if (pTime == NULL)
        return 1;

    memset(&tm, 0, sizeof(tm));
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    tz.tz_minuteswest = 0;
    tz.tz_dsttime = 0;
    (void)gettimeofday(&tv, &tz);
    (void)gmtime_r(&tv.tv_sec, &tm);
    return mm_time_fill(pTime, &tm, (long)(tv.tv_usec / 1000));
}

int MM_Time_GetCurrentTimeInMilliSecsFromEpoch(unsigned long long *pTime)
{
    struct timeval tv;

    if (pTime == NULL)
        return 1;

    tv.tv_sec = 0;
    tv.tv_usec = 0;
    (void)gettimeofday(&tv, NULL);
    *pTime = (unsigned long long)tv.tv_sec * 1000ULL +
             (unsigned long long)tv.tv_usec / 1000ULL;
    return 0;
}

void MM_Timer_Sleep(long ms)
{
    struct timespec ts;

    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    (void)nanosleep(&ts, NULL);
}





typedef struct {
    void *pfn;
    void *pdata;
    mm_critical_section *pcs;
    timer_t id;
    int repeat;
} mm_timer;

void video_Timer_Callback(union sigval value);

int MM_Timer_CreateEx(int repeat, void *pfn, void *pdata, void **pHandle);
int MM_Timer_Create(long ms, int repeat, void *pfn, void *pdata, void **pHandle);
int MM_Timer_Release(void *handle);
int MM_Timer_Start(void *handle, long ms);
int MM_Timer_StartEx(void *handle, long sec, long nsec);
int MM_Timer_Stop(void *handle);

void video_Timer_Callback(union sigval value)
{
    mm_timer *timer = (mm_timer *)value.sival_ptr;

    if (timer == NULL)
        return;

    MM_CriticalSection_Enter(timer->pcs);
    ((void (*)(void *))timer->pfn)(timer->pdata);
    MM_CriticalSection_Leave(timer->pcs);
}

int MM_Timer_CreateEx(int repeat, void *pfn, void *pdata, void **pHandle)
{
    mm_timer *timer;
    struct sigevent sev;
    int rc;

    if (pfn == NULL || pHandle == NULL)
        return 1;

    timer = (mm_timer *)MM_malloc(sizeof(*timer), MM_TIMER_FILE, 173);
    if (timer == NULL)
        return 1;

    memset(timer, 0, sizeof(*timer));
    timer->repeat = repeat;
    timer->pfn = pfn;
    timer->pdata = pdata;

    rc = MM_CriticalSection_Create((void **)&timer->pcs);
    if (rc != 0) {
        MM_free(timer, MM_TIMER_FILE, 208);
        return 1;
    }

    memset(&sev, 0, sizeof(sev));
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_value.sival_ptr = timer;
    sev.sigev_notify_function = video_Timer_Callback;

    rc = timer_create(CLOCK_MONOTONIC, &sev, &timer->id);
    if (rc != 0) {
        MM_CriticalSection_Release(timer->pcs);
        timer->pcs = NULL;
        MM_free(timer, MM_TIMER_FILE, 203);
        return rc;
    }

    *pHandle = timer;
    if (GetLogMask(MM_ID_THREAD) & 0x10)
        MM_LOGE("timer_create: timer_id %d", (int)(intptr_t)timer->id);
    return 0;
}

int MM_Timer_Release(void *handle)
{
    mm_timer *timer = (mm_timer *)handle;

    if (timer == NULL)
        return 1;

    MM_CriticalSection_Enter(timer->pcs);
    if (GetLogMask(MM_ID_THREAD) & 0x10)
        MM_LOGE("timer_delete: timer_id %d", (int)(intptr_t)timer->id);
    (void)timer_delete(timer->id);
    MM_CriticalSection_Leave(timer->pcs);
    MM_CriticalSection_Release(timer->pcs);
    timer->pcs = NULL;
    MM_free(timer, MM_TIMER_FILE, 350);
    return 0;
}

static int mm_timer_arm_rc(mm_timer *timer, long sec, long nsec)
{
    struct itimerspec its;
    int rc;

    memset(&its, 0, sizeof(its));
    its.it_value.tv_sec = sec;
    its.it_value.tv_nsec = nsec;
    if (timer->repeat)
        its.it_interval = its.it_value;
    rc = timer_settime(timer->id, 0, &its, NULL);
    return rc;
}

int MM_Timer_Start(void *handle, long ms)
{
    mm_timer *timer = (mm_timer *)handle;
    int rc;

    if (timer == NULL)
        return 1;

    MM_CriticalSection_Enter(timer->pcs);
    rc = mm_timer_arm_rc(timer, ms / 1000, (ms % 1000) * 1000000L);
    MM_CriticalSection_Leave(timer->pcs);
    return rc;
}

int MM_Timer_StartEx(void *handle, long sec, long nsec)
{
    mm_timer *timer = (mm_timer *)handle;
    int rc;

    if (timer == NULL)
        return 1;

    MM_CriticalSection_Enter(timer->pcs);
    rc = mm_timer_arm_rc(timer, sec, nsec);
    MM_CriticalSection_Leave(timer->pcs);
    return rc;
}

int MM_Timer_Stop(void *handle)
{
    mm_timer *timer = (mm_timer *)handle;
    struct itimerspec its;
    int rc;

    if (timer == NULL)
        return 1;

    memset(&its, 0, sizeof(its));
    MM_CriticalSection_Enter(timer->pcs);
    rc = timer_settime(timer->id, 0, &its, NULL);
    MM_CriticalSection_Leave(timer->pcs);
    return rc;
}

int MM_Timer_Create(long ms, int repeat, void *pfn, void *pdata, void **pHandle)
{
    mm_timer *timer;
    mm_timer *release;
    int rc;

    rc = MM_Timer_CreateEx(repeat, pfn, pdata, pHandle);
    if (rc != 0)
        return rc;

    timer = (mm_timer *)*pHandle;
    if (timer == NULL) {
        release = NULL;
        rc = 1;
    } else {
        MM_CriticalSection_Enter(timer->pcs);
        rc = mm_timer_arm_rc(timer, ms / 1000, (ms % 1000) * 1000000L);
        MM_CriticalSection_Leave(timer->pcs);
        if (rc == 0)
            return 0;
        release = timer;
    }

    (void)MM_Timer_Release(release);
    *pHandle = NULL;
    return rc;
}





int MM_Thread_CreateEx(int priority, const char *name, void *(*pfn)(void *),
                       void *arg, int stackSize, int flags, pthread_t *pHandle);
int MM_Thread_Create(int priority, const char *name, void *(*pfn)(void *),
                     void *arg, int stackSize, pthread_t *pHandle);
int MM_Thread_Release(pthread_t handle);
int MM_Thread_Join(pthread_t handle, int *pStatus);
void MM_Thread_Exit(int status);
int MM_Thread_Detach(pthread_t handle);
int MM_Thread_Suspend(pthread_t handle);
int MM_Thread_Resume(pthread_t handle);
int MM_Thread_GetPriority(pthread_t handle);
int MM_Thread_SetPriority(pthread_t handle, int priority);
int MM_Thread_GetCurrent(pthread_t *pHandle);
int MM_Thread_Compare(pthread_t a, pthread_t b);

const int MM_Thread_DefaultPriority = 0;

int MM_Thread_CreateEx(int priority, const char *name, void *(*pfn)(void *),
                       void *arg, int stackSize, int flags, pthread_t *pHandle)
{
    pthread_attr_t attr;
    struct sched_param param;
    size_t curStack;
    int rc;

    (void)name;
    (void)flags;

    memset(&attr, 0, sizeof(attr));
    rc = pthread_attr_init(&attr);
    if (rc != 0) {
        if (GetLogMask(MM_ID_THREAD) & 0x8)
            MM_LOGE("MM_Thread_CreateEx::attr init failed %d", rc);
        return rc;
    }

    rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    if (rc != 0) {
        if (GetLogMask(MM_ID_THREAD) & 0x8)
            MM_LOGE("MM_Thread_CreateEx::set detach state failed %d", rc);
        (void)pthread_attr_destroy(&attr);
        return rc;
    }

    param.sched_priority = priority;
    rc = pthread_attr_setschedparam(&attr, &param);
    if (rc != 0) {
        (void)pthread_attr_destroy(&attr);
        return rc;
    }

    curStack = 0;
    (void)pthread_attr_getstacksize(&attr, &curStack);
    if (curStack < (size_t)stackSize) {
        rc = pthread_attr_setstacksize(&attr, (size_t)stackSize);
        if (rc != 0) {
            if (GetLogMask(MM_ID_THREAD) & 0x8)
                MM_LOGE("MM_Thread_CreateEx::set stack size failed %d", rc);
            (void)pthread_attr_destroy(&attr);
            return rc;
        }
        curStack = (size_t)stackSize;
    }

    rc = pthread_create(pHandle, &attr, pfn, arg);
    if (rc != 0) {
        if (GetLogMask(MM_ID_THREAD) & 0x8)
            MM_LOGE("MM_Thread_CreateEx::create failed %d", rc);
    } else if (GetLogMask(MM_ID_THREAD) & 0x4) {
        MM_LOGE("MM_Thread_CreateEx::thread 0x%lu created with stack %d bytes",
                (unsigned long)*pHandle, (int)curStack);
    }
    (void)pthread_attr_destroy(&attr);
    return rc;
}

int MM_Thread_Create(int priority, const char *name, void *(*pfn)(void *),
                     void *arg, int stackSize, pthread_t *pHandle)
{
    return MM_Thread_CreateEx(priority, name, pfn, arg, stackSize, 0, pHandle);
}

int MM_Thread_Release(pthread_t handle)
{
    (void)handle;
    return 0;
}

int MM_Thread_Join(pthread_t handle, int *pStatus)
{
    void *retval = NULL;
    int rc;

    if (handle == 0)
        return 1;

    rc = pthread_join(handle, &retval);
    if (pStatus != NULL)
        *pStatus = (int)(intptr_t)retval;
    return rc;
}

void MM_Thread_Exit(int status)
{
    pthread_exit(&status);
}

int MM_Thread_Detach(pthread_t handle)
{
    int rc;
    int mask;

    rc = pthread_detach(handle);
    mask = GetLogMask(MM_ID_THREAD);
    if (rc != 0) {
        if (mask & 0x8)
            MM_LOGE("pthread_detach(): failed %d", rc);
        return 1;
    }
    if (mask & 0x1)
        MM_LOGE("pthread_detach(): success");
    return 0;
}

int MM_Thread_Suspend(pthread_t handle)
{
    (void)handle;
    return 1;
}

int MM_Thread_Resume(pthread_t handle)
{
    (void)handle;
    return 1;
}

int MM_Thread_GetPriority(pthread_t handle)
{
    (void)handle;
    return 1;
}

int MM_Thread_SetPriority(pthread_t handle, int priority)
{
    (void)handle;
    (void)priority;
    return 1;
}

int MM_Thread_GetCurrent(pthread_t *pHandle)
{
    *pHandle = pthread_self();
    return 0;
}

int MM_Thread_Compare(pthread_t a, pthread_t b)
{
    return pthread_equal(a, b);
}














typedef struct mm_signal_slot {
    struct mm_signal_slot *pNext;
    int kind;
    uint8_t inUse;
    uint8_t signalled;
    uint8_t pad[2];
    void *pArg;
    void *pHandler;
} mm_signal_slot;

#define MM_SLOT_COUNT 32

typedef struct {
    mm_signal_slot nodes[MM_SLOT_COUNT];
    void *pField0;
    void *pField1;
    uintptr_t field2;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} mm_signalq;

int MM_SignalQ_Create(void **pHandle);
int MM_SignalQ_Release(void *handle);
int MM_SignalQ_Wait(void *handle, void **ppData);
int MM_SignalQ_WaitEx(void *handle, void **ppData, void **handles, int count);
int MM_SignalQ_TimedWait(void *handle, int ms, void **ppData, int *pStatus);
int MM_SignalQ_TimedWaitEx(void *handle, int ms, void **ppData, int *pStatus,
                           void **handles, int count);

int MM_Signal_Create(void *qh, void *pArg, void *pHandler, void **pHandle);
int MM_Signal_CreateEx(void *qh, void *pArg, void *pHandler, int kind,
                       void **pHandle);
int MM_Signal_Release(void *handle);
int MM_Signal_Set(void *handle);
int MM_Signal_Reset(void *handle);


static mm_signal_slot *mm_slot_of(void *handle)
{
    return (mm_signal_slot *)((char *)handle - sizeof(void *));
}

static mm_signalq *mm_signalq_of(void *handle)
{
    return *(mm_signalq **)((char *)handle + sizeof(mm_signal_slot) -
                            sizeof(void *));
}

int MM_SignalQ_Create(void **pHandle)
{
    mm_signalq *q;
    int i;

    if (pHandle == NULL)
        return 1;

    q = (mm_signalq *)MM_malloc(sizeof(*q), MM_SIGNAL_FILE, 156);
    if (q == NULL)
        return 1;

    memset(q, 0, sizeof(*q));
    ((uint8_t *)q)[0] = 1;

    (void)pthread_cond_init(&q->cond, NULL);
    (void)pthread_mutex_init(&q->mutex, NULL);

    for (i = 1; i <= MM_SLOT_COUNT; i++)
        *(void **)((char *)q + i * sizeof(mm_signal_slot)) = q;

    *pHandle = q;
    return 0;
}

int MM_SignalQ_Release(void *handle)
{
    mm_signalq *q = (mm_signalq *)handle;
    int i;

    if (q == NULL)
        return 1;

    (void)pthread_mutex_lock(&q->mutex);
    ((uint8_t *)q)[0] = 0;

    for (i = 0; i < MM_SLOT_COUNT; i++) {
        if (q->nodes[i].inUse) {
            (void)pthread_mutex_unlock(&q->mutex);
            return 0;
        }
    }

    (void)pthread_mutex_unlock(&q->mutex);
    (void)pthread_cond_destroy(&q->cond);
    (void)pthread_mutex_destroy(&q->mutex);
    MM_free(q, MM_SIGNAL_FILE, 236);
    return 0;
}

static int mm_signal_claim(mm_signalq *q, void *pArg, void *pHandler,
                           int kind, void **pHandle)
{
    int i;

    for (i = 0; i < MM_SLOT_COUNT; i++) {
        mm_signal_slot *slot = &q->nodes[i];
        if (slot->inUse || slot->signalled)
            continue;
        slot->kind = kind;
        slot->inUse = 1;
        slot->pArg = pArg;
        slot->pHandler = pHandler;
        *pHandle = &slot->kind;
        return 0;
    }
    return 1;
}

int MM_Signal_Create(void *qh, void *pArg, void *pHandler, void **pHandle)
{
    mm_signalq *q = (mm_signalq *)qh;
    int rc = 1;

    if (q == NULL || pHandle == NULL)
        return 1;

    (void)pthread_mutex_lock(&q->mutex);
    if (((uint8_t *)q)[0] != 0)
        rc = mm_signal_claim(q, pArg, pHandler, 1, pHandle);
    (void)pthread_mutex_unlock(&q->mutex);
    return rc;
}

int MM_Signal_CreateEx(void *qh, void *pArg, void *pHandler, int kind,
                       void **pHandle)
{
    mm_signalq *q = (mm_signalq *)qh;
    int rc = 1;

    if (q == NULL || pHandle == NULL)
        return 1;

    (void)pthread_mutex_lock(&q->mutex);
    if (((uint8_t *)q)[0] != 0)
        rc = mm_signal_claim(q, pArg, pHandler, kind, pHandle);
    (void)pthread_mutex_unlock(&q->mutex);
    return rc;
}

int MM_Signal_Release(void *handle)
{
    mm_signal_slot *slot = mm_slot_of(handle);
    mm_signalq *q;

    if (handle == NULL)
        return 1;

    q = mm_signalq_of(handle);
    if (q == NULL)
        return 1;

    (void)pthread_mutex_lock(&q->mutex);
    if (slot->inUse) {
        slot->inUse = 0;
        if (((uint8_t *)q)[0] == 0)
            (void)MM_SignalQ_Release(q);
    }
    (void)pthread_mutex_unlock(&q->mutex);
    return 0;
}

int MM_Signal_Set(void *handle)
{
    mm_signal_slot *slot = mm_slot_of(handle);
    mm_signalq *q;
    int rc = 1;

    if (handle == NULL)
        return 1;

    q = mm_signalq_of(handle);
    if (q == NULL)
        return 1;

    (void)pthread_mutex_lock(&q->mutex);
    if (slot->inUse != 0 && ((uint8_t *)q)[0] != 0) {
        slot->signalled = 1;
        (void)pthread_cond_signal(&q->cond);
        rc = 0;
    }
    (void)pthread_mutex_unlock(&q->mutex);
    return rc;
}

int MM_Signal_Reset(void *handle)
{
    mm_signal_slot *slot = mm_slot_of(handle);
    mm_signalq *q;

    if (handle == NULL)
        return 1;

    q = mm_signalq_of(handle);
    if (q == NULL)
        return 1;

    (void)pthread_mutex_lock(&q->mutex);
    slot->signalled = 0;
    (void)pthread_mutex_unlock(&q->mutex);
    return 0;
}


static int mm_signal_dispatch(mm_signal_slot *slot, void **ppData)
{
    if (slot->kind == 1)
        slot->signalled = 0;

    if (slot->pHandler != NULL) {
        ((void (*)(void *))slot->pHandler)(slot->pArg);
        return 0;
    }

    if (slot->pArg != NULL && ppData != NULL)
        *ppData = slot->pArg;
    return 0;
}


static int mm_signalq_scan(mm_signalq *q, void **handles, int count)
{
    int i;

    if (count >= 1) {
        for (i = 0; i < count; i++)
            if (mm_slot_of(handles[i])->signalled)
                return i;
    }
    return count;
}

int MM_SignalQ_Wait(void *handle, void **ppData)
{
    mm_signalq *q = (mm_signalq *)handle;
    int i;
    int rc;

    if (q == NULL)
        return 1;

    (void)pthread_mutex_lock(&q->mutex);

    for (;;) {
        for (i = 0; i < MM_SLOT_COUNT; i++)
            if (q->nodes[i].signalled)
                break;
        if (i != MM_SLOT_COUNT)
            break;
        (void)pthread_cond_wait(&q->cond, &q->mutex);
    }

    if (i > MM_SLOT_COUNT - 1) {
        (void)pthread_mutex_unlock(&q->mutex);
        return 1;
    }

    rc = mm_signal_dispatch(&q->nodes[i], ppData);
    (void)pthread_mutex_unlock(&q->mutex);
    return rc;
}

int MM_SignalQ_WaitEx(void *handle, void **ppData, void **handles, int count)
{
    mm_signalq *q = (mm_signalq *)handle;
    int i;
    int rc;

    if (count >= 1) {
        for (i = 0; i < count; i++)
            if (handles[i] == NULL)
                return 1;
    }

    if (q == NULL)
        return 0;

    (void)pthread_mutex_lock(&q->mutex);

    for (;;) {
        i = mm_signalq_scan(q, handles, count);
        if (i != count)
            break;
        (void)pthread_cond_wait(&q->cond, &q->mutex);
    }

    if (i >= count) {
        (void)pthread_mutex_unlock(&q->mutex);
        return 1;
    }

    rc = mm_signal_dispatch(mm_slot_of(handles[i]), ppData);
    (void)pthread_mutex_unlock(&q->mutex);
    return rc;
}

int MM_SignalQ_TimedWait(void *handle, int ms, void **ppData, int *pStatus)
{
    mm_signalq *q = (mm_signalq *)handle;
    long sec;
    long nsec;
    int i;
    int timedOut = 0;
    int rc;

    if (q == NULL)
        return 1;
    if (pStatus == NULL)
        return 1;

    (void)pthread_mutex_lock(&q->mutex);
    *pStatus = 0;

    i = 0;
    while (i < MM_SLOT_COUNT && !q->nodes[i].signalled)
        i++;

    if (i == MM_SLOT_COUNT) {
        sec = ms / 1000;
        nsec = (ms % 1000) * 1000000L;
        for (;;) {
            struct timespec ts;
            int r;

            if (timedOut) {
                *pStatus = 1;
                (void)pthread_mutex_unlock(&q->mutex);
                return 0;
            }

            ts.tv_sec = 0;
            ts.tv_nsec = 0;
            (void)clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += sec;
            ts.tv_nsec += nsec;
            if (ts.tv_nsec >= 1000000000L) {
                ts.tv_sec++;
                ts.tv_nsec -= 1000000000L;
            }

            r = pthread_cond_timedwait(&q->cond, &q->mutex, &ts);
            if (r == ETIMEDOUT) {
                timedOut = 1;
                continue;
            }

            i = 0;
            while (i < MM_SLOT_COUNT && !q->nodes[i].signalled)
                i++;
            if (i != MM_SLOT_COUNT)
                break;
        }
    }

    if (*pStatus == 1) {
        (void)pthread_mutex_unlock(&q->mutex);
        return 0;
    }
    if (i > MM_SLOT_COUNT - 1) {
        (void)pthread_mutex_unlock(&q->mutex);
        return 1;
    }

    rc = mm_signal_dispatch(&q->nodes[i], ppData);
    (void)pthread_mutex_unlock(&q->mutex);
    return rc;
}

int MM_SignalQ_TimedWaitEx(void *handle, int ms, void **ppData, int *pStatus,
                           void **handles, int count)
{
    mm_signalq *q = (mm_signalq *)handle;
    long sec;
    long nsec;
    int i;
    int rc;

    if (count >= 1) {
        for (i = 0; i < count; i++)
            if (handles[i] == NULL)
                return 1;
    }

    if (q == NULL)
        return 0;
    if (pStatus == NULL)
        return 0;

    (void)pthread_mutex_lock(&q->mutex);
    *pStatus = 0;

    i = mm_signalq_scan(q, handles, count);

    if (i == count) {
        sec = ms / 1000;
        nsec = (ms % 1000) * 1000000L;
        for (;;) {
            struct timespec ts;
            int r;

            if (*pStatus != 0)
                break;

            ts.tv_sec = 0;
            ts.tv_nsec = 0;
            (void)clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += sec;
            ts.tv_nsec += nsec;
            if (ts.tv_nsec >= 1000000000L) {
                ts.tv_sec++;
                ts.tv_nsec -= 1000000000L;
            }

            r = pthread_cond_timedwait(&q->cond, &q->mutex, &ts);
            if (r == ETIMEDOUT) {
                *pStatus = 1;
                i = count;
                break;
            }

            i = mm_signalq_scan(q, handles, count);
            if (i != count)
                break;
        }
    }

    if (*pStatus == 1) {
        (void)pthread_mutex_unlock(&q->mutex);
        return 0;
    }
    if (i >= count) {
        (void)pthread_mutex_unlock(&q->mutex);
        return 1;
    }

    rc = mm_signal_dispatch(mm_slot_of(handles[i]), ppData);
    (void)pthread_mutex_unlock(&q->mutex);
    return rc;
}





int MM_File_CreateW(const uint32_t *path, int mode, MM_FILE *handle);
int MM_File_Read(MM_FILE handle, void *buf, unsigned long size,
                 unsigned long *pRead);
int MM_File_Write(MM_FILE handle, const void *buf, unsigned long size,
                  unsigned long *pWritten);
int MM_File_Seek(MM_FILE handle, long offset, int whence);
int MM_File_SeekEx(MM_FILE handle, long long offset, int whence);
int MM_File_Truncate(MM_FILE handle, long length);
int MM_File_Copy(const char *src, const char *dst);
int MM_File_Move(const char *oldPath, const char *newPath);
int MM_File_Delete(const char *path);
int MM_File_GetSize(MM_FILE handle, unsigned long *pSize);
int MM_File_GetSizeEx(MM_FILE handle, unsigned long long *pSize);
int MM_File_GetFreeSpace(const char *path, unsigned long long *pSize);
int MM_File_GetFreeSpaceEx(const char *path, unsigned long long *pSize);
int MM_File_GetCurrentPosition(MM_FILE handle, unsigned long *pPos);
int MM_File_GetCurrentPositionEx(MM_FILE handle, unsigned long long *pPos);

static int mm_file_flags(int mode)
{
    static const int k_flags[4] = {
        O_RDWR,
        O_WRONLY | O_CREAT | O_TRUNC,
        O_RDWR | O_CREAT | O_TRUNC,
        O_WRONLY | O_CREAT | O_APPEND,
    };

    if ((unsigned)(mode - 1) > 3u)
        return 0;
    return k_flags[mode - 1];
}

int MM_File_Create(const char *path, int mode, MM_FILE *handle)
{
    int fd;
    int flags = mm_file_flags(mode);

    if (handle == NULL)
        return 1;

    fd = open(path, flags, 0777);
    *handle = (MM_FILE)fd;
    if (fd >= 0)
        return 0;

    if (GetLogMask(MM_ID_FILE) & 0x8)
        MM_LOGE("MM_File_Create failed .Efs Error No %d , File Name %s , Mode %d",
                fd, path, mode);
    return 1;
}


int MM_File_CreateW(const uint32_t *path, int mode, MM_FILE *handle)
{
    const uint8_t *raw = (const uint8_t *)path;
    unsigned long chars = 0;
    unsigned long i;
    char *narrow;
    const uint32_t *src;
    char *dst;
    int fd;
    int flags;

    if (raw[0] != 0) {
        for (i = 1; raw[i * 4] != 0; i++)
            ;
        chars = i;
    }

    narrow = (char *)MM_malloc(chars + 1, MM_FILE_FILE, 194);
    if (narrow == NULL)
        return 1;

    if (handle != NULL) {
        flags = mm_file_flags(mode);
        fd = open(narrow, flags, 0777);
        *handle = (MM_FILE)fd;
        if (fd >= 0) {
            MM_free(narrow, MM_FILE_FILE, 213);
            return 0;
        }
        if (GetLogMask(MM_ID_FILE) & 0x8)
            MM_LOGE("MM_File_Create failed .Efs Error No %d , File Name %s , "
                    "Mode %d", fd, narrow, mode);
    }

    src = path;
    dst = narrow;
    while ((*src & 0xffff) != 0)
        *dst++ = (char)*src++;
    *dst = 0;

    if (GetLogMask(MM_ID_FILE) & 0x8)
        MM_LOGE("MM_File_Create Failed Once  %s , Mode %d", narrow, mode);

    if (handle != NULL) {
        flags = mm_file_flags(mode);
        fd = open(narrow, flags, 0777);
        *handle = (MM_FILE)fd;
        if (fd >= 0) {
            MM_free(narrow, MM_FILE_FILE, 213);
            return 0;
        }
        if (GetLogMask(MM_ID_FILE) & 0x8)
            MM_LOGE("MM_File_Create failed .Efs Error No %d , File Name %s , "
                    "Mode %d", fd, narrow, mode);
    }

    MM_free(narrow, MM_FILE_FILE, 213);
    return 1;
}

int MM_File_Release(MM_FILE handle)
{
    if (handle < 0)
        return 1;
    return close((int)handle);
}

int MM_File_Read(MM_FILE handle, void *buf, unsigned long size,
                 unsigned long *pRead)
{
    ssize_t n;

    if (handle < 0)
        return 1;
    if (pRead == NULL)
        return 1;

    n = read((int)handle, buf, size);
    if (n < 0)
        return 1;
    *pRead = (unsigned long)n;
    return 0;
}

int MM_File_Write(MM_FILE handle, const void *buf, unsigned long size,
                  unsigned long *pWritten)
{
    ssize_t n;

    if (handle < 0)
        return 1;
    if (pWritten == NULL)
        return 1;

    n = write((int)handle, buf, size);
    if (n < 0)
        return 1;
    *pWritten = (unsigned long)n;
    return 0;
}

static int mm_file_whence(int whence)
{
    if (whence == 0)
        return 0;
    if (whence == 2)
        return 2;
    return 1;
}

int MM_File_Seek(MM_FILE handle, long offset, int whence)
{
    if (handle < 0)
        return 1;
    return lseek((int)handle, offset, mm_file_whence(whence)) != offset;
}

int MM_File_SeekEx(MM_FILE handle, long long offset, int whence)
{
    if (handle < 0)
        return 1;
    return lseek64((int)handle, (off64_t)offset, mm_file_whence(whence)) !=
           (off64_t)offset;
}

int MM_File_Truncate(MM_FILE handle, long length)
{
    return ftruncate((int)handle, (off_t)length);
}

int MM_File_Copy(const char *src, const char *dst)
{
    (void)src;
    (void)dst;
    return 1;
}

int MM_File_Move(const char *oldPath, const char *newPath)
{
    return rename(oldPath, newPath);
}

int MM_File_Delete(const char *path)
{
    return unlink(path);
}

int MM_File_GetSize(MM_FILE handle, unsigned long *pSize)
{
    struct stat st;

    if (handle < 0)
        return 1;
    if (pSize == NULL)
        return 1;

    memset(&st, 0, sizeof(st));
    if (fstat((int)handle, &st) != 0)
        return 1;
    *pSize = (unsigned long)st.st_size;
    return 0;
}

int MM_File_GetSizeEx(MM_FILE handle, unsigned long long *pSize)
{
    struct stat64 st;

    if (handle < 0)
        return 1;
    if (pSize == NULL)
        return 1;

    memset(&st, 0, sizeof(st));
    if (fstat64((int)handle, &st) != 0)
        return 1;
    *pSize = (unsigned long long)st.st_size;
    return 0;
}

static int mm_file_free_space(const char *path, unsigned long long *pSize,
                              int extended)
{
    struct statfs st;
    unsigned long long total;

    if (path == NULL || pSize == NULL)
        return 1;

    memset(&st, 0, sizeof(st));
    if (extended)
        (void)statfs64(path, (struct statfs64 *)&st);
    else
        (void)statfs(path, &st);

    total = (unsigned long long)st.f_bsize * (unsigned long long)st.f_bavail;
#ifdef __LP64__
    {
        unsigned __int128 wide = (unsigned __int128)st.f_bsize * st.f_bavail;
        if ((wide >> 64) != 0)
            MM_UBSAN_MUL((unsigned long long)(wide >> 64));
    }
#endif
    *pSize = total;
    return 0;
}

int MM_File_GetFreeSpace(const char *path, unsigned long long *pSize)
{
    return mm_file_free_space(path, pSize, 0);
}

int MM_File_GetFreeSpaceEx(const char *path, unsigned long long *pSize)
{
    return mm_file_free_space(path, pSize, 1);
}

int MM_File_GetCurrentPosition(MM_FILE handle, unsigned long *pPos)
{
    off_t pos;

    if (handle < 0)
        return 1;
    if (pPos == NULL)
        return 1;

    pos = lseek((int)handle, 0, SEEK_CUR);
    *pPos = (unsigned long)pos;
    return pos == 0;
}

int MM_File_GetCurrentPositionEx(MM_FILE handle, unsigned long long *pPos)
{
    off64_t pos;

    if (handle < 0)
        return 1;
    if (pPos == NULL)
        return 1;

    pos = lseek64((int)handle, 0, SEEK_CUR);
    *pPos = (unsigned long long)pos;
    return pos == 0;
}





int std_getversion(char *buf, unsigned long size);
unsigned long std_strlcpy(char *dst, const char *src, unsigned long size);
unsigned long std_strlcat(char *dst, const char *src, unsigned long size);
int std_tolower(int c);
int std_toupper(int c);
unsigned long std_strlen(const char *s);
long std_strncmp(const char *a, const char *b, unsigned long n);
long std_strcmp(const char *a, const char *b);
long std_strnicmp(const char *a, const char *b, unsigned long n);
long std_stricmp(const char *a, const char *b);
void *std_memmove(void *dst, const void *src, unsigned long n);
long std_memcmp(const void *a, const void *b, unsigned long n);
void *std_memchr(const void *p, int c, unsigned long n);
void *std_memrchr(const void *p, int c, unsigned long n);
void *std_memrchrbegin(const void *p, int c, unsigned long n);
void *std_memchrend(const void *p, int c, unsigned long n);
void *std_memchrsend(const void *p, const char *set, unsigned long n);
void *std_memset(void *p, int c, unsigned long n);
unsigned long std_wstrlen(const void *s);
char *std_strchr(const char *s, int c);
char *std_strrchr(const char *s, int c);
char *std_strchrend(const char *s, int c);
char *std_strchrsend(const char *s, const char *set);
char *std_strchrs(const char *s, const char *set);
char *std_strstr(const char *haystack, const char *needle);
void *std_memstr(const void *haystack, const char *needle, unsigned long n);
char *std_striends(const char *s, const char *suffix);
char *std_strends(const char *s, const char *suffix);
char *std_strbegins(const char *s, const char *prefix);
char *std_stribegins(const char *s, const char *prefix);
unsigned long std_strcspn(const char *s, const char *reject);
unsigned long std_strspn(const char *s, const char *accept);
unsigned long long std_scanux(const char *s, unsigned long long base,
                             const char **pEnd, int *pStatus);
int std_scanul(const char *s, unsigned long long base, const char **pEnd,
               int *pStatus);
long long std_scanull(const char *s, unsigned long long base,
                      const char **pEnd, int *pStatus);

int std_getversion(char *buf, unsigned long size)
{
    unsigned long n;

    if ((long)size < 1)
        return 3;

    n = (size < 4 ? size : 4) - 1;
    if (size != 1)
        memcpy(buf, "1.1", n);
    buf[n] = 0;
    return 3;
}

unsigned long std_strlcpy(char *dst, const char *src, unsigned long size)
{
    unsigned long len = strlen(src);
    unsigned long n;

    if ((long)(size - 1) >= 0) {
        n = len < size - 1 ? len : size - 1;
        if ((long)n >= 1)
            memmove(dst, src, n);
        dst[n] = 0;
    }
    return len;
}

unsigned long std_strlcat(char *dst, const char *src, unsigned long size)
{
    unsigned long used = 0;
    unsigned long space;
    unsigned long len;

    if ((long)size >= 1) {
        while (dst[used] != 0) {
            used++;
            if (used == size)
                break;
        }
    }

    space = size - used;
    len = strlen(src);

    if ((long)space >= 1) {
        char *out = dst + used;
        unsigned long n = space - 1;

        n = len < n ? len : n;
        if ((long)n >= 1)
            memmove(out, src, n);
        out[n] = 0;
    }
    return used + len;
}

int std_tolower(int c)
{
    int ch = c & 0xff;

    if ((unsigned)(ch - 0x41) < 0x1a)
        return ch | 0x20;
    return ch;
}

int std_toupper(int c)
{
    int ch = c & 0xff;

    if ((unsigned)(ch - 0x61) < 0x1a)
        return c & 0x5f;
    return ch;
}

unsigned long std_strlen(const char *s)
{
    return strlen(s);
}

long std_strncmp(const char *a, const char *b, unsigned long n)
{
    if ((long)n < 1)
        return 0;

    for (;;) {
        unsigned char ca = (unsigned char)*a++;
        unsigned char cb = (unsigned char)*b++;

        if (ca != cb)
            return (long)ca - (long)cb;
        if (ca == 0)
            return 0;
        if (--n == 0)
            return 0;
    }
}

long std_strcmp(const char *a, const char *b)
{
    unsigned long n = 0x7fffffff;

    for (;;) {
        unsigned char ca = (unsigned char)*a++;
        unsigned char cb = (unsigned char)*b++;

        if (ca != cb)
            return (long)ca - (long)cb;
        if (ca == 0)
            return 0;
        if (--n == 0)
            return 0;
    }
}


static long mm_ncase_cmp(unsigned char ca, unsigned char cb)
{
    long diff = (long)ca - (long)cb;

    if ((unsigned)(ca - 0x41) < 0x1a)
        diff += 0x20;
    if ((unsigned)(cb - 0x41) < 0x1a)
        diff -= 0x20;
    return diff;
}

long std_strnicmp(const char *a, const char *b, unsigned long n)
{
    if ((long)n < 1)
        return 0;

    for (;;) {
        unsigned char ca = (unsigned char)*a++;
        unsigned char cb = (unsigned char)*b++;
        long d = mm_ncase_cmp(ca, cb);

        if (d != 0)
            return d;
        if (ca == 0)
            return 0;
        if (--n == 0)
            return 0;
    }
}

long std_stricmp(const char *a, const char *b)
{
    unsigned long n = 0x7fffffff;

    for (;;) {
        unsigned char ca = (unsigned char)*a++;
        unsigned char cb = (unsigned char)*b++;
        long d = mm_ncase_cmp(ca, cb);

        if (d != 0)
            return d;
        if (ca == 0)
            return 0;
        if (--n == 0)
            return 0;
    }
}

void *std_memmove(void *dst, const void *src, unsigned long n)
{
    if ((long)n < 1)
        return dst;
    return memmove(dst, src, n);
}

void *std_memset(void *p, int c, unsigned long n)
{
    if ((long)n < 0)
        return p;
    return memset(p, c, n);
}

long std_memcmp(const void *a, const void *b, unsigned long n)
{
    const unsigned char *pa = (const unsigned char *)a;
    const unsigned char *pb = (const unsigned char *)b;

    while (n != 0) {
        long d = (long)*pa++ - (long)*pb++;

        if (d != 0)
            return d;
        n--;
    }
    return 0;
}

unsigned long std_wstrlen(const void *s)
{
    const uint16_t *w = (const uint16_t *)s;
    unsigned long n = 0;

    while (w[n] != 0)
        n++;
    return n;
}

char *std_strchrend(const char *s, int c)
{
    unsigned char m = (unsigned char)c;

    for (;;) {
        unsigned char b = (unsigned char)*s;

        if (b == 0 || b == m)
            return (char *)s;
        s++;
    }
}

char *std_strchr(const char *s, int c)
{
    unsigned char m = (unsigned char)c;

    for (;;) {
        unsigned char b = (unsigned char)*s;

        if (b == 0 || b == m)
            return (int)b == c ? (char *)s : NULL;
        s++;
    }
}

char *std_strrchr(const char *s, int c)
{
    size_t n = strlen(s);

    for (;;) {
        if ((int)(unsigned char)s[n] == c)
            return (char *)(s + n);
        if (n == 0)
            return NULL;
        n--;
    }
}

char *std_strstr(const char *haystack, const char *needle)
{
    unsigned char first = (unsigned char)needle[0];
    const char *p = haystack;
    unsigned long k;

    if (first == 0)
        return (char *)haystack;

    for (;;) {
        if ((unsigned char)*p != first) {
            if ((unsigned char)*p == 0)
                return NULL;
            p++;
            continue;
        }
        for (k = 1;; k++) {
            unsigned char nc = (unsigned char)needle[k];

            if (nc == 0)
                return (char *)p;
            if ((unsigned char)p[k] != nc)
                break;
        }
        p += k;
        if ((unsigned char)*p == 0)
            return NULL;
    }
}

void *std_memstr(const void *haystack, const char *needle, unsigned long n)
{
    const unsigned char *h = (const unsigned char *)haystack;
    unsigned char first = (unsigned char)needle[0];
    unsigned long i;

    if (first == 0)
        return (void *)haystack;
    if ((long)n < 1)
        return NULL;

    i = 0;
    for (;;) {
        if (h[i] != first) {
            i++;
            if ((long)i >= (long)n)
                return NULL;
            continue;
        }
        {
            const char *n2 = needle + 1;
            unsigned long j = i;

            for (;;) {
                unsigned char nc = (unsigned char)*n2++;

                if (nc == 0)
                    return (void *)(h + i);
                j++;
                if (h[j] != nc)
                    break;
            }
            i = j;
            if ((long)i >= (long)n)
                return NULL;
        }
    }
}

void *std_memchr(const void *p, int c, unsigned long n)
{
    const unsigned char *b = (const unsigned char *)p;
    unsigned long i;

    if ((long)n < 1)
        return NULL;
    for (i = 0; i < n; i++)
        if ((int)b[i] == c)
            return (void *)(b + i);
    return NULL;
}

void *std_memrchr(const void *p, int c, unsigned long n)
{
    const unsigned char *b = (const unsigned char *)p;

    while ((long)n >= 1) {
        if ((int)b[n - 1] == c)
            return (void *)(b + n - 1);
        n--;
    }
    return NULL;
}

void *std_memrchrbegin(const void *p, int c, unsigned long n)
{
    const unsigned char *b = (const unsigned char *)p;

    while ((long)n >= 1) {
        if ((int)b[n - 1] == c)
            return (void *)(b + n - 1);
        n--;
    }
    return (void *)p;
}

void *std_memchrend(const void *p, int c, unsigned long n)
{
    const unsigned char *b = (const unsigned char *)p;
    unsigned long i;

    if ((long)n < 1)
        return (void *)b;
    for (i = 0; i < n; i++)
        if ((int)b[i] == c)
            return (void *)(b + i);
    return (void *)(b + n);
}


typedef struct {
    uint32_t w[8];
} mm_charset;

static void mm_charset_add(mm_charset *set, unsigned char c)
{
    set->w[c & 7] |= (uint32_t)0x80000000u >> (c >> 3);
}

static int mm_charset_test(const mm_charset *set, unsigned char c)
{
    return (int)((set->w[c & 7] >> (31 - (c >> 3))) & 1u);
}

static void mm_charset_build(mm_charset *set, const char *chars)
{

    unsigned char c;

    memset(set, 0, sizeof(*set));
    c = (unsigned char)*chars++;
    while (c != 0) {
        mm_charset_add(set, c);
        c = (unsigned char)*chars++;
    }
}


static void mm_charset_build_nul(mm_charset *set, const char *chars)
{
    unsigned char c = 0;

    memset(set, 0, sizeof(*set));
    do {
        mm_charset_add(set, c);
        c = (unsigned char)*chars++;
    } while (c != 0);
}

void *std_memchrsend(const void *p, const char *set, unsigned long n)
{
    const unsigned char *b = (const unsigned char *)p;
    mm_charset cs;
    unsigned long i;

    if ((long)n < 1)
        return (void *)p;
    if (set[0] == 0)
        return (void *)(b + n);

    mm_charset_build(&cs, set);
    for (i = 0; i < n; i++)
        if (mm_charset_test(&cs, b[i]))
            return (void *)(b + i);
    return (void *)(b + n);
}

char *std_strchrsend(const char *s, const char *set)
{
    mm_charset cs;

    mm_charset_build_nul(&cs, set);
    for (;;) {
        if (mm_charset_test(&cs, (unsigned char)*s))
            return (char *)s;
        s++;
    }
}

char *std_strchrs(const char *s, const char *set)
{
    mm_charset cs;

    mm_charset_build_nul(&cs, set);
    for (;;) {
        unsigned char c = (unsigned char)*s;

        if (mm_charset_test(&cs, c))
            return c == 0 ? NULL : (char *)s;
        s++;
    }
}

unsigned long std_strcspn(const char *s, const char *reject)
{
    mm_charset cs;
    unsigned long i = 0;

    mm_charset_build_nul(&cs, reject);
    while (!mm_charset_test(&cs, (unsigned char)s[i]))
        i++;
    return i;
}

unsigned long std_strspn(const char *s, const char *accept)
{
    mm_charset cs;
    unsigned long i = 0;
    unsigned char c;

    memset(&cs, 0xff, sizeof(cs));
    c = (unsigned char)*accept;
    while (c != 0) {
        cs.w[c & 7] ^= (uint32_t)0x80000000u >> (c >> 3);
        c = (unsigned char)*++accept;
    }
    while (!mm_charset_test(&cs, (unsigned char)s[i]))
        i++;
    return i;
}

char *std_strends(const char *s, const char *suffix)
{
    size_t ls = strlen(s);
    size_t lx = strlen(suffix);
    size_t off;

    if (ls < lx)
        return NULL;
    off = ls - lx;
    if ((long)off < 0)
        return NULL;
    {
        const char *x = s + off;
        const char *y = suffix;
        unsigned long n = 0x7fffffff;

        for (;;) {
            unsigned char ca = (unsigned char)*x++;
            unsigned char cb = (unsigned char)*y++;

            if (ca != cb)
                return NULL;
            if (ca == 0)
                break;
            if (--n == 0)
                break;
        }
    }
    return (char *)(s + off);
}

char *std_striends(const char *s, const char *suffix)
{
    size_t ls = strlen(s);
    size_t lx = strlen(suffix);
    size_t off;

    if (ls < lx)
        return NULL;
    off = ls - lx;
    if ((long)off < 0)
        return NULL;
    {
        const char *x = s + off;
        const char *y = suffix;
        unsigned long n = 0x7fffffff;

        for (;;) {
            unsigned char ca = (unsigned char)*x++;
            unsigned char cb = (unsigned char)*y++;

            if (mm_ncase_cmp(ca, cb) != 0)
                return NULL;
            if (ca == 0)
                break;
            if (--n == 0)
                break;
        }
    }
    return (char *)(s + off);
}

char *std_strbegins(const char *s, const char *prefix)
{
    for (;;) {
        unsigned char c = (unsigned char)*prefix;

        if (c == 0)
            return (char *)s;
        if (c != (unsigned char)*s)
            return NULL;
        prefix++;
        s++;
    }
}

char *std_stribegins(const char *s, const char *prefix)
{
    for (;;) {
        unsigned char c = (unsigned char)*prefix;

        if (c == 0)
            return (char *)s;
        if (mm_ncase_cmp(c, (unsigned char)*s) != 0)
            return NULL;
        prefix++;
        s++;
    }
}


#define MM_SCAN_MAX UINT64_MAX
#define MM_SCAN_THR8 0x1fffffffffffffffULL
#define MM_SCAN_THR10 0x1999999999999999ULL
#define MM_SCAN_THR16 0x0fffffffffffffffULL

unsigned long long std_scanux(const char *s, unsigned long long base,
                              const char **pEnd, int *pStatus)
{
    const char *p = s;
    const char *digits;
    const char *next;
    unsigned long long acc = 0;
    unsigned long long thr = 0;
    unsigned int maxDigit = 0;
    int sign = 0;
    int err = 0;
    unsigned char c;

    if (base > 0x24 || base == 1) {
        if (pStatus)
            *pStatus = 4;
        if (pEnd)
            *pEnd = s;
        return 0;
    }


    for (;;) {
        c = (unsigned char)*p;
        if (c <= 0x1f) {
            if (c == '\t') {
                p++;
                continue;
            }
            break;
        }
        if (c == ' ') {
            p++;
            continue;
        }
        break;
    }

    if (c == '+') {
        p++;
    } else if (c == '-') {
        p++;
        sign = 2;
    }

    if (base == 16 || base == 0) {
        if (p[0] == '0' && ((unsigned char)p[1] | 0x20) == 'x') {
            unsigned char d = (unsigned char)p[2];
            unsigned int v = (unsigned int)d - 0x30u;
            int valid;

            if (v < 10u) {
                valid = 1;
            } else {
                unsigned int t = (unsigned int)d | 0x20u;

                if (t < 0x61u)
                    MM_UBSAN_SUB(0x61u - t);
                valid = (t - 0x61u) < 6u;
            }
            if (valid) {
                p += 2;
                base = 16;
                goto have_base;
            }
        }
        base = (base == 0) ? ((p[0] == '0') ? 8 : 10) : 16;
    }

have_base:
    if (base == 8) {
        thr = MM_SCAN_THR8;
        maxDigit = 7;
    } else if (base == 10) {
        thr = MM_SCAN_THR10;
        maxDigit = 5;
    } else if (base == 16) {
        thr = MM_SCAN_THR16;
        maxDigit = 0xf;
    } else {
        thr = (MM_SCAN_MAX / base) * base;
        maxDigit = ~(unsigned int)thr;
    }

    digits = p;
    next = p + 1;
    c = (unsigned char)*p;


    if (c < '0') {
        MM_UBSAN_SUB(0x30u - c);
        goto stop_at_previous;
    }

    for (;;) {
        unsigned long long d;

        if ((unsigned int)(c - '0') <= 9u) {
            d = (unsigned long long)(c - '0');
        } else {
            unsigned int t = (unsigned int)c | 0x20u;

            if (t < 0x61u)
                MM_UBSAN_SUB(0x61u - t);
            if (t - 0x61u > 25u)
                goto done;
            d = (unsigned long long)(t - 0x57u);
        }

        if (base <= d) {
            next = digits + 1;
            digits = next - 1;
            goto done;
        }

        digits++;

        if (acc > thr) {
            acc = MM_SCAN_MAX;
            err = 3;
            goto done;
        }
        if (acc == thr && d > (unsigned long long)maxDigit) {
            acc = MM_SCAN_MAX;
            err = 3;
            digits = next;
            goto done;
        }

        acc = acc * base + d;

        c = (unsigned char)*next++;
        if (c >= '0')
            continue;

        MM_UBSAN_SUB(0x30u - c);
        goto stop_at_previous;
    }

stop_at_previous:
    digits = next - 1;

done:
    if (pStatus)
        *pStatus = (digits != p) ? (err != 0 ? err : sign) : 1;
    if (pEnd)
        *pEnd = (digits != p) ? digits : s;
    return acc;
}

int std_scanul(const char *s, unsigned long long base, const char **pEnd,
               int *pStatus)
{
    int st = 0;
    unsigned long long v;
    unsigned int w;

    v = std_scanux(s, base, pEnd, &st);
    if ((v >> 32) != 0) {
        if (pStatus)
            *pStatus = 3;
        return -1;
    }

    w = (unsigned int)v;
    if (st == 2) {
        if (w == 0x80000000u)
            MM_UBSAN_NEG(w);
        w = (unsigned int)(-(int)w);
        st = (w != 0) ? 2 : 0;
    }
    if (pStatus)
        *pStatus = st;
    return (int)w;
}

long long std_scanull(const char *s, unsigned long long base, const char **pEnd,
                      int *pStatus)
{
    int st = 0;
    unsigned long long v;

    v = std_scanux(s, base, pEnd, &st);
    if (st == 2) {
        if (v == 0x8000000000000000ULL)
            MM_UBSAN_NEG(v);
        v = (unsigned long long)(-(long long)v);
        st = (v != 0) ? 2 : 0;
    }
    if (pStatus)
        *pStatus = st;
    return (long long)v;
}





void __cfi_check(uint64_t typeId, void *addr, void *diag);

void __cfi_check(uint64_t typeId, void *addr, void *diag)
{
    (void)typeId;
    (void)addr;
    (void)diag;
}
