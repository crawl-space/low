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

#include "low-fake-repo.h"

/**
 * Stub in-memory replacement for a package repository.
 */

LowRepo *
low_fake_repo_initialize (const char *id, const char *name, gboolean enabled)
{
	LowFakeRepo *repo = malloc (sizeof (LowFakeRepo));

	repo->super.id = strdup (id);
	repo->super.name = strdup (name);
	repo->super.enabled = enabled;

	/* Set this yourself */
	repo->packages = NULL;

	return (LowRepo *) repo;
}

void
low_fake_repo_shutdown (LowRepo *repo)
{
	free (repo);
}

typedef gboolean (*LowFakePackageIterFilterFn) (LowPackage *pkg, gpointer data);

typedef struct _LowFakePackageIter {
	LowPackageIter super;
	int position; /** current package in the list we're examining */
	LowFakePackageIterFilterFn func;
	gpointer data;
} LowFakePackageIter;

static LowPackageIter *
low_fake_repo_fake_iter_next (LowPackageIter *iter)
{
	LowFakePackageIter *iter_fake = (LowFakePackageIter *) iter;
	LowFakeRepo *repo_fake = (LowFakeRepo *) iter->repo;

	if (repo_fake->packages[iter_fake->position] == NULL) {
		free (iter_fake);
		return NULL;
	}

	iter->pkg = repo_fake->packages[iter_fake->position++];
	low_package_ref (iter->pkg);

	if (iter_fake->func != NULL) {
		/* move on to the next rpm if this one fails the filter */
		if (!(iter_fake->func) (iter->pkg, iter_fake->data)) {
			return low_package_iter_next (iter);
		}
	}

	return iter;
}

LowPackageIter *
low_fake_repo_list_all (LowRepo *repo)
{
	LowFakePackageIter *iter = malloc (sizeof (LowFakePackageIter));
	iter->super.repo = repo;
	iter->super.next_func = low_fake_repo_fake_iter_next;
	iter->super.pkg = NULL;

	iter->position = 0;
	iter->func = NULL;
	iter->data = NULL;

	return (LowPackageIter *) iter;
}

static gboolean
low_fake_repo_list_by_name_filter_fn (LowPackage *pkg, gpointer data)
{
	const char *name = (const char *) data;

	if (strcmp (pkg->name, name) == 0) {
		return TRUE;
	}

	return FALSE;
}

LowPackageIter *
low_fake_repo_list_by_name (LowRepo *repo, const char *name)
{
	LowFakePackageIter *iter = malloc (sizeof (LowFakePackageIter));
	iter->super.repo = repo;
	iter->super.next_func = low_fake_repo_fake_iter_next;
	iter->super.pkg = NULL;

	iter->position = 0;
	iter->func = low_fake_repo_list_by_name_filter_fn;
	iter->data = g_strdup (name);

	return (LowPackageIter *) iter;
}

static gboolean
low_fake_repo_search_provides_filter_fn (LowPackage *pkg, gpointer data)
{
	int i;
	LowPackageDependency **deps = low_package_get_provides (pkg);
	const LowPackageDependency *query_dep =
		(const LowPackageDependency *) data;

	for (i = 0; deps[i] != NULL; i++) {
		if (low_package_dependency_satisfies (query_dep, deps[i])) {
			return TRUE;
		}
	}

	return FALSE;
}

LowPackageIter *
low_fake_repo_search_provides (LowRepo *repo,
			       const LowPackageDependency *provides)
{
	LowFakePackageIter *iter = malloc (sizeof (LowFakePackageIter));
	iter->super.repo = repo;
	iter->super.next_func = low_fake_repo_fake_iter_next;
	iter->super.pkg = NULL;

	iter->position = 0;
	iter->func = low_fake_repo_search_provides_filter_fn;
	iter->data = low_package_dependency_new (provides->name,
						 provides->sense,
						 provides->evr);

	return (LowPackageIter *) iter;
}

static gboolean
low_fake_repo_search_requires_filter_fn (LowPackage *pkg, gpointer data)
{
	int i;
	LowPackageDependency **deps = low_package_get_requires (pkg);
	const LowPackageDependency *query_dep =
		(const LowPackageDependency *) data;

	for (i = 0; deps[i] != NULL; i++) {
		if (low_package_dependency_satisfies (query_dep, deps[i])) {
			return TRUE;
		}
	}

	return FALSE;
}

