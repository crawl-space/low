/*
 *  Low: a yum-like package manager
 *
 *  Copyright (C) 2010 James Bowes <jbowes@repl.ca>
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

#ifndef _LOW_SQLITE_IMPORTER_H_
#define _LOW_SQLITE_IMPORTER_H_

#include <stdbool.h>

#include <sqlite3.h>

typedef enum {
	DEPENDENCY_TYPE_PROVIDES,
	DEPENDENCY_TYPE_REQUIRES,
	DEPENDENCY_TYPE_CONFLICTS,
	DEPENDENCY_TYPE_OBSOLETES
} LowSqliteImporterDepType;

typedef struct _LowSqliteImporter {
	sqlite3 *primary_db;
	sqlite3 *filelists_db;
} LowSqliteImporter;

LowSqliteImporter *low_sqlite_importer_new (const char *directory);
void low_sqlite_importer_begin_package (LowSqliteImporter *importer,
					const char *name,
					const char *arch,
					const char *epoch,
					const char *version,
					const char *release);
void low_sqlite_importer_add_details (LowSqliteImporter *importer,
				      const char *summary,
				      const char *description,
				      const char *url,
				      const char *license);
void low_sqlite_importer_add_dependency (LowSqliteImporter *importer,
					 const char *name,
					 const char *sense,
					 LowSqliteImporterDepType type,
					 bool is_pre,
					 const char *epoch,
					 const char *version,
					 const char *release);
void low_sqlite_importer_add_file (LowSqliteImporter *importer,
				   const char *name);
void low_sqlite_importer_finish_package (LowSqliteImporter *importer);
void low_sqlite_importer_free (LowSqliteImporter *importer);

#endif /* _LOW_SQLITE_IMPORTER_H_ */

/* vim: set ts=8 sw=8 noet: */
