/**
 * @author caofuxiang
 *         2015-07-01 14:44:44.
 */

#include <stdio.h>
#include <utils/utils.h>
#include "nt_distributor.h"

const char * parse_address(address_t * address, const char * str) {
    char name[101];
    char port[21];
    int read;
    if (sscanf(str, "%100[^:]:%20[0-9]%n", name, port, &read) < 2) {
        return NULL;
    }
    if (address_from(address, name, port, SOCK_DGRAM)) {
        return str+read;
    }
    return NULL;
}

const char * parse_path(nt_path_t * path, const char *str) {
    str = parse_address(&path->src, str);
    if (str == NULL) return NULL;
    const char * s = skip_delimit("-", str);
    if (s == NULL) {
        path->dst = path->src;
        address_from(&path->src, "0.0.0.0", "0", 0); // Should not fail.
        return str;
    }
    return parse_address(&path->dst, s);
}

const char * parse_paths(nt_path_t * paths, int * num /* inout */, const char *str) {
    int n = 0;
    for (int i = 0; i < *num; ++i) {
        const char * s = parse_path(&paths[i], str);
        if (s != NULL) {
            n++;
            str = s;
            s = skip_delimit(",", str);
            if (s != NULL) {
                str = s;
                continue;
            }
        }
        break;
    }
    *num = n;
    return str;
}

const char * parse_seq2addr(int *seq2addr, int seq_num, int dup_num, const char * str) {
    for (int i = 0; i < seq_num; ++i) {
        for (int j = 0; j < dup_num; ++j) {
            seq2addr[i*dup_num + j] = -1;
        }
    }

    for (int i = 0; i < seq_num; ++i) {
        for (int j = 0; j < dup_num; ++j) {
            int read;
            if (sscanf(str, "%d%n", &seq2addr[i*dup_num + j], &read) >= 1) {
                str += read;
                const char *s = skip_delimit(",", str);
                if (s != NULL) {
                    str = s;
                    continue;
                }
            }
            break;
        }
        const char *s = skip_delimit(";", str);
        if (s != NULL) {
            str = s;
            continue;
        }
        break;
    }
    return str;
}

bool sbuilder_path(sbuilder_t * builder, const nt_path_t *path) {
    sbuilder_address(builder, &path->src);
    sbuilder_str(builder, "-");
    return sbuilder_address(builder, &path->dst);
}

bool nt_distributor_init(nt_distributor_t * distributor, nt_path_t * paths, int path_num, int *seq2addr, int seq_num, int dup_num) {
    if (seq_num < 0 || dup_num < 0) {
        return false;
    }
    int actual_seq_num = seq_num;
    for (int i = 0; i < seq_num; ++i) {
        if (seq2addr[i*dup_num] < 0) {
            actual_seq_num = i;
            break;
        }
        for (int j = 0; j < dup_num; ++j) {
            if (seq2addr[i*dup_num + j] >= path_num) {
                return false;
            }
        }
    }
    distributor->paths = paths;
    distributor->path_num = path_num;
    distributor->seq2addr = seq2addr;
    distributor->seq_num = actual_seq_num;
    distributor->dup_num = dup_num;
    return true;
}

bool sbuilder_distributor(sbuilder_t * builder, nt_distributor_t * distributor) {
    sbuilder_str(builder, "{path=[");
    for (int i = 0; i < distributor->path_num; ++i) {
        if (i > 0) sbuilder_str(builder, ", ");
        sbuilder_path(builder, &distributor->paths[i]);
    }
    sbuilder_str(builder, "], seq2addr=[");
    for (int i = 0; i < distributor->seq_num; ++i) {
        if (distributor->seq2addr[i*distributor->dup_num] < 0) {
            break;
        }
        if (i > 0) sbuilder_str(builder, "; ");
        for (int j = 0; j < distributor->dup_num; ++j) {
            int v = distributor->seq2addr[i*distributor->dup_num + j];
            if (v >= 0) {
                if (j > 0) sbuilder_str(builder, ", ");
                sbuilder_long(builder, v);
            }
        }
    }
    return sbuilder_str(builder, "]}");
}
