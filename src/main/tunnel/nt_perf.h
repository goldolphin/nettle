/**
 * @author caofuxiang
 *         2015-07-10 11:39:39.
 */

#ifndef NETTLE_NT_PERF_H
#define NETTLE_NT_PERF_H

#include <stddef.h>

#define COUNTER_DEF(a, b) extern const char * a;
#include "nt_perf.def"
#undef COUNTER_DEF

extern const char * nt_counter_names[];

extern const size_t nt_counter_num;

#endif //NETTLE_NT_PERF_H
