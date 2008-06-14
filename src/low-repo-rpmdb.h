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


#include <rpm/rpmlib.h>
#include <rpm/rpmts.h>
#include "low-repo.h"
#include "low-package.h"

#ifndef _LOW_REPO_RPMDB_H_
#define _LOW_REPO_RPMDB_H_

LowRepo *		low_repo_rpmdb_initialize 	(void);
void 			low_repo_rpmdb_shutdown 	(LowRepo *repo);

LowPackageIter *	low_repo_rpmdb_list_all 	(LowRepo *repo);
LowPackageIter *	low_repo_rpmdb_list_by_name 	(LowRepo *repo,
							 const char *name);

LowPackageIter *	low_repo_rpmdb_search_provides 	(LowRepo *repo,
							 const LowPackageDependency *provides);
LowPackageIter * 	low_repo_rpmdb_search_requires  (LowRepo *repo,
							 const LowPackageDependency *requires);
LowPackageIter * 	low_repo_rpmdb_search_conflicts (LowRepo *repo,
							 const LowPackageDependency *conflicts);
LowPackageIter *	low_repo_rpmdb_search_obsoletes (LowRepo *repo,
							 const LowPackageDependency *obsoletes);

LowPackageIter *	low_repo_rpmdb_search_files 	(LowRepo *repo,
							 const char *file);

LowPackageIter *	low_repo_rpmdb_search_details 	(LowRepo *repo,
							 const char *querystr);

LowPackageDetails *	low_repo_rpmdb_get_details 	(LowRepo *repo,
							 LowPackage *pkg);

LowPackageDependency **	low_repo_rpmdb_get_provides 	(LowRepo *repo,
							 LowPackage *pkg);
LowPackageDependency **	low_repo_rpmdb_get_requires 	(LowRepo *repo,
							 LowPackage *pkg);
LowPackageDependency **	low_repo_rpmdb_get_conflicts 	(LowRepo *repo,
							 LowPackage *pkg);
LowPackageDependency **	low_repo_rpmdb_get_obsoletes 	(LowRepo *repo,
							 LowPackage *pkg);

char **			low_repo_rpmdb_get_files 	(LowRepo *repo,
							 LowPackage *pkg);

typedef struct _LowPackageRpmdb {
	LowPackage pkg;
} LowPackageRpmdb;

typedef gboolean (*LowPackageIterFilterFn) (LowPackage *pkg, gpointer data);
typedef void 	 (*LowPackageIterFilterDataFree) (gpointer data);

typedef struct _LowPackageIterRpmdb {
	LowPackageIter super;
	rpmdbMatchIterator rpm_iter;
	LowPackageIterFilterFn func;
	gpointer filter_data;
	LowPackageIterFilterDataFree filter_data_free_func;
} LowPackageIterRpmdb;

LowPackageIter * low_package_iter_rpmdb_next (LowPackageIter *iter);

LowPackageDetails *	low_rpmdb_package_get_details	(LowPackage *pkg);

LowPackageDependency **	low_rpmdb_package_get_provides	(LowPackage *pkg);
LowPackageDependency **	low_rpmdb_package_get_requires	(LowPackage *pkg);
LowPackageDependency **	low_rpmdb_package_get_conflicts	(LowPackage *pkg);
LowPackageDependency **	low_rpmdb_package_get_obsoletes	(LowPackage *pkg);

char **		low_rpmdb_package_get_files 		(LowPackage *pkg);

#endif /* _LOW_REPO_RPMDB_H_ */

/* vim: set ts=8 sw=8 noet: */
