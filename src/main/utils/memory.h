/**
 * @author caofuxiang
 *         2015-06-26 16:20:20.
 */

#ifndef NETTLE_MEMORY_H
#define NETTLE_MEMORY_H
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <stdio.h>

/**
 * Utils.
 */
#define ensure(expr) do {if(!(expr)) { \
fprintf(stderr, "Ensure fails for `%s` with errno=%d(%s) at %s:%d in %s()\n", #expr, errno, strerror(errno), __FILE__, __LINE__, __FUNCTION__); \
abort();}} while(0)

#define new_data(type) ((type *) malloc(sizeof(type)))

#define new_array(type, size) ((type *) malloc(sizeof(type)*(size)))

#define zero_data(a) memset(a, 0, sizeof *a)

#define zero_array(a, size) memset(a, 0, (sizeof *a)*(size))

#define array_size(a) ((sizeof a)/(sizeof a[0]))

#define int2ptr(n, type) ((type *)(intptr_t) (n))

#define uint2ptr(n, type) ((type *)(uintptr_t) (n))

#define ptr2int(p, type) ((type)(intptr_t) (p))

#define ptr2uint(p, type) ((type)(uintptr_t) (p))

#define container_of(ptr, type, member) ({ \
const typeof(((type *)0)->member) *__mptr = (ptr); \
(type *)((char *)__mptr - offsetof(type, member));})

#endif //NETTLE_MEMORY_H
