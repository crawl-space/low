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
#include <stdio.h>
#include <string.h>
#include <sqlite3.h>
#include "low-package-sqlite.h"
#include "low-repo-sqlite.h"

#define SELECT_FIELDS_FROM "SELECT p.pkgKey, p.name, p.arch, p.version, " \
			   "p.release, p.size_package, p.summary, " \
			   "p.description, p.url, p.rpm_license, " \
			   "p.location_href FROM "

typedef struct _LowRepoSqlite {
	LowRepo super;
	sqlite3 *primary_db;
	sqlite3 *filelists_db;
} LowRepoSqlite;

static void
low_repo_sqlite_open_db (const char *db_file, sqlite3 **db)
{
	if (sqlite3_open (db_file, db)) {
//		fprintf (stderr, "Can't open database '%s': %s\n",
//				 primary_db, sqlite3_errmsg (repo->primary_db));
		sqlite3_close (*db);
//		exit(1);
	}
}

static void
attach_db (sqlite3 *db, const char *db_file)
{
	const char *stmt = "ATTACH :db_file AS :name";

	sqlite3_stmt *pp_stmt;

	sqlite3_prepare (db, stmt, -1, &pp_stmt, NULL);
	sqlite3_bind_text (pp_stmt, 1, db_file, -1, SQLITE_STATIC);
	sqlite3_bind_text (pp_stmt, 2, "filelists", -1, SQLITE_STATIC);

	sqlite3_step (pp_stmt);
	sqlite3_finalize (pp_stmt);
}

static void
detach_db (sqlite3 *db)
{
	const char *stmt = "DETACH :name";

	sqlite3_stmt *pp_stmt;

	sqlite3_prepare (db, stmt, -1, &pp_stmt, NULL);
	sqlite3_bind_text (pp_stmt, 1, "filelists", -1, SQLITE_STATIC);

	sqlite3_step (pp_stmt);
	sqlite3_finalize (pp_stmt);
}

static void
low_repo_sqlite_regexp (sqlite3_context *ctx, int argc G_GNUC_UNUSED,
			sqlite3_value **args)
{
	gboolean matched;

	const char *expr = (const char *) sqlite3_value_text (args[0]);
	const char *item = (const char *) sqlite3_value_text (args[1]);

	/* XXX Can we compile this and free it when done? */
	matched = g_regex_match_simple (expr, item, 0, 0);
	sqlite3_result_int (ctx, matched);
}

typedef void (*sqlFunc) (sqlite3_context *, int, sqlite3_value **);
typedef void (*sqlFinal) (sqlite3_context *);

LowRepo *
low_repo_sqlite_initialize (const char *id, const char *name, gboolean enabled)
{
	LowRepoSqlite *repo = malloc (sizeof (LowRepoSqlite));
	char *primary_db =
		g_strdup_printf ("/var/cache/yum/%s/primary.sqlite", id);
	char *filelists_db =
		g_strdup_printf ("/var/cache/yum/%s/filelists.sqlite", id);

	/* Will need a way to flick this on later */
	if (enabled) {
		low_repo_sqlite_open_db (primary_db, &repo->primary_db);
		attach_db (repo->primary_db, filelists_db);
		sqlite3_create_function (repo->primary_db, "regexp", 2,
					 SQLITE_ANY, NULL,
					 low_repo_sqlite_regexp,
					 (sqlFunc) NULL, (sqlFinal) NULL);
	//	low_repo_sqlite_open_db (filelists_db, &repo->filelists_db);
	} else {
		repo->primary_db = NULL;
	}

	repo->super.id = g_strdup (id);
	repo->super.name = g_strdup (name);
	repo->super.enabled = enabled;

	free (primary_db);
	free (filelists_db);

	return (LowRepo *) repo;
}

void
low_repo_sqlite_shutdown (LowRepo *repo)
{
	LowRepoSqlite *repo_sqlite = (LowRepoSqlite *) repo;

	free (repo->id);
	free (repo->name);

	if (repo_sqlite->primary_db) {
		detach_db (repo_sqlite->primary_db);
		sqlite3_close (repo_sqlite->primary_db);
	//	sqlite3_close (repo_sqlite->filelists_db);
	}
	free (repo);
}

LowPackageIter *
low_repo_sqlite_list_all (LowRepo *repo)
{
	const char *stmt = SELECT_FIELDS_FROM "packages p";

	LowRepoSqlite *repo_sqlite = (LowRepoSqlite *) repo;
	LowPackageIterSqlite *iter = malloc (sizeof (LowPackageIterSqlite));
	iter->super.repo = repo;
	iter->super.next_func = low_sqlite_package_iter_next;
	iter->super.pkg = NULL;

	sqlite3_prepare (repo_sqlite->primary_db, stmt, -1, &iter->pp_stmt,
			 NULL);
	return (LowPackageIter *) iter;
}

