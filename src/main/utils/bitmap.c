/**
 * @author caofuxiang
 *         2015-05-11 19:35:35.
 */

#include <stdlib.h>
#include "utils.h"
#include "bitmap.h"

struct bitmap_s {
    size_t size;
    size_t n_bytes;
    unsigned char buffer[0];
};

bitmap_t * make_bitmap(size_t size) {
    size_t n_bytes = (size+7)/8;
    bitmap_t * bitmap = (bitmap_t *)malloc(sizeof(bitmap_t) + n_bytes);
    bitmap->size = size;
    bitmap->n_bytes = n_bytes;
    memset(bitmap->buffer, 0, n_bytes);
    return bitmap;
}

void destroy_bitmap(bitmap_t * bitmap) {
    free(bitmap);
}

bool bitmap_set(bitmap_t * bitmap, size_t key) {
    if (key >= bitmap->size) return false;
    size_t n = key / 8;
    size_t m = key % 8;
    bitmap->buffer[n] |= (1 << m);
    return true;
}

bool bitmap_unset(bitmap_t * bitmap, size_t key) {
    if (key >= bitmap->size) return false;
    size_t n = key / 8;
    size_t m = key % 8;
    bitmap->buffer[n] &= ~(1 << m);
    return true;
}

bool bitmap_isset(bitmap_t * bitmap, size_t key) {
    if (key >= bitmap->size) return false;
    size_t n = key / 8;
    size_t m = key % 8;
    return (bitmap->buffer[n] & (1 << m)) != 0;
}
