#include <rpm/rpmlib.h>
#include <rpm/rpmts.h>
#include "low-package.h"

#ifndef _LOW_PACKAGE_RPMDB_H_
#define _LOW_PACKAGE_RPMDB_H_

typedef struct _LowPackageRpmdb {
    LowPackage pkg;
} LowPackageRpmdb;

typedef struct _LowPackageIterRpmdb {
    LowPackageIter super;
    rpmdbMatchIterator rpm_iter;
} LowPackageIterRpmdb;

LowPackageIter * low_package_iter_next (LowPackageIter *iter);

#endif /* _LOW_PACKAGE_RPMDB_H_ */
