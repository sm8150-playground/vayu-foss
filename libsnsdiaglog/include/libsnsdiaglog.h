/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIB_SNSDIAGLOG_H
#define LIB_SNSDIAGLOG_H



namespace std {
inline namespace __1 {

template <class T>
class allocator {};

template <class C>
struct char_traits {};

template <class C, class T, class A>
class basic_string {
  public:
    unsigned long size() const {
        return (_b[0] & 1) ? *(const unsigned long *)(_b + 8)
                           : (unsigned long)(unsigned char)(_b[0] >> 1);
    }
    const char *data() const {
        return (_b[0] & 1) ? *(const char *const *)(_b + 16) : _b + 1;
    }
    bool empty() const { return size() == 0; }

  private:
    char _b[24];
};

}
}

typedef std::basic_string<char, std::char_traits<char>, std::allocator<char> > string_t;


enum sensors_diag_logid {
    SENSORS_DIAG_LOG_REQUEST = 100,
    SENSORS_DIAG_LOG_RESPONSE = 101,
    SENSORS_DIAG_LOG_EVENT = 102,
};


struct sensors_diag_log_mutex {
    unsigned long __opaque[5];
};


struct sensors_diag_log {
    bool diag_flag;
    unsigned char reserved[7];
    unsigned long timestamp;
    sensors_diag_log_mutex mutex;

    __attribute__((visibility("default")))
    sensors_diag_log();
    __attribute__((visibility("default")))
    ~sensors_diag_log();

    __attribute__((visibility("default")))
    int log_request_msg(const string_t &request, const string_t &pretty_request, char *logging_module_type);
    __attribute__((visibility("default")))
    int log_event_msg(const string_t &event, const string_t &pretty_event, char *logging_module_type);
    __attribute__((visibility("default")))
    int log_response_msg(unsigned int response_id, const string_t &response, char *logging_module_type);

    __attribute__((visibility("default")))
    int submit_log_packet(sensors_diag_logid id, string_t &serialized_log)
        __attribute__((noinline));


    __attribute__((visibility("default")))
    int sensors_diag_log_get_diag_log_id(sensors_diag_logid id);
};

#endif
