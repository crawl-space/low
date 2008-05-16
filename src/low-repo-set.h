#include <glib.h>
#include "low-config.h"
#include "low-repo.h"

#ifndef _LOW_REPO_SET_H_
#define _LOW_REPO_SET_H_

typedef struct _LowRepoSet {
    GHashTable *repos;
} LowRepoSet;

typedef enum {
	ENABLED,
	DISABLED,
	ALL
} LowRepoSetFilter;

typedef gboolean (*LowRepoSetFunc) 						(LowRepo *repo,
														 gpointer data);

LowRepoSet *    low_repo_set_initialize_from_config 	(LowConfig *config);
void            low_repo_set_free                      	(LowRepoSet *repo_set);

void            low_repo_set_for_each                  	(LowRepoSet *repo_set,
														 LowRepoSetFilter filter,
														 LowRepoSetFunc func,
														 gpointer data);

LowPackageIter * 	low_repo_set_search_provides      	(LowRepoSet *repo_set,
														 const char *provides);

#endif /* _LOW_REPO_SET_H_ */
