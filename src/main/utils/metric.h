/**
 * @author caofuxiang
 *         2015-05-28 14:57:57.
 */

#ifndef NETTLE_METRIC_H
#define NETTLE_METRIC_H

struct metric_s;

typedef struct metric_s metric_t;

metric_t * make_metric(size_t capacity);

void destroy_metric(metric_t * metric);

void metric_record(metric_t * metric, long time);

double metric_qps(metric_t * metric);

long metric_min(metric_t * metric);

long metric_max(metric_t * metric);

double metric_mean(metric_t * metric);

long metric_ratio(metric_t * metric, double ratio);


#endif //NETTLE_METRIC_H
