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
		char *name = low_config_get_string (config, repo_names[i], "name");
		gboolean enabled = low_config_get_boolean (config, repo_names[i],
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

/* vim: set ts=8 sw=8 noet: */
