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
#include <glib.h>
#include "low-package.h"

void
low_package_free (LowPackage *pkg)
{
	free (pkg->summary);
	free (pkg->description);

	free (pkg);
}

LowPackageIter *
low_package_iter_next (LowPackageIter *iter)
{
	return (iter->next_func) (iter);
}

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

static LowPackageDependencySense
low_package_dependency_sense_from_string (const char *sensestr)
{
	if (!strcmp ("=", sensestr)) {
		return DEPENDENCY_SENSE_EQ;
	} else if (!strcmp (">", sensestr)) {
		return DEPENDENCY_SENSE_GT;
	} else if (!strcmp (">=", sensestr)) {
		return DEPENDENCY_SENSE_GE;
	} else if (!strcmp ("<", sensestr)) {
		return DEPENDENCY_SENSE_LT;
	} else if (!strcmp ("<=", sensestr)) {
		return DEPENDENCY_SENSE_LE;
	} else {
		/* XXX be smarter here */
		exit (1);
	}
}

LowPackageDependency *
low_package_dependency_new_from_string (const char *depstr)
{
	int length;
	gchar **split;
	LowPackageDependency *dep = malloc (sizeof (LowPackageDependency));

	split = g_strsplit (depstr, " ", 3);

	for (length = 0; split[length] != NULL; length++) ;

	if (length == 3) {
		dep->name = g_strdup (split[0]);
		dep->sense =
			low_package_dependency_sense_from_string (split[1]);
		dep->evr = g_strdup (split[2]);
	} else if (length == 1) {
		dep->name = g_strdup (split[0]);
		dep->sense = DEPENDENCY_SENSE_NONE;
		dep->evr = NULL;
	} else {
		/* XXX do better here */
		exit (1);
	}

	g_strfreev (split);
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
