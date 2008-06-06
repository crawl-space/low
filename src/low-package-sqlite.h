/*
 *  Low: a yum-like package manager
 *
 *  Copyright (C) 2008 James Bowes <jbowes@dangerouslyinc.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301  USA
 */

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

LowPackageIter * low_sqlite_package_iter_next	(LowPackageIter *iter);

LowPackageDetails *	low_sqlite_package_get_details	(LowPackage *pkg);

LowPackageDependency **	low_sqlite_package_get_provides	 (LowPackage *pkg);
LowPackageDependency **	low_sqlite_package_get_requires	 (LowPackage *pkg);
LowPackageDependency **	low_sqlite_package_get_conflicts (LowPackage *pkg);
LowPackageDependency **	low_sqlite_package_get_obsoletes (LowPackage *pkg);

char **		low_sqlite_package_get_files		(LowPackage *pkg);

#endif /* _LOW_PACKAGE_SQLITE_H_ */

/* vim: set ts=8 sw=8 noet: */
