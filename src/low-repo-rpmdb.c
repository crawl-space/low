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
#include <string.h>
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

	repo->super.id = "installed";
	repo->super.name = "Installed Packages";
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

	rpmdbClose (repo_rpmdb->db);
	rpmFreeRpmrc ();

	free (repo);
}

LowPackageIter *
low_repo_rpmdb_search (LowRepo *repo, int_32 tag, const char *querystr)
{
	LowRepoRpmdb *repo_rpmdb = (LowRepoRpmdb *) repo;
	LowPackageIterRpmdb *iter = malloc (sizeof (LowPackageIterRpmdb));
	iter->super.repo = repo;
	iter->super.next_func = low_package_iter_rpmdb_next;
	iter->super.pkg = NULL;

	iter->func = NULL;

	iter->rpm_iter = rpmdbInitIterator (repo_rpmdb->db, tag, querystr, 0);
	return (LowPackageIter *) iter;
}

LowPackageIter *
low_repo_rpmdb_list_all (LowRepo *repo)
{
	return low_repo_rpmdb_search (repo, 0, NULL);
}

LowPackageIter *
low_repo_rpmdb_list_by_name (LowRepo *repo, const char *name)
{
	return low_repo_rpmdb_search (repo, RPMTAG_NAME, name);
}

LowPackageIter *
low_repo_rpmdb_search_provides (LowRepo *repo, const char *provides)
{
	return low_repo_rpmdb_search (repo, RPMTAG_PROVIDENAME, provides);
}

LowPackageIter *
low_repo_rpmdb_search_requires (LowRepo *repo, const char *requires)
{
	return low_repo_rpmdb_search (repo, RPMTAG_REQUIRENAME, requires);
}

LowPackageIter *
low_repo_rpmdb_search_conflicts (LowRepo *repo, const char *conflicts)
{
	return low_repo_rpmdb_search (repo, RPMTAG_CONFLICTNAME, conflicts);
}

LowPackageIter *
low_repo_rpmdb_search_obsoletes (LowRepo *repo, const char *obsoletes)
{
	/* XXX This seems to be broken in RPM itself. */
	return low_repo_rpmdb_search (repo, RPMTAG_OBSOLETENAME, obsoletes);
}

LowPackageIter *
low_repo_rpmdb_search_files (LowRepo *repo, const char *file)
{
	return low_repo_rpmdb_search (repo, RPMTAG_BASENAMES, file);
}

static gboolean
low_repo_rpmdb_generic_search_filter_fn (LowPackage *pkg, gpointer data)
{
	gchar *querystr = (gchar *) data;

	/* url can be NULL, so check first. */
	if (strstr (pkg->name, querystr) ||
	    strstr (pkg->summary, querystr) ||
	    strstr (pkg->description, querystr) ||
	    (pkg->url != NULL && strstr (pkg->url, querystr))) {
		return TRUE;
	} else {
		return FALSE;
	}
}

LowPackageIter *
low_repo_rpmdb_generic_search (LowRepo *repo, const char *querystr)
{
	LowRepoRpmdb *repo_rpmdb = (LowRepoRpmdb *) repo;
	LowPackageIterRpmdb *iter = malloc (sizeof (LowPackageIterRpmdb));
	iter->super.repo = repo;
	iter->super.next_func = low_package_iter_rpmdb_next;
	iter->super.pkg = NULL;

	iter->func = low_repo_rpmdb_generic_search_filter_fn;
	iter->filter_data = (gpointer) querystr;

	iter->rpm_iter = rpmdbInitIterator (repo_rpmdb->db, 0, NULL, 0);
	return (LowPackageIter *) iter;
}

/* vim: set ts=8 sw=8 noet: */
