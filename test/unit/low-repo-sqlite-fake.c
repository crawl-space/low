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
#include "low-repo-sqlite.h"
#include "low-repo-sqlite-fake.h"

/**
 * Stub in-memory replacement for the sqlite repo type.
 */

LowRepo *
low_repo_sqlite_initialize (const char *id, const char *name, gboolean enabled)
{
	LowRepoSqliteFake *repo = malloc (sizeof (LowRepoSqliteFake));

	repo->super.id = strdup (id);
	repo->super.name = strdup (name);
	repo->super.enabled = enabled;

	/* Set this yourself */
	repo->packages = NULL;

	return (LowRepo *) repo;
}

void
low_repo_sqlite_shutdown (LowRepo *repo)
{
	free (repo);
}

typedef struct _LowFakePackageIter {
	LowPackageIter super;
	int position; /** current package in the list we're examining */
} LowFakePackageIter;

LowPackageIter *
low_repo_sqlite_fake_iter_next (LowPackageIter *iter) {
	LowFakePackageIter *iter_fake = (LowFakePackageIter *) iter;
	LowRepoSqliteFake *repo_fake = (LowRepoSqliteFake *) iter->repo;

	if (repo_fake->packages[iter_fake->position] == NULL) {
		free (iter_fake);
		return NULL;
	}

	iter->pkg = repo_fake->packages[iter_fake->position++];

	return iter;
}

LowPackageIter *
low_repo_sqlite_list_all (LowRepo *repo)
{
	LowFakePackageIter *iter = malloc (sizeof (LowFakePackageIter));
	iter->super.repo = repo;
	iter->super.next_func = low_repo_sqlite_fake_iter_next;
	iter->super.pkg = NULL;

	iter->position = 0;

	return (LowPackageIter *) iter;
}

LowPackageIter *
low_repo_sqlite_list_by_name (LowRepo *repo, const char *name)
{
	return low_repo_sqlite_list_all (repo);
}

LowPackageIter *
low_repo_sqlite_search_provides (LowRepo *repo, const char *provides)
{
	return low_repo_sqlite_list_all (repo);
}

LowPackageIter *
low_repo_sqlite_search_requires (LowRepo *repo, const char *requires)
{
	return low_repo_sqlite_list_all (repo);
}

LowPackageIter *
low_repo_sqlite_search_conflicts (LowRepo *repo, const char *conflicts)
{
	return low_repo_sqlite_list_all (repo);
}

LowPackageIter *
low_repo_sqlite_search_obsoletes (LowRepo *repo, const char *obsoletes)
{
	return low_repo_sqlite_list_all (repo);
}

LowPackageIter *
low_repo_sqlite_search_files (LowRepo *repo, const char *file)
{
	return low_repo_sqlite_list_all (repo);
}

/**
 * Search name, summary, description and & url for the provided string.
 *
 * XXX this function needs a better name
 */
LowPackageIter *
low_repo_sqlite_generic_search (LowRepo *repo, const char *querystr)
{
	return low_repo_sqlite_list_all (repo);
}

/* vim: set ts=8 sw=8 noet: */
