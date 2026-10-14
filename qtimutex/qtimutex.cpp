/*
 * Copyright (C) 2025 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#include <__config>


#undef _LIBCPP_HIDE_FROM_ABI_VIRTUAL
#define _LIBCPP_HIDE_FROM_ABI_VIRTUAL __attribute__((visibility("default")))
__asm__(".hidden _ZNSt3__110__function6__baseIFvvEED2Ev");

#include <chrono>
#include <condition_variable>
#include <cstring>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <string>

#include <pthread.h>
#include <cutils/properties.h>


#define QTIMUTEX_API __attribute__((visibility("default")))

namespace qtimutex {

namespace {


constexpr int64_t kDefaultDeadlockTimeoutMs = 300000;


constexpr char kTimeoutMessage[] = "FAILURE - QtiSharedMutex timeout.";

}

class QTIMUTEX_API QtiMutexDefaults {
public:
    static QtiMutexDefaults& getInstance();

    void setDebugFlag(bool flag);
    bool getDebugFlag();

    void setDefaultDeadlockTimeout(std::chrono::milliseconds timeout);
    std::chrono::milliseconds getDeadlockTimeout();

    void setDefaultDeadlockAction(std::function<void()> action);
    std::function<void()> getDeadlockAction();

    void setDefaultDeadlockLogger(std::function<void(std::string)> logger);
    std::function<void(std::string)> getDeadlockLogger();

    void setDefaultDebugLogger(std::function<void(std::string)> logger);
    std::function<void(std::string)> getDebugLogger();

    void setPropertyName(std::string name);
    std::string getPropertyName();

private:
    QtiMutexDefaults();


    std::chrono::milliseconds deadlockTimeout_;

    uint64_t reserved08_ __attribute__((unused));

    std::function<void()> deadlockAction_;

    std::function<void(std::string)> deadlockLogger_;

    std::function<void(std::string)> debugLogger_;

    std::string propertyName_;

    bool debugFlag_;
};

class QTIMUTEX_API QtiRecursiveMutex {
public:
    void lock();
    bool try_lock();
    void unlock();

private:

    std::recursive_timed_mutex mutex_;
};

class QTIMUTEX_API QtiSharedMutex {
public:
    void lock();
    bool try_lock();
    void unlock();

    void lock_shared();
    bool try_lock_shared();
    void unlock_shared();

private:

    std::shared_timed_mutex mutex_;
};

class QTIMUTEX_API QtiShmMutex {
public:
    QtiShmMutex();

    void lock();
    bool try_lock();
    void unlock();

private:
    pthread_mutex_t mutex_;
};

class QTIMUTEX_API QtiShmConditionVariable {
public:
    QtiShmConditionVariable();
    ~QtiShmConditionVariable();

    void notify_one();
    void notify_all();

private:
    pthread_cond_t cond_;
};

namespace {


bool wrapperBypassed() {
    std::string name = QtiMutexDefaults::getInstance().getPropertyName();
    if (name.empty()) {
        return false;
    }
    char value[PROPERTY_VALUE_MAX];
    property_get(name.c_str(), value, "");
    return strcmp(value, "false") == 0;
}


void logDebug(const std::function<void(std::string)>& logger, const char* message) {
    if (QtiMutexDefaults::getInstance().getDebugFlag()) {
        logger(message);
    }
}

}

QtiMutexDefaults::QtiMutexDefaults() {

    deadlockAction_ = abort;
    deadlockTimeout_ = std::chrono::milliseconds(kDefaultDeadlockTimeoutMs);
}

QtiMutexDefaults& QtiMutexDefaults::getInstance() {
    static QtiMutexDefaults instance;
    return instance;
}

void QtiMutexDefaults::setDebugFlag(bool flag) {
    debugFlag_ = flag;
}

bool QtiMutexDefaults::getDebugFlag() {
    return debugFlag_;
}

void QtiMutexDefaults::setDefaultDeadlockTimeout(std::chrono::milliseconds timeout) {
    deadlockTimeout_ = timeout;
}

std::chrono::milliseconds QtiMutexDefaults::getDeadlockTimeout() {
    return deadlockTimeout_;
}

void QtiMutexDefaults::setDefaultDeadlockAction(std::function<void()> action) {
    deadlockAction_ = action;
}

std::function<void()> QtiMutexDefaults::getDeadlockAction() {
    return deadlockAction_;
}

void QtiMutexDefaults::setDefaultDeadlockLogger(std::function<void(std::string)> logger) {
    deadlockLogger_ = logger;
}

std::function<void(std::string)> QtiMutexDefaults::getDeadlockLogger() {
    return deadlockLogger_;
}

void QtiMutexDefaults::setDefaultDebugLogger(std::function<void(std::string)> logger) {
    debugLogger_ = logger;
}

std::function<void(std::string)> QtiMutexDefaults::getDebugLogger() {
    return debugLogger_;
}

void QtiMutexDefaults::setPropertyName(std::string name) {
    propertyName_ = name;
}

std::string QtiMutexDefaults::getPropertyName() {
    return propertyName_;
}


void QtiRecursiveMutex::lock() {
    if (wrapperBypassed()) {
        mutex_.lock();
        return;
    }

    const std::function<void(std::string)> debugLogger =
            QtiMutexDefaults::getInstance().getDebugLogger();
    const std::function<void(std::string)> deadlockLogger =
            QtiMutexDefaults::getInstance().getDeadlockLogger();
    const std::function<void()> deadlockAction =
            QtiMutexDefaults::getInstance().getDeadlockAction();

    logDebug(debugLogger, "Entering QtiRecursiveMutex::lock()");

    const std::chrono::steady_clock::time_point deadline =
            std::chrono::steady_clock::now() +
            std::chrono::milliseconds(QtiMutexDefaults::getInstance().getDeadlockTimeout());

    bool acquired = false;
    while (!acquired && std::chrono::steady_clock::now() < deadline) {
        acquired = mutex_.try_lock_until(deadline);
    }
    if (!acquired) {
        deadlockLogger(kTimeoutMessage);
        deadlockAction();
    }

    logDebug(debugLogger, "Exiting QtiRecursiveMutex::lock()");
}


bool QtiRecursiveMutex::try_lock() {
    if (wrapperBypassed()) {
        return mutex_.try_lock();
    }

    const std::function<void(std::string)> debugLogger =
            QtiMutexDefaults::getInstance().getDebugLogger();

    logDebug(debugLogger, "Entering QtiRecursiveMutex::try_lock()");
    const bool result = mutex_.try_lock();
    logDebug(debugLogger,
             result ? "Exiting QtiRecursiveMutex::try_lock() with return true"
                    : "Exiting QtiRecursiveMutex::try_lock() with return false");
    return result;
}


void QtiRecursiveMutex::unlock() {
    if (wrapperBypassed()) {
        mutex_.unlock();
        return;
    }

    const std::function<void(std::string)> debugLogger =
            QtiMutexDefaults::getInstance().getDebugLogger();

    logDebug(debugLogger, "Entering QtiRecursiveMutex::unlock()");
    mutex_.unlock();
    logDebug(debugLogger, "Exiting QtiRecursiveMutex::unlock()");
}


void QtiSharedMutex::lock() {
    if (wrapperBypassed()) {
        mutex_.lock();
        return;
    }

    const std::function<void(std::string)> debugLogger =
            QtiMutexDefaults::getInstance().getDebugLogger();
    const std::function<void(std::string)> deadlockLogger =
            QtiMutexDefaults::getInstance().getDeadlockLogger();
    const std::function<void()> deadlockAction =
            QtiMutexDefaults::getInstance().getDeadlockAction();

    logDebug(debugLogger, "Entering QtiSharedMutex::lock()");

    const std::chrono::steady_clock::time_point deadline =
            std::chrono::steady_clock::now() +
            std::chrono::milliseconds(QtiMutexDefaults::getInstance().getDeadlockTimeout());

    bool acquired = false;
    while (!acquired && std::chrono::steady_clock::now() < deadline) {
        acquired = mutex_.try_lock_until(deadline);
    }
    if (!acquired) {
        deadlockLogger(kTimeoutMessage);
        deadlockAction();
    }

    logDebug(debugLogger, "Exiting QtiSharedMutex::lock()");
}


void QtiSharedMutex::lock_shared() {
    if (wrapperBypassed()) {
        mutex_.lock_shared();
        return;
    }

    const std::function<void(std::string)> debugLogger =
            QtiMutexDefaults::getInstance().getDebugLogger();
    const std::function<void(std::string)> deadlockLogger =
            QtiMutexDefaults::getInstance().getDeadlockLogger();
    const std::function<void()> deadlockAction =
            QtiMutexDefaults::getInstance().getDeadlockAction();

    logDebug(debugLogger, "Entering QtiSharedMutex::lock_shared()");

    const std::chrono::steady_clock::time_point deadline =
            std::chrono::steady_clock::now() +
            std::chrono::milliseconds(QtiMutexDefaults::getInstance().getDeadlockTimeout());

    bool acquired = false;
    while (!acquired && std::chrono::steady_clock::now() < deadline) {
        acquired = mutex_.try_lock_shared_until(deadline);
    }
    if (!acquired) {
        deadlockLogger(kTimeoutMessage);
        deadlockAction();
    }

    logDebug(debugLogger, "Exiting QtiSharedMutex::lock_shared()");
}


bool QtiSharedMutex::try_lock() {
    if (wrapperBypassed()) {
        return mutex_.try_lock();
    }

    const std::function<void(std::string)> debugLogger =
            QtiMutexDefaults::getInstance().getDebugLogger();

    logDebug(debugLogger, "Entering QtiSharedMutex::try_lock()");
    const bool result = mutex_.try_lock();
    logDebug(debugLogger,
             result ? "Exiting QtiSharedMutex::try_lock() with return true"
                    : "Exiting QtiSharedMutex::try_lock() with return false");
    return result;
}

bool QtiSharedMutex::try_lock_shared() {
    if (wrapperBypassed()) {
        return mutex_.try_lock_shared();
    }

    const std::function<void(std::string)> debugLogger =
            QtiMutexDefaults::getInstance().getDebugLogger();

    logDebug(debugLogger, "Entering QtiSharedMutex::try_lock_shared()");
    const bool result = mutex_.try_lock_shared();
    logDebug(debugLogger,
             result ? "Exiting QtiSharedMutex::try_lock_shared() with return true"
                    : "Exiting QtiSharedMutex::try_lock_shared() with return false");
    return result;
}


void QtiSharedMutex::unlock() {
    if (wrapperBypassed()) {
        mutex_.unlock();
        return;
    }

    const std::function<void(std::string)> debugLogger =
            QtiMutexDefaults::getInstance().getDebugLogger();

    logDebug(debugLogger, "Entering QtiSharedMutex::unlock()");
    mutex_.unlock();
    logDebug(debugLogger, "Exiting QtiSharedMutex::unlock()");
}

void QtiSharedMutex::unlock_shared() {
    if (wrapperBypassed()) {
        mutex_.unlock_shared();
        return;
    }

    const std::function<void(std::string)> debugLogger =
            QtiMutexDefaults::getInstance().getDebugLogger();

    logDebug(debugLogger, "Entering QtiSharedMutex::unlock_shared()");
    mutex_.unlock_shared();
    logDebug(debugLogger, "Exiting QtiSharedMutex::unlock_shared()");
}


QtiShmMutex::QtiShmMutex() {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&mutex_, &attr);
}


void QtiShmMutex::lock() {
    pthread_mutex_lock(&mutex_);
}

bool QtiShmMutex::try_lock() {
    return pthread_mutex_trylock(&mutex_) != 0;
}

void QtiShmMutex::unlock() {
    pthread_mutex_unlock(&mutex_);
}


QtiShmConditionVariable::QtiShmConditionVariable() {
    pthread_condattr_t attr;
    pthread_condattr_init(&attr);
    if (pthread_condattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) != 0) {
        abort();
    }
    pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
    pthread_cond_init(&cond_, &attr);
}


QtiShmConditionVariable::~QtiShmConditionVariable() {
    pthread_cond_destroy(&cond_);
}

void QtiShmConditionVariable::notify_one() {
    pthread_cond_signal(&cond_);
}

void QtiShmConditionVariable::notify_all() {
    pthread_cond_broadcast(&cond_);
}

}


extern "C" {

QTIMUTEX_API void* qtimutex_recursive_mutex_create(void) {
    return new qtimutex::QtiRecursiveMutex();
}

QTIMUTEX_API void qtimutex_recursive_mutex_destroy(void* handle) {
    delete static_cast<qtimutex::QtiRecursiveMutex*>(handle);
}

QTIMUTEX_API void qtimutex_recursive_mutex_lock(void* handle) {
    if (handle != nullptr) {
        static_cast<qtimutex::QtiRecursiveMutex*>(handle)->lock();
    }
}

QTIMUTEX_API bool qtimutex_recursive_mutex_try_lock(void* handle) {
    return handle != nullptr && static_cast<qtimutex::QtiRecursiveMutex*>(handle)->try_lock();
}

QTIMUTEX_API void qtimutex_recursive_mutex_unlock(void* handle) {
    if (handle != nullptr) {
        static_cast<qtimutex::QtiRecursiveMutex*>(handle)->unlock();
    }
}

QTIMUTEX_API void* qtimutex_shared_mutex_create(void) {
    return new qtimutex::QtiSharedMutex();
}

QTIMUTEX_API void qtimutex_shared_mutex_destroy(void* handle) {
    delete static_cast<qtimutex::QtiSharedMutex*>(handle);
}

QTIMUTEX_API void qtimutex_shared_mutex_lock(void* handle) {
    if (handle != nullptr) {
        static_cast<qtimutex::QtiSharedMutex*>(handle)->lock();
    }
}

QTIMUTEX_API void qtimutex_shared_mutex_lock_shared(void* handle) {
    if (handle != nullptr) {
        static_cast<qtimutex::QtiSharedMutex*>(handle)->lock_shared();
    }
}

QTIMUTEX_API bool qtimutex_shared_mutex_try_lock(void* handle) {
    return handle != nullptr && static_cast<qtimutex::QtiSharedMutex*>(handle)->try_lock();
}

QTIMUTEX_API bool qtimutex_shared_mutex_try_lock_shared(void* handle) {
    return handle != nullptr && static_cast<qtimutex::QtiSharedMutex*>(handle)->try_lock_shared();
}

QTIMUTEX_API void qtimutex_shared_mutex_unlock(void* handle) {
    if (handle != nullptr) {
        static_cast<qtimutex::QtiSharedMutex*>(handle)->unlock();
    }
}

QTIMUTEX_API void qtimutex_shared_mutex_unlock_shared(void* handle) {
    if (handle != nullptr) {
        static_cast<qtimutex::QtiSharedMutex*>(handle)->unlock_shared();
    }
}




QTIMUTEX_API int __fe_getround(void) {
#if defined(__aarch64__)
    static const int kRoundModes[3] = {2, 1, 3};
    uint64_t fpcr;
    __asm__ __volatile__("mrs %0, fpcr" : "=r"(fpcr));
    const unsigned int rmode = static_cast<unsigned int>((fpcr >> 22) & 0x3);
    return rmode == 0 ? 0 : kRoundModes[rmode - 1];
#else
    return 0;
#endif
}


QTIMUTEX_API int __fe_raise_inexact(void) {
#if defined(__aarch64__)
    uint64_t fpsr;
    __asm__ __volatile__("mrs %0, fpsr" : "=r"(fpsr));
    fpsr |= 0x10;
    __asm__ __volatile__("msr fpsr, %0" ::"r"(fpsr));
#endif
    return 0;
}

}
