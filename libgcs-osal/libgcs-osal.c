/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <android/log.h>


extern int __open_2(const char *path, int flags);
extern ssize_t __read_chk(int fd, void *buf, size_t count, size_t buf_size);
extern ssize_t __write_chk(int fd, const void *buf, size_t count, size_t buf_size);


#define OSAL_LOG(fmt, ...) \
    __android_log_print(ANDROID_LOG_ERROR, NULL, fmt, __func__, ##__VA_ARGS__)


#define OSAL_EXPORT __attribute__((visibility("default")))


extern void __aeabi_memcpy(void *dst, const void *src, size_t n);
extern void __aeabi_memset(void *dst, size_t n, int c);
extern void __aeabi_memclr(void *dst, size_t n);






typedef pthread_mutex_t osal_lock_t;

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} osal_cond_t;

typedef struct {
    int fd;
} osal_dev_t;

typedef struct {
    pthread_t tid;
    void *(*fn)(void *);
    void *arg;
} osal_thread_t;





OSAL_EXPORT void *osal_mem_alloc(size_t size)
{

    return malloc(size);
}

OSAL_EXPORT void *osal_mem_zalloc(size_t size)
{

    void *ptr = malloc(size);

    if (ptr != NULL) {
        __aeabi_memclr(ptr, size);
    }

    return ptr;
}

OSAL_EXPORT void *osal_mem_calloc(size_t nmemb, size_t size)
{

    return calloc(nmemb, size);
}

OSAL_EXPORT void *osal_mem_realloc(void *ptr, size_t size)
{

    return realloc(ptr, size);
}

OSAL_EXPORT void osal_mem_free(void *ptr)
{

    free(ptr);
}


OSAL_EXPORT void *osal_mem_cpy(void *dst, uint32_t dst_size, const void *src, uint32_t len)
{
    (void)dst_size;

    __aeabi_memcpy(dst, src, len);
    return dst;
}


OSAL_EXPORT void *osal_mem_set(void *dst, int value, uint32_t len)
{

    __aeabi_memset(dst, len, (uint8_t)value);
    return dst;
}





OSAL_EXPORT int osal_lock_create(osal_lock_t **ret_lock)
{

    osal_lock_t *lock;
    int rc;

    if (ret_lock == NULL) {
        OSAL_LOG("%s: ret_lock is NULL\n");
        return -EINVAL;
    }

    lock = malloc(sizeof(*lock));
    if (lock == NULL) {
        OSAL_LOG("%s: failed to allocate memory for lock\n");
        return -ENOMEM;
    }

    rc = pthread_mutex_init(lock, NULL);
    if (rc != 0) {

        OSAL_LOG("%s: Failed to create lock\n");
        free(lock);
        return -rc;
    }

    *ret_lock = lock;
    return 0;
}

OSAL_EXPORT int osal_lock_destroy(osal_lock_t *lock)
{

    int rc;

    if (lock == NULL) {
        OSAL_LOG("%s: lock is NULL\n");
        return -EINVAL;
    }

    rc = pthread_mutex_destroy(lock);
    if (rc != 0) {
        OSAL_LOG("%s: Failed to destroy lock\n");

        return -rc;
    }

    free(lock);
    return 0;
}

OSAL_EXPORT int osal_lock_acquire(osal_lock_t *lock)
{

    int rc;

    if (lock == NULL) {
        OSAL_LOG("%s: lock is NULL\n");
        return -EINVAL;
    }

    rc = pthread_mutex_lock(lock);
    if (rc != 0) {
        OSAL_LOG("%s: Failed to acquire lock\n");
        return -rc;
    }

    return 0;
}

OSAL_EXPORT int osal_lock_release(osal_lock_t *lock)
{

    int rc;

    if (lock == NULL) {
        OSAL_LOG("%s: lock is NULL\n");
        return -EINVAL;
    }

    rc = pthread_mutex_unlock(lock);
    if (rc != 0) {
        OSAL_LOG("%s: Failed to release lock\n");
        return -rc;
    }

    return 0;
}





OSAL_EXPORT int osal_cond_create(osal_cond_t **ret_cond)
{

    osal_cond_t *cond;
    pthread_condattr_t attr;
    int rc;

    if (ret_cond == NULL) {
        OSAL_LOG("%s: cond is NULL\n");
        return -EINVAL;
    }

    cond = malloc(sizeof(*cond));
    if (cond == NULL) {
        OSAL_LOG("%s: failed to allocate cond memory\n");
        return -ENOMEM;
    }

    rc = pthread_mutex_init(&cond->mutex, NULL);
    if (rc != 0) {
        OSAL_LOG("%s: Failed to init mutex, rc = %d\n", rc);
        free(cond);
        return -rc;
    }


    memset(&attr, 0, sizeof(attr));
    pthread_condattr_init(&attr);
    pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);

    rc = pthread_cond_init(&cond->cond, &attr);
    if (rc != 0) {
        OSAL_LOG("%s: Failed to init cond, rc = %d\n", rc);
        goto out_fail;
    }


    *ret_cond = cond;

    rc = pthread_mutex_lock(&cond->mutex);
    if (rc != 0) {
        OSAL_LOG("%s: Failed to acquire lock, rc = %d\n", rc);
        goto out_fail;
    }

    return 0;