LowPackageIter *
low_fake_repo_search_requires (LowRepo *repo,
			       const LowPackageDependency *requires)
{
	LowFakePackageIter *iter = malloc (sizeof (LowFakePackageIter));
	iter->super.repo = repo;
	iter->super.next_func = low_fake_repo_fake_iter_next;
	iter->super.pkg = NULL;

	iter->position = 0;
	iter->func = low_fake_repo_search_requires_filter_fn;
	iter->data = low_package_dependency_new (requires->name,
						 requires->sense,
						 requires->evr);

	return (LowPackageIter *) iter;
}

static gboolean
low_fake_repo_search_conflicts_filter_fn (LowPackage *pkg, gpointer data)
{
	int i;
	LowPackageDependency **deps = low_package_get_conflicts (pkg);
	const LowPackageDependency *query_dep =
		(const LowPackageDependency *) data;

	for (i = 0; deps[i] != NULL; i++) {
		if (low_package_dependency_satisfies (query_dep, deps[i])) {
			return TRUE;
		}
	}

	return FALSE;
}

LowPackageIter *
low_fake_repo_search_conflicts (LowRepo *repo,
				const LowPackageDependency *conflicts)
{
	LowFakePackageIter *iter = malloc (sizeof (LowFakePackageIter));
	iter->super.repo = repo;
	iter->super.next_func = low_fake_repo_fake_iter_next;
	iter->super.pkg = NULL;

	iter->position = 0;
	iter->func = low_fake_repo_search_conflicts_filter_fn;
	iter->data = low_package_dependency_new (conflicts->name,
						 conflicts->sense,
						 conflicts->evr);

	return (LowPackageIter *) iter;
}

static gboolean
low_fake_repo_search_obsoletes_filter_fn (LowPackage *pkg, gpointer data)
{
	int i;
	LowPackageDependency **deps = low_package_get_obsoletes (pkg);
	const LowPackageDependency *query_dep =
		(const LowPackageDependency *) data;

	for (i = 0; deps[i] != NULL; i++) {
		if (low_package_dependency_satisfies (query_dep, deps[i])) {
			return TRUE;
		}
	}

	return FALSE;
}

LowPackageIter *
low_fake_repo_search_obsoletes (LowRepo *repo,
				const LowPackageDependency *obsoletes)
{
	LowFakePackageIter *iter = malloc (sizeof (LowFakePackageIter));
	iter->super.repo = repo;
	iter->super.next_func = low_fake_repo_fake_iter_next;
	iter->super.pkg = NULL;

	iter->position = 0;
	iter->func = low_fake_repo_search_obsoletes_filter_fn;
	iter->data = low_package_dependency_new (obsoletes->name,
						 obsoletes->sense,
						 obsoletes->evr);

	return (LowPackageIter *) iter;
}

static gboolean
low_fake_repo_search_files_filter_fn (LowPackage *pkg, gpointer data)
{
	int i;
	char **deps = low_package_get_files (pkg);
	const char *querystr = (const char *) data;

	for (i = 0; deps[i] != NULL; i++) {
		if (!strcmp (querystr, deps[i])) {
			return TRUE;
		}
	}

	return FALSE;
}

LowPackageIter *
low_fake_repo_search_files (LowRepo *repo, const char *file)
{
	LowFakePackageIter *iter = malloc (sizeof (LowFakePackageIter));
	iter->super.repo = repo;
	iter->super.next_func = low_fake_repo_fake_iter_next;
	iter->super.pkg = NULL;

	iter->position = 0;
	iter->func = low_fake_repo_search_files_filter_fn;
	iter->data = g_strdup (file);

	return (LowPackageIter *) iter;
}

/**
 * Search name, summary, description and & url for the provided string.
 *
 */
LowPackageIter *
low_fake_repo_search_details (LowRepo *repo, const char *querystr G_GNUC_UNUSED)
{
	return low_fake_repo_list_all (repo);
}

/* vim: set ts=8 sw=8 noet: */
