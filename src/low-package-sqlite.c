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
#include "low-package-sqlite.h"

static LowPackage *
low_package_sqlite_new_from_row (sqlite3_stmt *pp_stmt, LowRepo *repo)
{
	int i = 0;
	LowPackage *pkg = malloc (sizeof (LowPackage));

	pkg->name = (const char *) sqlite3_column_text (pp_stmt, i++);
	pkg->arch = (const char *) sqlite3_column_text (pp_stmt, i++);
	pkg->version = (const char *) sqlite3_column_text (pp_stmt, i++);
	pkg->release = (const char *) sqlite3_column_text (pp_stmt, i++);

	pkg->size = sqlite3_column_int (pp_stmt, i++);
	pkg->repo = repo->id;
	pkg->summary = (const char *) sqlite3_column_text (pp_stmt, i++);
	pkg->description = (const char *) sqlite3_column_text (pp_stmt, i++);
	pkg->url = (const char *) sqlite3_column_text (pp_stmt, i++);
	pkg->license = (const char *) sqlite3_column_text (pp_stmt, i++);
	return pkg;
}

LowPackageIter *
low_sqlite_package_iter_next (LowPackageIter *iter)
{
	LowPackageIterSqlite *iter_sqlite = (LowPackageIterSqlite *) iter;

	if (sqlite3_step(iter_sqlite->pp_stmt) == SQLITE_DONE) {
		sqlite3_finalize (iter_sqlite->pp_stmt);
		free (iter_sqlite);
		return NULL;
	}

	if (iter->pkg != NULL) {
		free (iter->pkg);
	}
	iter->pkg = low_package_sqlite_new_from_row (iter_sqlite->pp_stmt,
												 iter->repo);

	return iter;
}
