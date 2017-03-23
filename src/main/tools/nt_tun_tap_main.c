/**
 * @author caofuxiang
 *         2015-10-27 20:47:47.
 */
#include <utils/sbuilder.h>
#include <dispatcher/tun_tap.h>
#include <utils/logger.h>

typedef enum {
    OP_CREATE,
    OP_DELETE,
} op_t;

typedef struct {
    op_t op;
    const char *name;
    bool is_tap;
    bool no_pi;
} options_t;

bool options_parse(options_t * options, int argc, char ** argv) {
    if (argc < 2) return false;
    if (strcmp(argv[1], "create") == 0) {
        options->op = OP_CREATE;
    } else if (strcmp(argv[1], "delete") == 0) {
        options->op = OP_DELETE;
    } else {
        return false;
    }

    options->name = NULL;
    options->is_tap = false;
    options->no_pi = false;
    for (int i = 2; i < argc; ++i) {
        if (strcmp(argv[i], "-tap") == 0) {
            options->is_tap = true;
        } else if (strcmp(argv[i], "-nopi") == 0) {
            options->no_pi = true;
        } else if (argv[i][0] != '-' && options->name == NULL) {
            options->name = argv[i];
        } else {
            return false;
        }
    }
    if (options->op == OP_DELETE && options->name == NULL) {
        return false;
    }

    return true;
}

static void require(error_t err, const char * message) {
    if (err != 0) {
        log_error("%s: %s", message, strerror(errno));
        exit(err);
    }
}

int main(int argc, char ** argv) {
    options_t options;
    if (!options_parse(&options, argc, argv)) {
        fprintf(stderr, "Invalid arguments.\nUsage:\n"
                "\t%s create [-tap] [-nopi] [name]\n"
                "\t%s delete <name>\n", argv[0], argv[0]);
        return EINVAL;
    }

    if (options.op == OP_CREATE) {
        tun_tap_t tun_tap;
        require(tun_tap_init(&tun_tap, options.name, options.is_tap, options.no_pi), "Error init tuntap");
        require(tun_tap_set_persist(&tun_tap, true), "Error set persist on tuntap");
        log_info("Successfully create %s device: %s.", options.is_tap ? "tap" : "tun", tun_tap.name);
        require(tun_tap_close(&tun_tap), "Error close tun_tap fd");
    } else if (options.op == OP_DELETE) {
        tun_tap_t tun_tap;
        require(tun_tap_init(&tun_tap, options.name, options.is_tap, options.no_pi), "Error init tuntap");
        require(tun_tap_set_persist(&tun_tap, false), "Error remove persist on tuntap");
        require(tun_tap_close(&tun_tap), "Error close tun_tap fd");
        log_info("Successfully delete %s device: %s.", options.is_tap ? "tap" : "tun", tun_tap.name);
    }
    return 0;
}