LowPackageIter *
low_repo_sqlite_list_by_name (LowRepo *repo, const char *name)
{
	const char *stmt = SELECT_FIELDS_FROM "packages p WHERE name = :name";

	LowRepoSqlite *repo_sqlite = (LowRepoSqlite *) repo;
	LowPackageIterSqlite *iter = malloc (sizeof (LowPackageIterSqlite));
	iter->super.repo = repo;
	iter->super.next_func = low_sqlite_package_iter_next;
	iter->super.pkg = NULL;

	sqlite3_prepare (repo_sqlite->primary_db, stmt, -1, &iter->pp_stmt,
			 NULL);
	sqlite3_bind_text (iter->pp_stmt, 1, name, -1, SQLITE_STATIC);
	return (LowPackageIter *) iter;
}

LowPackageIter *
low_repo_sqlite_search_provides (LowRepo *repo, const char *provides)
{
	const char *stmt = SELECT_FIELDS_FROM "packages p, provides pr "
			   "WHERE pr.pkgKey = p.pkgKey AND pr.name = :provides";

	LowRepoSqlite *repo_sqlite = (LowRepoSqlite *) repo;
	LowPackageIterSqlite *iter = malloc (sizeof (LowPackageIterSqlite));
	iter->super.repo = repo;
	iter->super.next_func = low_sqlite_package_iter_next;
	iter->super.pkg = NULL;

	sqlite3_prepare (repo_sqlite->primary_db, stmt, -1, &iter->pp_stmt,
			 NULL);
	sqlite3_bind_text (iter->pp_stmt, 1, provides, -1, SQLITE_STATIC);
	return (LowPackageIter *) iter;
}

LowPackageIter *
low_repo_sqlite_search_requires (LowRepo *repo, const char *requires)
{
	const char *stmt = SELECT_FIELDS_FROM " packages p, requires req "
			   "WHERE req.pkgKey = p.pkgKey "
			   "AND req.name = :requires";

	LowRepoSqlite *repo_sqlite = (LowRepoSqlite *) repo;
	LowPackageIterSqlite *iter = malloc (sizeof (LowPackageIterSqlite));
	iter->super.repo = repo;
	iter->super.next_func = low_sqlite_package_iter_next;
	iter->super.pkg = NULL;

	sqlite3_prepare (repo_sqlite->primary_db, stmt, -1, &iter->pp_stmt,
			 NULL);
	sqlite3_bind_text (iter->pp_stmt, 1, requires, -1, SQLITE_STATIC);
	return (LowPackageIter *) iter;
}

LowPackageIter *
low_repo_sqlite_search_conflicts (LowRepo *repo, const char *conflicts)
{
	const char *stmt = SELECT_FIELDS_FROM "packages p, conflicts conf "
			   "WHERE conf.pkgKey = p.pkgKey "
			   "AND conf.name = :conflicts";

	LowRepoSqlite *repo_sqlite = (LowRepoSqlite *) repo;
	LowPackageIterSqlite *iter = malloc (sizeof (LowPackageIterSqlite));
	iter->super.repo = repo;
	iter->super.next_func = low_sqlite_package_iter_next;
	iter->super.pkg = NULL;

	sqlite3_prepare (repo_sqlite->primary_db, stmt, -1, &iter->pp_stmt,
			 NULL);
	sqlite3_bind_text (iter->pp_stmt, 1, conflicts, -1, SQLITE_STATIC);
	return (LowPackageIter *) iter;
}

LowPackageIter *
low_repo_sqlite_search_obsoletes (LowRepo *repo, const char *obsoletes)
{
	const char *stmt = SELECT_FIELDS_FROM "packages p, obsoletes obs "
			   "WHERE obs.pkgKey = p.pkgKey "
			   "AND obs.name = :obsoletes";

	LowRepoSqlite *repo_sqlite = (LowRepoSqlite *) repo;
	LowPackageIterSqlite *iter = malloc (sizeof (LowPackageIterSqlite));
	iter->super.repo = repo;
	iter->super.next_func = low_sqlite_package_iter_next;
	iter->super.pkg = NULL;

	sqlite3_prepare (repo_sqlite->primary_db, stmt, -1, &iter->pp_stmt,
			 NULL);
	sqlite3_bind_text (iter->pp_stmt, 1, obsoletes, -1, SQLITE_STATIC);
	return (LowPackageIter *) iter;
}

