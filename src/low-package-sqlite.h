#include <sqlite3.h>
#include "low-package.h"

#ifndef _LOW_PACKAGE_SQLITE_H_
#define _LOW_PACKAGE_SQLITE_H_

typedef struct _LowPackageSqlite {
    LowPackage pkg;
} LowPackageSqlite;

typedef struct _LowPackageIterSqlite {
    LowPackageIter super;
    sqlite3_stmt *pp_stmt;
} LowPackageIterSqlite;

LowPackageIter * low_sqlite_package_iter_next (LowPackageIter *iter);

#endif /* _LOW_PACKAGE_SQLITE_H_ */