out_fail:
    pthread_mutex_destroy(&cond->mutex);
    free(cond);
    return -rc;
}

OSAL_EXPORT int osal_cond_destroy(osal_cond_t *cond)
{

    int rc;

    if (cond == NULL) {
        OSAL_LOG("%s: cond is NULL\n");
        return -EINVAL;
    }

    rc = pthread_cond_destroy(&cond->cond);
    if (rc != 0) {

        OSAL_LOG("%s: Failed to destroy cond, rc = %d\n", rc);
    }

    rc = pthread_mutex_destroy(&cond->mutex);
    if (rc != 0) {
        OSAL_LOG("%s: Failed to destroy mutex, rc = %d\n", rc);
        rc = -rc;
    } else {
        rc = 0;
    }

    free(cond);
    return rc;
}

OSAL_EXPORT int osal_cond_wait(osal_cond_t *cond)
{

    int rc;

    if (cond == NULL) {

        OSAL_LOG("%s: condition is NULL\n");
        return -EINVAL;
    }

    rc = pthread_cond_wait(&cond->cond, &cond->mutex);
    if (rc != 0) {

        OSAL_LOG("%s: Failed to wait on cond, rc = %d\n", rc);
    }

    rc = pthread_mutex_unlock(&cond->mutex);
    if (rc != 0) {
        OSAL_LOG("%s: Failed to unlock, rc = %d\n", rc);
        return -rc;
    }

    return 0;
}

OSAL_EXPORT int osal_cond_timedwait(osal_cond_t *cond, uint32_t timeout_ms)
{

    struct timespec ts = { 0, 0 };
    int wait_rc = 0;
    int rc;

    if (cond == NULL) {
        OSAL_LOG("%s: cond is NULL\n");
        return -EINVAL;
    }

    clock_gettime(CLOCK_MONOTONIC, &ts);
    ts.tv_sec += (time_t)(timeout_ms / 1000u);

    rc = pthread_cond_timedwait(&cond->cond, &cond->mutex, &ts);
    if (rc != 0) {
        OSAL_LOG("%s: Failed to wait on cond, rc = %d\n", rc);
        wait_rc = -rc;
    }

    rc = pthread_mutex_unlock(&cond->mutex);
    if (rc != 0) {

        OSAL_LOG("%s: Failed to unlock, rc = %d\n", wait_rc);
        return -rc;
    }

    return wait_rc;
}

OSAL_EXPORT int osal_cond_signal(osal_cond_t *cond)
{

    int rc;

    if (cond == NULL) {
        OSAL_LOG("%s: cond is NULL\n");
        return -EINVAL;
    }

    rc = pthread_mutex_lock(&cond->mutex);
    if (rc != 0) {
        OSAL_LOG("%s: Failed to acquire lock, rc = %d\n", rc);
        return -rc;
    }

    rc = pthread_cond_signal(&cond->cond);
    if (rc != 0) {

        OSAL_LOG("%s: Failed to signal on cond, rc = %d\n", rc);
    }

    rc = pthread_mutex_unlock(&cond->mutex);
    if (rc != 0) {
        OSAL_LOG("%s: Failed to unlock, rc = %d\n", rc);
        return -rc;
    }

    return 0;
}






static void *osal_thread_trampoline(void *arg)
{

    osal_thread_t *thread = arg;

    thread->fn(thread->arg);
    return NULL;
}


OSAL_EXPORT int osal_thread_create(osal_thread_t **ret_thread, uint32_t arg2, uint32_t arg3,
                                   uint32_t arg4, uint32_t arg5, void *(*fn)(void *),
                                   void *arg)
{
    pthread_attr_t attr;
    osal_thread_t *thread;
    void *join_ret = NULL;
    int rc;

    (void)arg2;
    (void)arg3;
    (void)arg4;
    (void)arg5;

    if (ret_thread == NULL) {
        OSAL_LOG("%s: ret_thread is NULL\n");
        return -EINVAL;
    }

    thread = malloc(sizeof(*thread));
    if (thread == NULL) {
        OSAL_LOG("%s: Failed to allocate memory\n");
        return -ENOMEM;
    }


    memset(&attr, 0, sizeof(attr));

    rc = pthread_attr_init(&attr);
    if (rc != 0) {
        OSAL_LOG("%s: Failed to init attributes, rc = %d\n", rc);
        free(thread);
        return -rc;
    }

    rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    if (rc != 0) {
        OSAL_LOG("%s: Failed to set detach state, rc = %d\n", rc);
        goto out_free;
    }

    thread->fn = fn;
    thread->arg = arg;

    rc = pthread_create(&thread->tid, &attr, osal_thread_trampoline, thread);
    if (rc != 0) {
        OSAL_LOG("%s: Failed to create thread, rc = %d\n", rc);
        goto out_free;
    }

    rc = pthread_attr_destroy(&attr);
    if (rc != 0) {

        OSAL_LOG("%s: Failed to destroy attributes, rc = %d\n", rc);
        pthread_join(thread->tid, &join_ret);
        pthread_attr_destroy(&attr);
        free(thread);
        return -rc;
    }

    *ret_thread = thread;
    return 0;

out_free:
    pthread_attr_destroy(&attr);
    free(thread);
    return -rc;
}

