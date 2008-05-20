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
#include "low-repo-sqlite.h"
#include "low-repo-set.h"


LowRepoSet *
low_repo_set_initialize_from_config (LowConfig *config)
{
	int i;
	char **repo_names;
	LowRepoSet *repo_set = malloc (sizeof (LowRepoSet));

	repo_set->repos = g_hash_table_new (NULL, NULL);

	repo_names = low_config_get_repo_names (config);
	for (i = 0; i < g_strv_length (repo_names); i++) {
		char *id = repo_names[i];
		char *name =
			low_config_get_string (config, repo_names[i], "name");
		gboolean enabled = low_config_get_boolean (config,
							   repo_names[i],
							   "enabled");
		LowRepo *repo = low_repo_sqlite_initialize (id, name, enabled);

		g_free (name);

		g_hash_table_insert (repo_set->repos, id, repo);
	}
	g_strfreev (repo_names);

	return repo_set;
}

static void
low_repo_set_free_repo (gpointer key, gpointer value, gpointer user_data)
{
	LowRepo *repo = (LowRepo *) value;

	low_repo_sqlite_shutdown (repo);
}

void
low_repo_set_free (LowRepoSet *repo_set)
{
	g_hash_table_foreach (repo_set->repos, low_repo_set_free_repo, NULL);
	free (repo_set);
}

typedef struct _LowRepoSetForEachData {
	LowRepoSetFilter filter;
	LowRepoSetFunc func;
	gpointer data;
} LowRepoSetForEachData;

static void
low_repo_set_inner_for_each (gpointer key, gpointer value, gpointer data)
{
	LowRepo *repo = (LowRepo *) value;
	LowRepoSetForEachData *for_each_data = (LowRepoSetForEachData *) data;

	if (for_each_data->filter == ALL ||
		(for_each_data->filter == ENABLED && repo->enabled) ||
		(for_each_data->filter == DISABLED && !repo->enabled)) {
		(for_each_data->func) (repo, for_each_data->data);
	}
}

void
low_repo_set_for_each (LowRepoSet *repo_set, LowRepoSetFilter filter,
		       LowRepoSetFunc func, gpointer data)
{
	LowRepoSetForEachData for_each_data;
	for_each_data.filter = filter;
	for_each_data.func = func;
	for_each_data.data = data;

	g_hash_table_foreach (repo_set->repos, low_repo_set_inner_for_each,
						  &for_each_data);
}

typedef LowPackageIter * (*LowRepoSetIterSearchFunc) (LowRepo *repo,
						      const gchar *searchstr);

typedef struct _LowRepoSetPackageIter {
	LowPackageIter super;
	GHashTableIter *repo_iter;
	LowPackageIter *current_repo_iter;
	LowRepo *current_repo;
	const gchar *search_data;
	LowRepoSetIterSearchFunc search_func;
} LowRepoSetPackageIter;

LowPackageIter *
low_repo_set_package_iter_next (LowPackageIter *iter)
{
	LowRepoSetPackageIter *iter_set = (LowRepoSetPackageIter *) iter;
	LowRepo *current_repo = iter_set->current_repo;
	LowPackageIter *current_repo_iter = iter_set->current_repo_iter;

	current_repo_iter = low_package_iter_next (current_repo_iter);

	/* This should cover repos that return 0 packages from the iter */
	while (current_repo_iter == NULL && current_repo != NULL) {
		do {
			current_repo = NULL;
			g_hash_table_iter_next (iter_set->repo_iter, NULL,
						(gpointer) &current_repo);
		} while (current_repo != NULL && !current_repo->enabled);
		if (current_repo == NULL) {
			current_repo_iter = NULL;
			break;
		}
		current_repo_iter =
			(iter_set->search_func) (current_repo,
						 iter_set->search_data);
		current_repo_iter = low_package_iter_next (current_repo_iter);
	}

	if (current_repo_iter == NULL) {
		free (iter);
		return NULL;
	}

	iter->pkg = iter_set->current_repo_iter->pkg;

	return iter;
}

static LowPackageIter *
low_repo_set_package_iter_new (LowRepoSet *repo_set,
			       LowRepoSetIterSearchFunc search_func,
			       const char *searchstr)
{
	LowRepoSetPackageIter *iter = malloc (sizeof (LowRepoSetPackageIter));
	iter->super.next_func = low_repo_set_package_iter_next;
	iter->super.pkg = NULL;
	iter->repo_iter = malloc (sizeof (GHashTableIter));
	g_hash_table_iter_init(iter->repo_iter, repo_set->repos);

	/* XXX deal with an empty hashtable. */
	do {
		g_hash_table_iter_next (iter->repo_iter, NULL,
					(gpointer) &(iter->current_repo));
	} while (!iter->current_repo->enabled);

	iter->search_func = search_func;
	iter->current_repo_iter =
		(iter->search_func) (iter->current_repo, searchstr);
	iter->search_data = searchstr;

	return (LowPackageIter *) iter;
}

LowPackageIter *
low_repo_set_search_provides (LowRepoSet *repo_set, const char *provides)
{
	return low_repo_set_package_iter_new (repo_set,
					      low_repo_sqlite_search_provides,
					      provides);
}

LowPackageIter *
low_repo_set_search_requires (LowRepoSet *repo_set, const char *requires)
{
	return low_repo_set_package_iter_new (repo_set,
					      low_repo_sqlite_search_requires,
					      requires);
}

LowPackageIter *
low_repo_set_search_conflicts (LowRepoSet *repo_set, const char *conflicts)
{
	return low_repo_set_package_iter_new (repo_set,
					      low_repo_sqlite_search_conflicts,
					      conflicts);
}

LowPackageIter *
low_repo_set_search_obsoletes (LowRepoSet *repo_set, const char *obsoletes)
{
	return low_repo_set_package_iter_new (repo_set,
					      low_repo_sqlite_search_obsoletes,
					      obsoletes);
}

LowPackageIter *
low_repo_set_search_files (LowRepoSet *repo_set, const char *file)
{
	return low_repo_set_package_iter_new (repo_set,
					      low_repo_sqlite_search_files,
					      file);
}

/* vim: set ts=8 sw=8 noet: */
