/**
 * @author caofuxiang
 *         2015-05-11 19:35:35.
 */

#ifndef NETTLE_BITMAP_H
#define NETTLE_BITMAP_H

#include <stdbool.h>

struct bitmap_s;
typedef struct bitmap_s bitmap_t;

bitmap_t * make_bitmap(size_t size);

void destroy_bitmap(bitmap_t * bitmap);

bool bitmap_set(bitmap_t * bitmap, size_t key);

bool bitmap_unset(bitmap_t * bitmap, size_t key);

bool bitmap_isset(bitmap_t * bitmap, size_t key);

#endif //NETTLE_BITMAP_H
