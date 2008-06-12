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
#include "low-debug.h"
#include "low-package-sqlite.h"
#include "low-repo-sqlite.h"

/* XXX add this to the rpmdb repo struct */
static GHashTable *table = NULL;

static LowPackage *
low_package_sqlite_new_from_row (sqlite3_stmt *pp_stmt, LowRepo *repo)
{
	int i = 0;
	LowPackage *pkg = malloc (sizeof (LowPackage));

	if (!table) {
		low_debug ("initializing hash table\n");
		table = g_hash_table_new (NULL, NULL);
	}

	pkg = g_hash_table_lookup (table, GINT_TO_POINTER (sqlite3_column_int (pp_stmt, 0)));
	if (pkg) {
		low_package_ref (pkg);
		return pkg;
	}
	low_debug ("CACHE MISS");

	pkg = malloc (sizeof (LowPackage));
	low_package_ref_init (pkg);
	low_package_ref (pkg);

	g_hash_table_insert (table, GINT_TO_POINTER (sqlite3_column_int (pp_stmt, 0)), pkg);

	/* XXX kind of hacky */
	pkg->id = malloc (sizeof (int));
	*((int *) pkg->id) = sqlite3_column_int (pp_stmt, i++);

	pkg->name = strdup ((const char *) sqlite3_column_text (pp_stmt, i++));
	pkg->arch = strdup ((const char *) sqlite3_column_text (pp_stmt, i++));
	pkg->version =
		strdup ((const char *) sqlite3_column_text (pp_stmt, i++));
	pkg->release =
		strdup ((const char *) sqlite3_column_text (pp_stmt, i++));
	pkg->epoch =
		strdup ((const char *) sqlite3_column_text (pp_stmt, i++));

	pkg->size = sqlite3_column_int (pp_stmt, i++);
	pkg->repo = repo;

	pkg->location_href =
		strdup ((const char *) sqlite3_column_text(pp_stmt, i++));

	pkg->get_details = low_sqlite_package_get_details;

	pkg->get_provides = low_sqlite_package_get_provides;
	pkg->get_requires = low_sqlite_package_get_requires;
	pkg->get_conflicts = low_sqlite_package_get_conflicts;
	pkg->get_obsoletes = low_sqlite_package_get_obsoletes;

	pkg->provides = NULL;
	pkg->requires = NULL;

	pkg->get_files = low_sqlite_package_get_files;

	return pkg;
}

LowPackageIter *
low_sqlite_package_iter_next (LowPackageIter *iter)
{
	LowPackageIterSqlite *iter_sqlite = (LowPackageIterSqlite *) iter;

	if (sqlite3_step(iter_sqlite->pp_stmt) == SQLITE_DONE) {
		sqlite3_finalize (iter_sqlite->pp_stmt);

		if (iter_sqlite->filter_data_free_func) {
			(iter_sqlite->filter_data_free_func) (iter_sqlite->filter_data);
		}

		free (iter_sqlite);
		return NULL;
	}

	iter->pkg = low_package_sqlite_new_from_row (iter_sqlite->pp_stmt,
												 iter->repo);

	if (iter_sqlite->func != NULL) {
		/* move on to the next package if this one fails the filter */
		if (!(iter_sqlite->func) (iter->pkg, iter_sqlite->filter_data)) {
			low_package_unref (iter->pkg);
			return low_package_iter_next (iter);
		}
	}
	return iter;
}

LowPackageDetails *
low_sqlite_package_get_details (LowPackage *pkg)
{
	return low_repo_sqlite_get_details (pkg->repo, pkg);
}

LowPackageDependency **
low_sqlite_package_get_provides (LowPackage *pkg)
{
	/*
	 * XXX maybe this should all be in the same file,
	 *     or the sqlite repo struct should be in the header
	 */
	if (!pkg->provides)
		pkg->provides = low_repo_sqlite_get_provides (pkg->repo, pkg);

		return pkg->provides;
}

LowPackageDependency **
low_sqlite_package_get_requires (LowPackage *pkg)
{
	return low_repo_sqlite_get_requires (pkg->repo, pkg);
}

LowPackageDependency **
low_sqlite_package_get_conflicts (LowPackage *pkg)
{
	return low_repo_sqlite_get_conflicts (pkg->repo, pkg);
}

LowPackageDependency **
low_sqlite_package_get_obsoletes (LowPackage *pkg)
{
	return low_repo_sqlite_get_obsoletes (pkg->repo, pkg);
}

char **
low_sqlite_package_get_files(LowPackage *pkg)
{
	return low_repo_sqlite_get_files (pkg->repo, pkg);
}

/* vim: set ts=8 sw=8 noet: */
