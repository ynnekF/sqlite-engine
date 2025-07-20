#ifndef BUF_H
#define BUF_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
        char* data;
        size_t size;
        size_t capacity;
} InputBuffer;

InputBuffer* inbuf_new(size_t initial_capacity);
void inbuf_free(InputBuffer* buf);

#endif // BUF_H