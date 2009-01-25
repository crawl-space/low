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
#include <rpm/rpmds.h>
#include "low-package.h"

static void
low_package_free (LowPackage *pkg)
{
	g_free (pkg->id);
	g_free (pkg->name);
	g_free (pkg->version);
	g_free (pkg->release);
	g_free (pkg->epoch);
	g_free (pkg->arch);

	g_free (pkg->location_href);

	if (pkg->requires) {
		low_package_dependency_list_free (pkg->requires);
	}
	if (pkg->provides) {
		low_package_dependency_list_free (pkg->provides);
	}
	if (pkg->conflicts) {
		low_package_dependency_list_free (pkg->conflicts);
	}
	if (pkg->obsoletes) {
		low_package_dependency_list_free (pkg->obsoletes);
	}

	g_free (pkg);
}

LowPackage *
low_package_ref_init (LowPackage *pkg)
{
	pkg->ref_count = 1;

	return pkg;
}

LowPackage *
low_package_ref (LowPackage *pkg)
{
	pkg->ref_count++;

	return pkg;
}

void
low_package_unref (LowPackage *pkg)
{
//      pkg->ref_count--;
	if (pkg->ref_count == 0) {
		low_package_free (pkg);
	}
}

void
low_package_details_free (LowPackageDetails *details)
{
	free (details->summary);
	free (details->description);

	free (details->url);
	free (details->license);

	free (details);
}

LowPackageDetails *
low_package_get_details (LowPackage *pkg)
{
	return (pkg->get_details) (pkg);
}

LowPackageDependency **
low_package_get_provides (LowPackage *pkg)
{
	return (pkg->get_provides) (pkg);
}

LowPackageDependency **
low_package_get_requires (LowPackage *pkg)
{
	return (pkg->get_requires) (pkg);
}

LowPackageDependency **
low_package_get_conflicts (LowPackage *pkg)
{
	return (pkg->get_conflicts) (pkg);
}

LowPackageDependency **
low_package_get_obsoletes (LowPackage *pkg)
{
	return (pkg->get_obsoletes) (pkg);
}

char **
low_package_get_files (LowPackage *pkg)
{
	return (pkg->get_files) (pkg);
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
	/* XXX should check that evr is empty iff sense is NONE */
	LowPackageDependency *dep = malloc (sizeof (LowPackageDependency));

	dep->name = strdup (name);
	dep->sense = sense;
	/* EVR can be empty */
	dep->evr = evr ? strdup (evr) : NULL;

	return dep;
}

LowPackageDependencySense
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

void
low_package_dependency_list_free (LowPackageDependency **dependencies)
{
	int i;

	for (i = 0; dependencies[i] != NULL; i++) {
		low_package_dependency_free (dependencies[i]);
	}

	free (dependencies);
}

static rpmsenseFlags
low_dependency_sense_to_rpm (LowPackageDependencySense sense)
{
	switch (sense) {
		case DEPENDENCY_SENSE_LT:
			return RPMSENSE_LESS;
		case DEPENDENCY_SENSE_LE:
			return RPMSENSE_LESS | RPMSENSE_EQUAL;
		case DEPENDENCY_SENSE_EQ:
			return RPMSENSE_EQUAL;
		case DEPENDENCY_SENSE_GE:
			return RPMSENSE_GREATER | RPMSENSE_EQUAL;
		case DEPENDENCY_SENSE_GT:
			return RPMSENSE_GREATER;
		case DEPENDENCY_SENSE_NONE:
		default:
			/* XXX handle better */
			return 0;
	}
}

gboolean
low_package_dependency_satisfies (const LowPackageDependency *needs,
				  const LowPackageDependency *satisfies)
{
	int res;

	/* The tag type doesn't matter for comparison, so just use a default */
	rpmds rpm_needs = rpmdsSingle (RPMTAG_PROVIDES, needs->name, needs->evr,
				       low_dependency_sense_to_rpm
				       (needs->sense));
	rpmds rpm_satisfies =
		rpmdsSingle (RPMTAG_PROVIDES, satisfies->name, satisfies->evr,
			     low_dependency_sense_to_rpm (satisfies->sense));

	res = rpmdsCompare (rpm_needs, rpm_satisfies);

	rpmdsFree (rpm_needs);
	rpmdsFree (rpm_satisfies);

	return res == 1 ? TRUE : FALSE;
}

char *
low_package_evr_as_string (LowPackage *pkg)
{
	return g_strdup_printf ("%s:%s-%s", pkg->epoch ? pkg->epoch : "0",
				pkg->version, pkg->release);
}

/* vim: set ts=8 sw=8 noet: */
