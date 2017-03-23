#include <utils/config.h>
#include <dispatcher/common.h>
#include <utils/logger.h>
#include <tunnel/nt_distributor.h>
#include <dispatcher/dispatcher.h>
#include "nt-proxy.h"

/**
 * @author caofuxiang
 *         2015-10-29 14:03:03.
 */

static void require(error_t err, const char * message) {
    if (err != 0) {
        log_error("%s: %s", message, strerror(errno));
        exit(err);
    }
}

int main(int argc, char ** argv) {
    if (argc < 2) {
        fprintf(stderr, "Invalid arguments.\nUsage: %s <config-path>\n", argv[0]);
        return EINVAL;
    }
    const char * config_path = argv[1];
    SBUILDER(builder, 4096);

    // Init signal handlers.
    set_signal_backtrace(SIGSEGV);
    set_signal_backtrace(SIGABRT);

    // Read configuration.
    config_t config;
    config_init(&config);
    ensure(config_load_from_file(&config, config_path));

    /// Misc
    log_set_priority(log_parse_priority(config_get_default(&config, "log_priority", "ALL")));

    const char * tun_tap_name = config_get(&config, "tun_tap_name");
    int tun_tap_buffer_size = atoi(config_get(&config, "tun_tap_buffer_size"));

    address_t local_addr;
    parse_address(&local_addr, config_get(&config, "local"));

    address_t tunnel_addr;
    parse_address(&tunnel_addr, config_get(&config, "tunnel"));

    nt_proxy_t proxy;
    require(nt_proxy_init(&proxy, tun_tap_name, &local_addr, &tunnel_addr, tun_tap_buffer_size), "Error on initializing proxy");

    dispatcher_t dispatcher;
    require(dispatcher_init1(&dispatcher, 100), "Error on initializing dispatcher");

    ensure(dispatcher_add_nt_proxy(&dispatcher, &proxy) == 0);

    return dispatcher_run(&dispatcher);
}
