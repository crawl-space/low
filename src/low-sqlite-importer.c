/*
 *  Low: a yum-like package manager
 *
 *  Copyright (C) 2010 James Bowes <jbowes@repl.ca>
 *
 *  adapted from yum-metadata-parser
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
#include <unistd.h>

#include <glib.h>

#include "low-debug.h"

#include "low-sqlite-importer.h"

static sqlite3_stmt *
primary_package_prepare (sqlite3 *db)
{
	int rc;
	sqlite3_stmt *handle = NULL;
	const char *query;

	query = "INSERT INTO packages ("
		"  name, arch, epoch, version, release"
		")" "VALUES (?, ?, ?, ?, ?)";

	rc = sqlite3_prepare (db, query, -1, &handle, NULL);
	if (rc != SQLITE_OK) {
		sqlite3_finalize (handle);
		handle = NULL;
	}

	return handle;
}

static int
primary_package_write (sqlite3 *db, sqlite3_stmt *handle, const char *name,
		       const char *arch, const char *epoch, const char *version,
		       const char *release)
{
	int rc;

	sqlite3_bind_text (handle, 1, name, -1, SQLITE_STATIC);
	sqlite3_bind_text (handle, 2, arch, -1, SQLITE_STATIC);
	sqlite3_bind_text (handle, 3, version, -1, SQLITE_STATIC);
	sqlite3_bind_text (handle, 4, epoch, -1, SQLITE_STATIC);
	sqlite3_bind_text (handle, 5, release, -1, SQLITE_STATIC);

	rc = sqlite3_step (handle);
	sqlite3_reset (handle);

	return sqlite3_last_insert_rowid (db);
}

void
low_sqlite_importer_begin_package (LowSqliteImporter *importer,
				   const char *name, const char *arch,
				   const char *epoch,
				   const char *version, const char *release)
{
	low_debug ("begin package: %s-%s-%s.%s", name, version, release, arch);
	importer->row_id = primary_package_write (importer->primary_db,
						  importer->pkg_stmt, name,
						  arch, epoch, version,
						  release);
}

void
low_sqlite_importer_add_details (LowSqliteImporter *importer G_GNUC_UNUSED,
				 const char *summary G_GNUC_UNUSED,
				 const char *description G_GNUC_UNUSED,
				 const char *url G_GNUC_UNUSED,
				 const char *license G_GNUC_UNUSED)
{
	low_debug ("add details");
}

void
low_sqlite_importer_add_dependency (LowSqliteImporter *importer G_GNUC_UNUSED,
				    const char *name,
				    const char *sense G_GNUC_UNUSED,
				    LowSqliteImporterDepType type G_GNUC_UNUSED,
				    bool is_pre G_GNUC_UNUSED,
				    const char *epoch G_GNUC_UNUSED,
				    const char *version G_GNUC_UNUSED,
				    const char *release G_GNUC_UNUSED)
{
	low_debug ("dep - %s", name);
}

void
low_sqlite_importer_add_file (LowSqliteImporter *importer G_GNUC_UNUSED,
			      const char *name)
{
	low_debug ("file - %s", name);
}

void
low_sqlite_importer_finish_package (LowSqliteImporter *importer G_GNUC_UNUSED)
{

}

static void
create_dbinfo_table (sqlite3 *db)
{
	const char *sql;

	sql = "CREATE TABLE db_info (dbversion INTEGER, checksum TEXT)";
	sqlite3_exec (db, sql, NULL, NULL, NULL);
	/* XXX populate dbinfo */
}

#define DEP_TABLE \
        "CREATE TABLE %s (" \
        "  name TEXT," \
        "  flags TEXT," \
        "  epoch TEXT," \
        "  version TEXT," \
        "  release TEXT," \
        "  pkgKey INTEGER %s)"

static void
create_primary_tables (sqlite3 *db)
{
	int rc;
	const char *sql;

	const char *deps[] = { "requires", "provides", "conflicts", "obsoletes",
		NULL
	};
	int i;

	sql = "CREATE TABLE packages ("
		"  pkgKey INTEGER PRIMARY KEY,"
		"  pkgId TEXT,"
		"  name TEXT,"
		"  arch TEXT,"
		"  version TEXT,"
		"  epoch TEXT,"
		"  release TEXT,"
		"  summary TEXT,"
		"  description TEXT,"
		"  url TEXT,"
		"  time_file INTEGER,"
		"  time_build INTEGER,"
		"  rpm_license TEXT,"
		"  rpm_vendor TEXT,"
		"  rpm_group TEXT,"
		"  rpm_buildhost TEXT,"
		"  rpm_sourcerpm TEXT,"
		"  rpm_header_start INTEGER,"
		"  rpm_header_end INTEGER,"
		"  rpm_packager TEXT,"
		"  size_package INTEGER,"
		"  size_installed INTEGER,"
		"  size_archive INTEGER,"
		"  location_href TEXT,"
		"  location_base TEXT," "  checksum_type TEXT)";

	rc = sqlite3_exec (db, sql, NULL, NULL, NULL);
	if (rc != SQLITE_OK) {
		return;
	}

	sql = "CREATE TABLE files ("
		"  name TEXT," "  type TEXT," "  pkgKey INTEGER)";
	rc = sqlite3_exec (db, sql, NULL, NULL, NULL);
	if (rc != SQLITE_OK) {
		return;
	}

	for (i = 0; deps[i]; i++) {
		const char *prereq;
		char *query;

		if (!strcmp (deps[i], "requires")) {
			prereq = ", pre BOOLEAN DEFAULT FALSE";
		} else
			prereq = "";

		query = g_strdup_printf (DEP_TABLE, deps[i], prereq);
		rc = sqlite3_exec (db, query, NULL, NULL, NULL);
		free (query);

		if (rc != SQLITE_OK) {
			return;
		}
	}

	sql = "CREATE TRIGGER removals AFTER DELETE ON packages"
		"  BEGIN"
		"    DELETE FROM files WHERE pkgKey = old.pkgKey;"
		"    DELETE FROM requires WHERE pkgKey = old.pkgKey;"
		"    DELETE FROM provides WHERE pkgKey = old.pkgKey;"
		"    DELETE FROM conflicts WHERE pkgKey = old.pkgKey;"
		"    DELETE FROM obsoletes WHERE pkgKey = old.pkgKey;" "  END;";

	rc = sqlite3_exec (db, sql, NULL, NULL, NULL);
	if (rc != SQLITE_OK) {
		return;
	}
}