OSAL_EXPORT int osal_thread_destroy(osal_thread_t *thread)
{

    void *join_ret = NULL;
    int rc;

    if (thread == NULL) {
        OSAL_LOG("%s: Thread is NULL\n");
        return -EINVAL;
    }

    rc = pthread_join(thread->tid, &join_ret);
    if (rc != 0) {
        OSAL_LOG("%s: Failed to join the thread, rc = %d\n", rc);
        rc = -rc;
    } else {
        rc = 0;
    }

    free(thread);
    return rc;
}

OSAL_EXPORT int osal_thread_close_from_thread_fn(void)
{

    return 0;
}





OSAL_EXPORT int osal_dev_open(osal_dev_t **ret_dev, const char *dev_name, int flags)
{

    osal_dev_t *dev;

    if (ret_dev == NULL || dev_name == NULL) {
        OSAL_LOG("%s: dev or dev_name is NULL\n");
        return -EINVAL;
    }

    dev = malloc(sizeof(*dev));
    if (dev == NULL) {
        OSAL_LOG("%s: failed to allocate dev memory\n");
        return -ENOMEM;
    }


    dev->fd = __open_2(dev_name, flags);
    if (dev->fd == -1) {

        OSAL_LOG("%s: open failed: %d", errno);
        free(dev);
        return -errno;
    }

    *ret_dev = dev;
    return 0;
}

OSAL_EXPORT int osal_dev_close(osal_dev_t *dev)
{

    int rc;

    if (dev == NULL) {
        OSAL_LOG("%s: dev is NULL\n");
        return -EINVAL;
    }

    rc = close(dev->fd);
    if (rc == -1) {
        OSAL_LOG("%s: close failed: %d", errno);

        rc = -errno;
    }

    free(dev);
    return rc;
}

OSAL_EXPORT int osal_dev_read(osal_dev_t *dev, void *buf, size_t count)
{

    ssize_t rc;

    if (dev == NULL) {
        OSAL_LOG("%s: dev is NULL\n");
        return -EINVAL;
    }

    rc = __read_chk(dev->fd, buf, count, (size_t)-1);
    if (rc == -1) {
        OSAL_LOG("%s: read failed: %d", errno);
        return -errno;
    }

    return (int)rc;
}

OSAL_EXPORT int osal_dev_write(osal_dev_t *dev, const void *buf, size_t count)
{

    ssize_t rc;

    if (dev == NULL) {
        OSAL_LOG("%s: dev is NULL\n");
        return -EINVAL;
    }

    rc = __write_chk(dev->fd, buf, count, (size_t)-1);
    if (rc == -1) {
        OSAL_LOG("%s: write failed: %d", errno);
        return -errno;
    }

    return (int)rc;
}






__asm__(
    ".text\n"
    ".thumb\n"
    ".balign 2\n"
    ".global __aeabi_memcpy\n"
    ".type __aeabi_memcpy, %function\n"
    "__aeabi_memcpy:\n"
    ".thumb_func\n"
    "  b memcpy\n"
    ".size __aeabi_memcpy, . - __aeabi_memcpy\n"
    ".global __aeabi_memset\n"
    ".type __aeabi_memset, %function\n"
    "__aeabi_memset:\n"
    ".thumb_func\n"
    "  mov r3, r1\n"
    "  mov r1, r2\n"
    "  mov r2, r3\n"
    "  b memset\n"
    ".size __aeabi_memset, . - __aeabi_memset\n"
    ".global __aeabi_memclr\n"
    ".type __aeabi_memclr, %function\n"
    "__aeabi_memclr:\n"
    ".thumb_func\n"
    "  mov r2, r1\n"
    "  movs r1, #0\n"
    "  b memset\n"
    ".size __aeabi_memclr, . - __aeabi_memclr\n"

    ".global __aeabi_memcpy4\n"
    ".global __aeabi_memcpy8\n"
    ".set __aeabi_memcpy4, __aeabi_memcpy\n"
    ".set __aeabi_memcpy8, __aeabi_memcpy\n"
    ".global __aeabi_memset4\n"
    ".global __aeabi_memset8\n"
    ".set __aeabi_memset4, __aeabi_memset\n"
    ".set __aeabi_memset8, __aeabi_memset\n"
    ".global __aeabi_memclr4\n"
    ".global __aeabi_memclr8\n"
    ".set __aeabi_memclr4, __aeabi_memclr\n"
    ".set __aeabi_memclr8, __aeabi_memclr\n");
