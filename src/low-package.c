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

#include <stdlib.h>
#include <string.h>
#include "low-package.h"

LowPackageDependency *
low_package_dependency_new (const char *name, LowPackageDependencySense sense,
							const char *evr)
{
	LowPackageDependency *dep = malloc (sizeof (LowPackageDependency));

	dep->name = strdup (name);
	dep->sense = sense;
	dep->evr = strdup (evr);

	return dep;
}

LowPackageDependency *
low_package_dependency_new_from_string (const char *depstr)
{
//	char *name;
//	char *evr;
//	LowPackageDependencySense sense;
	LowPackageDependency *dep;

	/* XXX Fill me in */
	dep = NULL;

	return dep;
}

void
low_package_dependency_free (LowPackageDependency *dependency)
{
	free (dependency->name);
	free (dependency->evr);
	free (dependency);
}

/* vim: set ts=8 sw=8 noet: */
