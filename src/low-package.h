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

#ifndef _LOW_PACKAGE_H_
#define _LOW_PACKAGE_H_

#include "low-repo.h"

typedef enum {
	DEPENDENCY_SENSE_EQ,
	DEPENDENCY_SENSE_GT,
	DEPENDENCY_SENSE_GE,
	DEPENDENCY_SENSE_LT,
	DEPENDENCY_SENSE_LE,
	DEPENDENCY_SENSE_NONE
} LowPackageDependencySense;

/**
 * A struct representing a package dependency.
 *
 * For instance: "foobar >= 1.2-3"
 */
typedef struct _LowPackageDependency {
	char *name;
	LowPackageDependencySense sense;
	char *evr;
} LowPackageDependency;

typedef struct _LowPackage LowPackage;

typedef void * signature;
typedef char ** (*LowPackageGetDependency) (LowPackage *);

struct _LowPackage {
	signature id; /** Repo type dependent package identifier */

	const char *name;
	const char *version;
	const char *release;
	const char *arch;
	const char *epoch;

	size_t size;
	LowRepo *repo;
	char *summary;
	char *description;
	const char *url; /** Optional URL for the package */
	const char *license;
	const char *location_href;

	LowPackageGetDependency get_provides;
};

typedef struct _LowPackageIter LowPackageIter;

typedef LowPackageIter * (*LowPackageIterNextFunc) (LowPackageIter *iter);

struct _LowPackageIter {
	LowRepo *repo;
	LowPackage *pkg;
	LowPackageIterNextFunc next_func;
};

void 			low_package_free 	(LowPackage *pkg);

char **			low_package_get_provides 	(LowPackage *pkg);

LowPackageIter * 	low_package_iter_next 	(LowPackageIter *iter);

LowPackageDependency *	low_package_dependency_new 		(const char *name,
								 LowPackageDependencySense sense,
								 const char *evr);
LowPackageDependency * 	low_package_dependency_new_from_string 	(const char *depstr);
void 			low_package_dependency_free 		(LowPackageDependency *dependency);

#endif /* _LOW_PACKAGE_H_ */

/* vim: set ts=8 sw=8 noet: */
