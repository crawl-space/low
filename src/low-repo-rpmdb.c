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
#include <fcntl.h>
#include <rpm/rpmlib.h>
#include <rpm/rpmdb.h>
#include "low-repo-rpmdb.h"
#include "low-package-rpmdb.h"

typedef struct _LowRepoRpmdb {
	LowRepo super;
	rpmdb db;
} LowRepoRpmdb;

LowRepo *
low_repo_rpmdb_initialize ()
{
	LowRepoRpmdb *repo = malloc (sizeof (LowRepoRpmdb));

	repo->super.id = "rpmdb";
	repo->super.name = "installed";
	repo->super.enabled = TRUE;

	rpmReadConfigFiles (NULL, NULL);
	if (rpmdbOpen ("", &repo->db, O_RDONLY, 0644) != 0) {
		fprintf (stderr, "Cannot open rpm database\n");
		exit (1);
	}

	return (LowRepo *) repo;
}

void
low_repo_rpmdb_shutdown (LowRepo *repo)
{
	LowRepoRpmdb *repo_rpmdb = (LowRepoRpmdb *) repo;
	rpmdbClose(repo_rpmdb->db);
	free (repo);
}

LowPackageIter *
low_repo_rpmdb_list_all (LowRepo *repo)
{
	LowRepoRpmdb *repo_rpmdb = (LowRepoRpmdb *) repo;
	LowPackageIterRpmdb *iter = malloc (sizeof (LowPackageIterRpmdb));
	iter->super.repo = repo;
	iter->super.pkg = NULL;

	iter->rpm_iter = rpmdbInitIterator (repo_rpmdb->db, 0, NULL, 0);
	return (LowPackageIter *) iter;
}

LowPackageIter *
low_repo_rpmdb_list_by_name (LowRepo *repo, const char *name)
{
	LowRepoRpmdb *repo_rpmdb = (LowRepoRpmdb *) repo;
	LowPackageIterRpmdb *iter = malloc (sizeof (LowPackageIterRpmdb));
	iter->super.repo = repo;
	iter->super.pkg = NULL;

	iter->rpm_iter = rpmdbInitIterator (repo_rpmdb->db, RPMTAG_NAME, name, 0);
	return (LowPackageIter *) iter;
}

LowPackageIter *
low_repo_rpmdb_search_provides (LowRepo *repo, const char *provides)
{
	LowRepoRpmdb *repo_rpmdb = (LowRepoRpmdb *) repo;
	LowPackageIterRpmdb *iter = malloc (sizeof (LowPackageIterRpmdb));
	iter->super.repo = repo;
	iter->super.pkg = NULL;

	iter->rpm_iter = rpmdbInitIterator (repo_rpmdb->db, RPMTAG_PROVIDENAME,
										provides, 0);
	return (LowPackageIter *) iter;
}

LowPackageIter *
low_repo_rpmdb_search_requires (LowRepo *repo, const char *requires)
{
   LowRepoRpmdb *repo_rpmdb = (LowRepoRpmdb *) repo;
   LowPackageIterRpmdb *iter = malloc (sizeof (LowPackageIterRpmdb));
   iter->super.pkg = NULL;

   iter->rpm_iter = rpmdbInitIterator (repo_rpmdb->db, RPMTAG_REQUIRENAME,
                                       requires, 0);
   return (LowPackageIter *) iter;
}