#define PACKAGE_INDEX "CREATE INDEX IF NOT EXISTS pkg%s on %s (pkgKey)"
#define NAME_INDEX "CREATE INDEX IF NOT EXISTS %sname ON %s (name)"

static void
index_primary_tables (sqlite3 *db)
{
	int rc;
	const char *sql;

	const char *deps[] = { "requires", "provides", "conflicts", "obsoletes",
		NULL
	};
	int i;

	sql = "CREATE INDEX IF NOT EXISTS packagename ON packages (name)";
	rc = sqlite3_exec (db, sql, NULL, NULL, NULL);
	if (rc != SQLITE_OK) {
		return;
	}

	sql = "CREATE INDEX IF NOT EXISTS packageId ON packages (pkgId)";
	rc = sqlite3_exec (db, sql, NULL, NULL, NULL);
	if (rc != SQLITE_OK) {
		return;
	}

	sql = "CREATE INDEX IF NOT EXISTS filenames ON files (name)";
	rc = sqlite3_exec (db, sql, NULL, NULL, NULL);
	if (rc != SQLITE_OK) {
		return;
	}

	for (i = 0; deps[i]; i++) {
		char *query;

		query = g_strdup_printf (PACKAGE_INDEX, deps[i], deps[i]);
		rc = sqlite3_exec (db, query, NULL, NULL, NULL);
		free (query);

		if (rc != SQLITE_OK) {
			return;
		}

		query = g_strdup_printf (NAME_INDEX, deps[i], deps[i]);
		rc = sqlite3_exec (db, query, NULL, NULL, NULL);
		if (rc != SQLITE_OK) {
			return;
		}
	}
}

static void
build_primary_schema (sqlite3 *primary)
{
	create_dbinfo_table (primary);
	create_primary_tables (primary);
	index_primary_tables (primary);
}

static void
create_filelist_tables (sqlite3 *db)
{
	int rc;
	const char *sql;

	sql = "CREATE TABLE packages ("
		"  pkgKey INTEGER PRIMARY KEY," "  pkgId TEXT)";
	rc = sqlite3_exec (db, sql, NULL, NULL, NULL);
	if (rc != SQLITE_OK) {
		return;
	}

	sql = "CREATE TABLE filelist ("
		"  pkgKey INTEGER,"
		"  dirname TEXT," "  filenames TEXT," "  filetypes TEXT)";
	rc = sqlite3_exec (db, sql, NULL, NULL, NULL);
	if (rc != SQLITE_OK) {
		return;
	}

	sql = "CREATE TRIGGER remove_filelist AFTER DELETE ON packages"
		"  BEGIN"
		"    DELETE FROM filelist WHERE pkgKey = old.pkgKey;" "  END;";

	rc = sqlite3_exec (db, sql, NULL, NULL, NULL);
	if (rc != SQLITE_OK) {
		return;
	}
}

static void
index_filelist_tables (sqlite3 *db)
{
	int rc;
	const char *sql;

	sql = "CREATE INDEX IF NOT EXISTS keyfile ON filelist (pkgKey)";
	rc = sqlite3_exec (db, sql, NULL, NULL, NULL);
	if (rc != SQLITE_OK) {
		return;
	}

	sql = "CREATE INDEX IF NOT EXISTS pkgId ON packages (pkgId)";
	rc = sqlite3_exec (db, sql, NULL, NULL, NULL);
	if (rc != SQLITE_OK) {
		return;
	}

	sql = "CREATE INDEX IF NOT EXISTS dirnames ON filelist (dirname)";
	rc = sqlite3_exec (db, sql, NULL, NULL, NULL);
	if (rc != SQLITE_OK) {
		return;
	}
}

static void
build_filelists_schema (sqlite3 *filelists)
{
	create_dbinfo_table (filelists);
	create_filelist_tables (filelists);
	index_filelist_tables (filelists);
}

LowSqliteImporter *
low_sqlite_importer_new (const char *directory)
{
	char *primary_file = g_strdup_printf ("%s/primary.sqlite", directory);
	char *filelists_file = g_strdup_printf ("%s/filelists.sqlite",
						directory);

	LowSqliteImporter *importer = malloc (sizeof (LowSqliteImporter));

	unlink (primary_file);
	unlink (filelists_file);

	sqlite3_open (primary_file, &(importer->primary_db));
	sqlite3_open (filelists_file, &(importer->filelists_db));

	free (primary_file);
	free (filelists_file);

	build_primary_schema (importer->primary_db);
	build_filelists_schema (importer->filelists_db);

	importer->pkg_stmt = primary_package_prepare (importer->primary_db);

	return importer;
}

void
low_sqlite_importer_free (LowSqliteImporter *importer)
{
	sqlite3_exec (importer->primary_db, "COMMIT", NULL, NULL, NULL);
	sqlite3_exec (importer->filelists_db, "COMMIT", NULL, NULL, NULL);

	sqlite3_close (importer->primary_db);
	sqlite3_close (importer->filelists_db);

	free (importer);
}

/* vim: set ts=8 sw=8 noet: */
