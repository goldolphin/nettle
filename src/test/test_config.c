/**
 * @author caofuxiang
 *         2015-07-09 10:56:56.
 */

#include <assert.h>
#include <utils/config.h>
#include <utils/logger.h>

int main() {
    SBUILDER(builder, 4096);
    const char * config_path = "src/test/conf/config.test";
    config_t config;
    config_init(&config);
    assert(config_load_from_file(&config, config_path));
    sbuilder_reset(&builder);
    sbuilder_config(&builder, &config);
    log_debug("config=\n%s", builder.buf);
    return 0;
}