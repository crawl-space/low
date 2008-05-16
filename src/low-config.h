#include <glib.h>

#ifndef _LOW_CONFIG_H_
#define _LOW_CONFIG_H_

typedef struct _LowConfig {
    GKeyFile *config;
} LowConfig;

LowConfig *     low_config_initialize       (void);
void            low_config_free             (LowConfig *config);

char **         low_config_get_repo_names   (LowConfig *config);

char *          low_config_get_string       (LowConfig *config,
                                             const char *group,
                                             const char *key);
gboolean        low_config_get_boolean      (LowConfig *config,
                                             const char *group,
                                             const char *key);

#endif /* _LOW_CONFIG_H_ */
