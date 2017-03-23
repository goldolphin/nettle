/**
 * @author caofuxiang
 *         2015-05-28 14:57:57.
 */

#include <stddef.h>
#include <stdlib.h>
#include "utils.h"
#include "metric.h"

#define INVALID_VALUE -1

struct metric_s {
    size_t capacity;
    long begin_time;
    long end_time;
    long min;
    long max;
    size_t count;
    size_t begin;
    long times[0];
};

#define FORWARD(n) ((metric->begin+(n))%metric->capacity)

metric_t * make_metric(size_t capacity) {
    size_t n_bytes = capacity * sizeof(long);
    metric_t * metric = (metric_t *)malloc(sizeof(metric_t) + n_bytes);
    metric->capacity = capacity;
    metric->begin_time = 0;
    metric->end_time = 0;
    metric->min = INVALID_VALUE;
    metric->max = INVALID_VALUE;
    metric->count = 0;
    metric->begin = 0;
    memset(metric->times, 0, n_bytes);
    return metric;
}

void destroy_metric(metric_t * metric) {
    free(metric);
}

void metric_record(metric_t * metric, long time) {
    long current = current_millis();
    if (metric->begin_time == 0) {
        metric->begin_time = current;
    }
    metric->end_time = current;

    if (metric->min == INVALID_VALUE || time < metric->min) {
        metric->min = time;
    }

    if (metric->max == INVALID_VALUE || time > metric->max) {
        metric->max = time;
    }

    metric->count ++;
    metric->times[metric->begin] = time;
    metric->begin = FORWARD(1);
}

double metric_qps(metric_t * metric) {
    if (metric->count < 2) {
        return 0;
    }
    return 1000.0 * (metric->count-1)/(metric->end_time - metric->begin_time);
}

long metric_min(metric_t * metric) {
    return metric->min;
}

long metric_max(metric_t * metric) {
    return metric->max;
}

double metric_mean(metric_t * metric) {
    size_t len = metric->count < metric->capacity ? metric->count : metric->capacity;
    if (len == 0) return INVALID_VALUE;
    double sum = 0;
    for (size_t i = 0; i < len; ++i) {
        sum += metric->times[i];
    }
    return sum/len;
}

long nth_min(long * data, size_t len, size_t n) {
    if (len == 1) {
        return data[0];
    }
    long pivot = data[len-1];

    size_t wi = 0;
    for (int i = 0; i < len-1; ++i) {
        if (data[i] < pivot) {
            long t = data[wi];
            data[wi++] = data[i];
            data[i] = t;
        }
    }
    data[len-1] = data[wi];
    data[wi] = pivot;

    if (n <= wi) {
        return nth_min(data, wi, n);
    } else if (n == wi+1) {
        return data[wi];
    } else {
        return nth_min(data+wi+1, len-wi-1, n-wi-1);
    }
}

long metric_ratio(metric_t * metric, double ratio) {
    size_t len = metric->count < metric->capacity ? metric->count : metric->capacity;
    if (len == 0) return INVALID_VALUE;
    size_t n = (size_t) (ratio*len);
    return nth_min(metric->times, len, n);
}