static LowPackageIter *
low_repo_sqlite_search_primary_files (LowRepo *repo, const char *file)
{
	const char *stmt = SELECT_FIELDS_FROM "packages p, files f "
			   "WHERE f.pkgKey = p.pkgKey "
			   "AND f.name = :file";

	LowRepoSqlite *repo_sqlite = (LowRepoSqlite *) repo;
	LowPackageIterSqlite *iter = malloc (sizeof (LowPackageIterSqlite));
	iter->super.repo = repo;
	iter->super.next_func = low_sqlite_package_iter_next;
	iter->super.pkg = NULL;

	sqlite3_prepare (repo_sqlite->primary_db, stmt, -1, &iter->pp_stmt,
			 NULL);
	sqlite3_bind_text (iter->pp_stmt, 1, file, -1, SQLITE_STATIC);
	return (LowPackageIter *) iter;
}

static LowPackageIter *
low_repo_sqlite_search_filelists_files (LowRepo *repo, const char *file)
{
	/* XXX try optimizing for caces where there's only one filename */
	const char *stmt = SELECT_FIELDS_FROM "packages p, filelist f "
			   "WHERE f.pkgKey = p.pkgKey "
			   "AND f.dirname = :dir "
			   "AND f.filenames REGEXP :file";

	char *slash = g_strrstr(file, "/");
	char *filename = g_strdup_printf ("(^|/)%s(/|$)", (char *) (slash + 1));
	char *dirname = g_strndup (file, slash - file);

	LowRepoSqlite *repo_sqlite = (LowRepoSqlite *) repo;
	LowPackageIterSqlite *iter = malloc (sizeof (LowPackageIterSqlite));
	iter->super.repo = repo;
	iter->super.next_func = low_sqlite_package_iter_next;
	iter->super.pkg = NULL;

	sqlite3_prepare (repo_sqlite->primary_db, stmt, -1, &iter->pp_stmt,
			 NULL);
	sqlite3_bind_text (iter->pp_stmt, 1, dirname, -1, free);
	sqlite3_bind_text (iter->pp_stmt, 2, filename, -1, free);

	return (LowPackageIter *) iter;
}

LowPackageIter *
low_repo_sqlite_search_files (LowRepo *repo, const char *file)
{
	/* Createrepo puts these files in primary.xml */
	if (strstr (file, "bin/") || g_str_has_prefix (file, "/etc/")
	    || !strcmp (file, "/usr/lib/sendmail")) {
		return low_repo_sqlite_search_primary_files (repo, file);
	} else {
		return low_repo_sqlite_search_filelists_files (repo, file);
	}
}

/**
 * Search name, summary, description and & url for the provided string.
 *
 * XXX this function needs a better name
 */
LowPackageIter *
low_repo_sqlite_generic_search (LowRepo *repo, const char *querystr)
{
	const char *stmt = SELECT_FIELDS_FROM "packages p "
			   "WHERE p.name LIKE :query OR "
			   "p.summary LIKE :query OR "
			   "p.description LIKE :query OR "
			   "p.url LIKE :query";

	char *like_querystr = g_strdup_printf ("%%%s%%", querystr);

	LowRepoSqlite *repo_sqlite = (LowRepoSqlite *) repo;
	LowPackageIterSqlite *iter = malloc (sizeof (LowPackageIterSqlite));
	iter->super.repo = repo;
	iter->super.next_func = low_sqlite_package_iter_next;
	iter->super.pkg = NULL;

	sqlite3_prepare (repo_sqlite->primary_db, stmt, -1, &iter->pp_stmt,
			 NULL);
	sqlite3_bind_text (iter->pp_stmt, 1, like_querystr, -1, free);

	return (LowPackageIter *) iter;
}


#define MAX_PROVIDES 1000

char **
low_repo_sqlite_get_provides (LowRepo *repo, LowPackage *pkg)
{
	const char *stmt = "SELECT prov.name from provides prov "
			   "WHERE prov.pkgKey = :pkgKey";

	/* XXX make this dynamic */
	char **provides = malloc (sizeof (char *) * MAX_PROVIDES);
	int i = 0;

	sqlite3_stmt *pp_stmt;
	LowRepoSqlite *repo_sqlite = (LowRepoSqlite *) repo;

	sqlite3_prepare (repo_sqlite->primary_db, stmt, -1, &pp_stmt,
			 NULL);
	sqlite3_bind_int (pp_stmt, 1, pkg->id);

	while (sqlite3_step(pp_stmt) != SQLITE_DONE) {
		/* XXX Do we need to strdup this? */
		provides[i++] =
			g_strdup ((const char *) sqlite3_column_text (pp_stmt,
								      0));
	}

	sqlite3_finalize (pp_stmt);

	provides[i] = NULL;
	return provides;
}

/* vim: set ts=8 sw=8 noet: */
