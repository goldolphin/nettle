/**
 * @author caofuxiang
 *         2015-07-10 11:39:39.
 */

#include <utils/memory.h>
#include "nt_perf.h"

#define COUNTER_DEF(a, b) const char * a = b;
#include "nt_perf.def"
#undef COUNTER_DEF

const char * nt_counter_names[] = {
#define COUNTER_DEF(a, b) b,
#include "nt_perf.def"
#undef COUNTER_DEF
};

const size_t nt_counter_num = array_size(nt_counter_names);
