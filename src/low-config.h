#include <glib.h>

#ifndef _LOW_CONFIG_H_
#define _LOW_CONFIG_H_

typedef struct _LowConfig {
    GKeyFile *config;
} LowConfig;

LowConfig *     low_config_initialize       (void);
void            low_config_free             (LowConfig *config);

char **         low_config_get_repo_names   (LowConfig *config);

#endif /* _LOW_CONFIG_H_ */
