/**
 * @author caofuxiang
 *         2015-07-01 14:44:44.
 */

#ifndef NETTLE_NT_DISTRIBUTOR_H
#define NETTLE_NT_DISTRIBUTOR_H

#include <dispatcher/common.h>

typedef struct {
    address_t src;
    address_t dst;
} nt_path_t;

const char * parse_address(address_t * address, const char * str);

const char * parse_path(nt_path_t * path, const char *str);

const char * parse_paths(nt_path_t * paths, int * num /* inout */, const char *str);

const char * parse_seq2addr(int *seq2addr, int seq_num, int dup_num, const char * str);

bool sbuilder_path(sbuilder_t * builder, const nt_path_t *path);

typedef struct {
    nt_path_t * paths;
    int path_num;
    int *seq2addr;
    int seq_num;
    int dup_num;
} nt_distributor_t;

bool nt_distributor_init(nt_distributor_t * distributor, nt_path_t * paths, int path_num, int *seq2addr, int seq_num, int dup_num);

static inline nt_path_t * nt_distributor_select(nt_distributor_t * distributor, int seq, int dup) {
    if(dup >= distributor->dup_num) return NULL;
    int n = distributor->seq2addr[(seq % distributor->seq_num)*distributor->dup_num + dup];
    if (n < 0) return NULL;
    return &distributor->paths[n];
}

bool sbuilder_distributor(sbuilder_t * builder, nt_distributor_t * distributor);

#endif //NETTLE_NT_DISTRIBUTOR_H
