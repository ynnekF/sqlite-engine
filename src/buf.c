#include "buf.h"

InputBuffer*
inbuf_new(size_t initial_capacity) {
        InputBuffer* buf = malloc(sizeof(InputBuffer));
        if (!buf) {
                return NULL; // Memory allocation failed
        }
        buf->data = malloc(initial_capacity);
        if (!buf->data) {
                free(buf);
                return NULL; // Memory allocation failed
        }
        buf->size = 0;
        buf->capacity = initial_capacity;
        return buf;
}

void
inbuf_free(InputBuffer* buf) {
        if (buf) {
                free(buf->data);
                free(buf);
        }
}
