
#include "low-repo.h"
#include "low-package.h"

#ifndef _LOW_REPO_RPMDB_H_
#define _LOW_REPO_RPMDB_H_

LowRepo *           low_repo_rpmdb_initialize   (void);
void                low_repo_rpmdb_shutdown     (LowRepo *repo);

LowPackageIter *    low_repo_rpmdb_list_all     (LowRepo *repo);
LowPackageIter *    low_repo_rpmdb_list_by_name (LowRepo *repo,
                                                 const char *name);

#endif /* _LOW_REPO_RPMDB_H_ */
