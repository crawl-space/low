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

typedef struct _LowFakePackageIter {
	LowPackageIter super;
	int position; /** current package in the list we're examining */
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

	return (LowPackageIter *) iter;
}

LowPackageIter *
low_fake_repo_list_by_name (LowRepo *repo, const char *name G_GNUC_UNUSED)
{
	return low_fake_repo_list_all (repo);
}

LowPackageIter *
low_fake_repo_search_provides (LowRepo *repo,
			       const char *provides G_GNUC_UNUSED)
{
	return low_fake_repo_list_all (repo);
}

LowPackageIter *
low_fake_repo_search_requires (LowRepo *repo,
			       const char *requires G_GNUC_UNUSED)
{
	return low_fake_repo_list_all (repo);
}

LowPackageIter *
low_fake_repo_search_conflicts (LowRepo *repo,
				const char *conflicts G_GNUC_UNUSED)
{
	return low_fake_repo_list_all (repo);
}

LowPackageIter *
low_fake_repo_search_obsoletes (LowRepo *repo,
				const char *obsoletes G_GNUC_UNUSED)
{
	return low_fake_repo_list_all (repo);
}

LowPackageIter *
low_fake_repo_search_files (LowRepo *repo, const char *file G_GNUC_UNUSED)
{
	return low_fake_repo_list_all (repo);
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
