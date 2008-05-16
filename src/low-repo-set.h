#include <glib.h>
#include "low-config.h"
#include "low-repo.h"

#ifndef _LOW_REPO_SET_H_
#define _LOW_REPO_SET_H_

typedef struct _LowRepoSet {
    GHashTable *repos;
} LowRepoSet;

typedef gboolean (*LowRepoSetFunc) 						(LowRepo *repo);

LowRepoSet *    low_repo_set_initialize_from_config 	(LowConfig *config);
void            low_repo_set_free                      	(LowRepoSet *repo_set);

void            low_repo_set_for_each                  	(LowRepoSet *repo_set,
														 LowRepoSetFunc func);
#endif /* _LOW_REPO_SET_H_ */
