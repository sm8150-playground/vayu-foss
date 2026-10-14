/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "record_stream.h"

#define RECORD_STREAM_MAGIC_OFFSET 0

struct record_stream {
    int fd;
    size_t size;
    char *buffer;
    char *read_ptr;
    char *write_ptr;
    char *end;
};


static uint32_t payload_length(const char *p)
{
    const unsigned char *u = (const unsigned char *)p;

    return ((uint32_t)u[0] << 24) | ((uint32_t)u[1] << 16) |
           ((uint32_t)u[2] << 8) | (uint32_t)u[3];
}

void *record_stream_new(int fd, size_t size)
{
    struct record_stream *stream = calloc(1, sizeof(struct record_stream));
    char *buffer;

    if (stream == NULL)
        return NULL;

    buffer = malloc(size + 4);
    if (buffer == NULL) {
        free(stream);
        return NULL;
    }

    stream->fd = fd;
    stream->size = size;
    stream->buffer = buffer;
    stream->read_ptr = buffer;
    stream->write_ptr = buffer;
    stream->end = buffer + size + 4;

    return stream;
}

int record_stream_get_next(void *stream_ptr, void **out_data,
                           unsigned int *out_len)
{
    struct record_stream *stream = stream_ptr;
    char *read_ptr = stream->read_ptr;
    char *write_ptr = stream->write_ptr;
    uint32_t length;
    ssize_t got;


    if (read_ptr + 4 <= write_ptr) {
        length = payload_length(read_ptr);
        if (read_ptr + 4 + length <= write_ptr) {
            *out_len = length;
            *out_data = read_ptr + 4;
            stream->read_ptr = read_ptr + 4 + length;
            return 0;
        }
    }


    if (read_ptr != stream->buffer) {
        size_t remaining = (size_t)(write_ptr - read_ptr);

        if (remaining != 0)
            memmove(stream->buffer, read_ptr, remaining);
        stream->read_ptr = stream->buffer;
        write_ptr = stream->buffer + remaining;
        stream->write_ptr = write_ptr;
    } else if (write_ptr == stream->end) {

        errno = EFBIG;
        return -1;
    }

    got = read(stream->fd, stream->write_ptr, (size_t)(stream->end - stream->write_ptr));
    if (got <= 0) {
        *out_data = NULL;
        return (int)got;
    }

    write_ptr = stream->write_ptr + got;
    stream->write_ptr = write_ptr;
    read_ptr = stream->read_ptr;

    if (read_ptr + 4 > write_ptr) {
        errno = EAGAIN;
        return -1;
    }

    length = payload_length(read_ptr);
    if (read_ptr + 4 + length > write_ptr) {
        errno = EAGAIN;
        return -1;
    }

    *out_len = length;
    *out_data = read_ptr + 4;
    stream->read_ptr = read_ptr + 4 + length;
    return 0;
}

void record_stream_free(void *stream_ptr)
{
    struct record_stream *stream = stream_ptr;

    free(stream->buffer);
    free(stream);
}

unsigned long long ril_nano_time(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (unsigned long long)ts.tv_sec * 1000000000ULL +
           (unsigned long long)ts.tv_nsec;
}
