#include <stdlib.h>
#include "low-package-sqlite.h"

static LowPackage *
low_package_sqlite_new_from_row (sqlite3_stmt *pp_stmt)
{
	int i = 0;
	LowPackage *pkg = malloc (sizeof (LowPackage));

	pkg->name = (const char *) sqlite3_column_text (pp_stmt, i++);
	pkg->arch = (const char *) sqlite3_column_text (pp_stmt, i++);
	pkg->version = (const char *) sqlite3_column_text (pp_stmt, i++);
	pkg->release = (const char *) sqlite3_column_text (pp_stmt, i++);
	
	pkg->size = sqlite3_column_int (pp_stmt, i++);
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
	iter->pkg = low_package_sqlite_new_from_row (iter_sqlite->pp_stmt);

	return iter;
}
