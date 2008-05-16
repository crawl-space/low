#include <stdlib.h>
#include <stdio.h>
#include <sqlite3.h>
#include "low-package-sqlite.h"
#include "low-repo-sqlite.h"

typedef struct _LowRepoSqlite {
	LowRepo super;
	sqlite3 *db;
} LowRepoSqlite;

LowRepo *
low_repo_sqlite_initialize (const char *id, const char *name, gboolean enabled)
{
	LowRepoSqlite *repo = malloc (sizeof (LowRepoSqlite));
	char *primary_db = g_strdup_printf ("/var/cache/yum/%s/primary.sqlite", id);

	if (sqlite3_open (primary_db, &repo->db)) {
//		fprintf (stderr, "Can't open database '%s': %s\n",
//				 primary_db, sqlite3_errmsg (repo->db));
		sqlite3_close (repo->db);
//		exit(1);
	}

	repo->super.id = g_strdup (id);
	repo->super.name = g_strdup (name);
	repo->super.enabled = enabled;

	return (LowRepo *) repo;
}

void
low_repo_sqlite_shutdown (LowRepo *repo)
{
	LowRepoSqlite *repo_sqlite = (LowRepoSqlite *) repo;
	sqlite3_close (repo_sqlite->db);
	free (repo);
}

LowPackageIter *
low_repo_sqlite_list_all (LowRepo *repo)
{
	const char *stmt = "SELECT name, arch, version, release, size_package,"
		  			   "summary, description, url, rpm_license  FROM packages";
	LowRepoSqlite *repo_sqlite = (LowRepoSqlite *) repo;
	LowPackageIterSqlite *iter = malloc (sizeof (LowPackageIterSqlite));
	iter->super.pkg = NULL;

	sqlite3_prepare (repo_sqlite->db, stmt, -1, &iter->pp_stmt, NULL);
	return (LowPackageIter *) iter;
}

LowPackageIter *
low_repo_sqlite_list_by_name (LowRepo *repo, const char *name)
{
	const char *stmt = "SELECT name, arch, version, release, size_package, "
		  			   "summary, description, url, rpm_license  FROM packages "
					   "WHERE name = :name";
	LowRepoSqlite *repo_sqlite = (LowRepoSqlite *) repo;
	LowPackageIterSqlite *iter = malloc (sizeof (LowPackageIterSqlite));
	iter->super.pkg = NULL;

	sqlite3_prepare (repo_sqlite->db, stmt, -1, &iter->pp_stmt, NULL);
	sqlite3_bind_text (iter->pp_stmt, 1, name, -1, SQLITE_STATIC);
	return (LowPackageIter *) iter;
}

LowPackageIter *
low_repo_sqlite_search_provides (LowRepo *repo, const char *provides)
{
	const char *stmt = "SELECT p.name, p.arch, p.version, p.release, "
					   "p.size_package, p.summary, p.description, p.url, "
					   "p.rpm_license FROM packages p, provides pr "
					   "WHERE pr.pkgKey = p.pkgKey AND pr.name = :provides";
	LowRepoSqlite *repo_sqlite = (LowRepoSqlite *) repo;
	LowPackageIterSqlite *iter = malloc (sizeof (LowPackageIterSqlite));
	iter->super.pkg = NULL;

	sqlite3_prepare (repo_sqlite->db, stmt, -1, &iter->pp_stmt, NULL);
	sqlite3_bind_text (iter->pp_stmt, 1, provides, -1, SQLITE_STATIC);
	return (LowPackageIter *) iter;
}
