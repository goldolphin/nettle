#include <stdio.h>
#include <errno.h>
#include <utils/sbuilder.h>
#include <utils/config.h>

/**
 * @author caofuxiang
 *         2015-08-19 14:40:40.
 */

int main(int argc, char ** argv) {
    if (argc < 3) {
        fprintf(stderr, "Invalid arguments.\nUsage: %s <config-path> <var-name>\n", argv[0]);
        return EINVAL;
    }
    const char *config_path = argv[1];
    const char *var_name = argv[2];
    SBUILDER(builder, 4096);

    // Read configuration.
    config_t config;
    config_init(&config);
    ensure(config_load_from_file(&config, config_path));

    const char * var_value = config_get(&config, var_name);
    if (var_value == NULL) {
        fprintf(stderr, "Variable not exist!\n");
        return 1;
    }
    printf("%s", var_value);
    return 0;
}
