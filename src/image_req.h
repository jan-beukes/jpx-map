#ifndef IMAGE_REQ_H
#define IMAGE_REQ_H

#include <stdio.h>
#include <stdint.h>

typedef struct {
    uint8_t *memory;
    size_t size;
} MemChunk;

MemChunk fetch_image(const char *url, size_t len);

#endif
